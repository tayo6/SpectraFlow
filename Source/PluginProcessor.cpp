#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace SpectraFlow;

// ─────────────────────────────────────────────────────────────────────────────
// Bus layout factory
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor::BusesProperties SpectraFlowProcessor::createBusesLayout()
{
    BusesProperties props;

    // Main stereo in/out (pass-through — we never modify audio)
    props.addBus(true,  "Main Input",  juce::AudioChannelSet::stereo(), true);
    props.addBus(false, "Main Output", juce::AudioChannelSet::stereo(), true);

    // 11 additional optional stereo sidechain/auxiliary buses
    for (int i = 1; i < kMaxChannels; ++i)
    {
        juce::String name (kBusNames[i]);
        props.addBus(true, name, juce::AudioChannelSet::stereo(), false);
    }

    return props;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
SpectraFlowProcessor::SpectraFlowProcessor()
    : AudioProcessor(createBusesLayout())
{
    m_paramManager = std::make_unique<ParameterManager>();
    m_apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
        *this, nullptr,
        juce::Identifier("SpectraFlow_State"),
        m_paramManager->buildParameterLayout()
    );
    m_paramManager->connectToAPVTS(*m_apvts);

    m_busManager = std::make_unique<BusManager>(kMaxChannels);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        m_fifos[ch]             = std::make_unique<FIFOManager>(kFIFOSize);
        m_fftAnalyzers[ch]      = std::make_unique<FFTAnalyzer>(kDefaultFFTSize);
        m_waveformAnalyzers[ch] = std::make_unique<WaveformAnalyzer>();

        m_analysisBuffer[0][ch].resize(512, 2048);
        m_analysisBuffer[1][ch].resize(512, 2048);
        m_guiSnapshot[ch].resize(512, 2048);
    }

    m_analysisThread = std::make_unique<AnalysisThread>(*this);
}

SpectraFlowProcessor::~SpectraFlowProcessor()
{
    if (m_analysisThread && m_analysisThread->isThreadRunning())
    {
        m_analysisThread->signalThreadShouldExit();
        m_analysisThread->waitForThreadToExit(2000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// prepareToPlay
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_prepared = false;

    m_sampleRate = sampleRate;
    m_blockSize  = samplesPerBlock;

    const int fftSize = getCurrentFFTSize();

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        m_fifos[ch]->reset(kFIFOSize);
        m_fftAnalyzers[ch]->prepare(fftSize, sampleRate);
        m_waveformAnalyzers[ch]->prepare(sampleRate, kWaveformHistory);

        const int displayBins = fftSize / 2;
        const int waveformLen = static_cast<int>(sampleRate * kWaveformHistory);

        m_analysisBuffer[0][ch].resize(displayBins, waveformLen);
        m_analysisBuffer[1][ch].resize(displayBins, waveformLen);
        m_guiSnapshot[ch].resize(displayBins, waveformLen);
    }

    if (!m_analysisThread->isThreadRunning())
        m_analysisThread->startThread(juce::Thread::Priority::low);

    m_prepared = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// releaseResources
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::releaseResources()
{
    m_prepared = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// isBusesLayoutSupported
// ─────────────────────────────────────────────────────────────────────────────
bool SpectraFlowProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Main buses must be stereo or mono
    auto mainIn  = layouts.getMainInputChannelSet();
    auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn  != juce::AudioChannelSet::stereo() &&
        mainIn  != juce::AudioChannelSet::mono())   return false;

    if (mainOut != juce::AudioChannelSet::stereo() &&
        mainOut != juce::AudioChannelSet::mono())   return false;

    // All extra buses must be stereo, mono, or disabled
    for (int bus = 1; bus < getBusCount(true); ++bus)
    {
        auto set = layouts.getChannelSet(true, bus);
        if (!set.isDisabled() &&
            set != juce::AudioChannelSet::stereo() &&
            set != juce::AudioChannelSet::mono())
            return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// processBlock  (audio thread — keep it thin!)
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (!m_prepared)
        return;

    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (!m_paramManager->isChannelEnabled(ch))
            continue;

        // Grab the audio bus for this channel
        auto* inputBus = getBus(true, ch);
        if (!inputBus || !inputBus->isEnabled())
            continue;

        const auto busBuffer = inputBus->getBusBuffer(buffer);
        const int numBusChannels = busBuffer.getNumChannels();
        if (numBusChannels == 0)
            continue;

        // Mix to mono for FIFO (L+R * 0.5)
        // Allocate temp scratch on the stack — small and bounded
        juce::HeapBlock<float> mono(numSamples);
        mono.clear(numSamples);

        for (int c = 0; c < numBusChannels; ++c)
        {
            const float* src = busBuffer.getReadPointer(c);
            for (int s = 0; s < numSamples; ++s)
                mono[s] += src[s];
        }

        const float scale = 1.f / static_cast<float>(numBusChannels);
        for (int s = 0; s < numSamples; ++s)
            mono[s] *= scale;

        // Push to lock-free FIFO — analysis thread will consume
        m_fifos[ch]->push(mono.getData(), numSamples);
    }

    // Pass-through: output = input (analyzer never modifies audio)
    // If main out != main in layout, just leave whatever is in buffer.
}

// ─────────────────────────────────────────────────────────────────────────────
// AnalysisThread::run
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::AnalysisThread::run()
{
    while (!threadShouldExit())
    {
        if (m_proc.m_prepared)
            m_proc.runAnalysisCycle();

        // ~16 ms target (≈60 Hz analysis rate)
        // Staggered batching keeps CPU per-tick manageable
        wait(4);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// runAnalysisCycle — called from analysis thread
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::runAnalysisCycle()
{
    // Staggered batch: only process channels in the current group
    const int group        = m_fftBatchGroup.fetch_add(1) % kFFTBatchGroups;
    const int groupStart   = group * kChannelsPerGroup;
    const int groupEnd     = std::min(groupStart + kChannelsPerGroup, kMaxChannels);

    for (int ch = groupStart; ch < groupEnd; ++ch)
    {
        if (!m_paramManager->isChannelEnabled(ch))
            continue;

        // Drain FIFO into analyzers
        static thread_local std::vector<float> scratch;

        const int available = m_fifos[ch]->getNumReady();
        if (available <= 0)
            continue;

        scratch.resize(available);
        m_fifos[ch]->pop(scratch.data(), available);

        // Feed waveform analyzer
        m_waveformAnalyzers[ch]->push(scratch.data(), available);

        // Feed FFT analyzer
        m_fftAnalyzers[ch]->push(scratch.data(), available);
        m_fftAnalyzers[ch]->process();
    }

    publishSnapshot();
}

// ─────────────────────────────────────────────────────────────────────────────
// publishSnapshot — write results into the write buffer, then flip
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::publishSnapshot()
{
    const int wi = m_writeIndex.load();
    auto& dst = m_analysisBuffer[wi];

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        m_fftAnalyzers[ch]->getMagnitudesDB(dst[ch].magnitudeDB);
        m_fftAnalyzers[ch]->getPeakHold(dst[ch].peakHold);
        m_waveformAnalyzers[ch]->getSnapshot(dst[ch].waveform);
        dst[ch].rmsLevel  = m_fftAnalyzers[ch]->getRMS();
        dst[ch].peakLevel = m_fftAnalyzers[ch]->getPeak();
        dst[ch].hasSignal = (dst[ch].rmsLevel > -100.f);
        dst[ch].enabled   = m_paramManager->isChannelEnabled(ch);
    }

    // Atomic pointer flip
    const int oldWrite = wi;
    const int newRead  = oldWrite;
    m_writeIndex.store(newRead ^ 1);
    m_readIndex.store(newRead);
    m_newDataReady.store(true, std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// swapAnalysisSnapshot — called from GUI timer, copies read buffer → guiSnapshot
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::swapAnalysisSnapshot()
{
    if (!m_newDataReady.load(std::memory_order_acquire))
        return;

    m_newDataReady.store(false, std::memory_order_relaxed);

    const int ri = m_readIndex.load();
    m_guiSnapshot = m_analysisBuffer[ri];
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
int SpectraFlowProcessor::getCurrentFFTSize() const noexcept
{
    const int idx = static_cast<int>(
        m_apvts->getRawParameterValue("globalFFTSize")->load());

    const int sizes[] = { 1024, 2048, 4096, 8192 };
    if (idx >= 0 && idx < 4) return sizes[idx];
    return kDefaultFFTSize;
}

int SpectraFlowProcessor::getNumActiveChannels() const noexcept
{
    int count = 0;
    for (int i = 0; i < kMaxChannels; ++i)
        if (m_paramManager->isChannelEnabled(i)) ++count;
    return count;
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* SpectraFlowProcessor::createEditor()
{
    return new SpectraFlowEditor(*this);
}

// ─────────────────────────────────────────────────────────────────────────────
// State
// ─────────────────────────────────────────────────────────────────────────────
void SpectraFlowProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_apvts->copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectraFlowProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(m_apvts->state.getType()))
        m_apvts->replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ─────────────────────────────────────────────────────────────────────────────
// Plugin entry point
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectraFlowProcessor();
}