#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/BusManager.h"
#include "DSP/FFTAnalyzer.h"
#include "DSP/WaveformAnalyzer.h"
#include "DSP/FIFOManager.h"
#include "Utils/ParameterManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────
namespace SpectraFlow
{
    static constexpr int kMaxChannels       = 12;
    static constexpr int kMaxFFTSize        = 8192;
    static constexpr int kDefaultFFTSize    = 2048;
    static constexpr int kWaveformHistory   = 5;   // seconds
    static constexpr int kFIFOSize          = 65536;

    // Channel names map to standard mixing categories
    static const char* kBusNames[kMaxChannels] = {
        "Bass", "Lead Vocal", "Backup Vocal", "Drums",
        "Synth", "FX", "Percussion", "Pads",
        "Master", "Reference", "Sidechain", "Utility"
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// AnalysisData — lock-free snapshot passed from analysis thread → GUI thread
// ─────────────────────────────────────────────────────────────────────────────
struct ChannelAnalysisData
{
    std::vector<float> magnitudeDB;        // FFT magnitude in dB, log-spaced
    std::vector<float> peakHold;           // Per-bin peak hold
    std::vector<float> waveform;           // Waveform ring-buffer snapshot
    float              rmsLevel   = -120.f;
    float              peakLevel  = -120.f;
    bool               hasSignal  = false;
    bool               enabled    = true;

    void resize(int fftDisplayBins, int waveformSamples)
    {
        magnitudeDB.assign(fftDisplayBins, -120.f);
        peakHold.assign(fftDisplayBins, -120.f);
        waveform.assign(waveformSamples, 0.f);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// PluginProcessor
// ─────────────────────────────────────────────────────────────────────────────
class SpectraFlowProcessor : public juce::AudioProcessor
{
public:
    SpectraFlowProcessor();
    ~SpectraFlowProcessor() override;

    // ── AudioProcessor overrides ─────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpectraFlow"; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── Public API for GUI ───────────────────────────────────────────────────
    // GUI thread calls this to grab the latest rendered snapshot
    const std::array<ChannelAnalysisData, SpectraFlow::kMaxChannels>&
        getLatestAnalysisData() const noexcept { return m_guiSnapshot; }

    // Flip the double-buffer from the GUI timer tick
    void swapAnalysisSnapshot();

    ParameterManager& getParameterManager() noexcept { return *m_paramManager; }

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return *m_apvts; }

    int  getCurrentFFTSize()    const noexcept;
    int  getNumActiveChannels() const noexcept;

private:
    // ── Subsystems ───────────────────────────────────────────────────────────
    std::unique_ptr<ParameterManager>  m_paramManager;
    std::unique_ptr<juce::AudioProcessorValueTreeState> m_apvts;

    std::array<std::unique_ptr<FIFOManager>,      SpectraFlow::kMaxChannels> m_fifos;
    std::array<std::unique_ptr<FFTAnalyzer>,       SpectraFlow::kMaxChannels> m_fftAnalyzers;
    std::array<std::unique_ptr<WaveformAnalyzer>,  SpectraFlow::kMaxChannels> m_waveformAnalyzers;
    std::unique_ptr<BusManager>                    m_busManager;

    // ── Analysis thread ──────────────────────────────────────────────────────
    class AnalysisThread : public juce::Thread
    {
    public:
        explicit AnalysisThread(SpectraFlowProcessor& p)
            : juce::Thread("SpectraFlow Analysis"), m_proc(p) {}
        void run() override;
    private:
        SpectraFlowProcessor& m_proc;
    };
    std::unique_ptr<AnalysisThread> m_analysisThread;

    // ── Double-buffered GUI snapshots ─────────────────────────────────────────
    // Index 0 = "write" buffer (analysis thread writes)
    // Index 1 = "read"  buffer (GUI thread reads)
    std::array<ChannelAnalysisData, SpectraFlow::kMaxChannels> m_analysisBuffer[2];
    std::atomic<int>  m_readIndex  { 1 };
    std::atomic<int>  m_writeIndex { 0 };
    std::atomic<bool> m_newDataReady { false };

    // GUI-visible snapshot — only touched by GUI thread after swap
    std::array<ChannelAnalysisData, SpectraFlow::kMaxChannels> m_guiSnapshot;

    // ── State ────────────────────────────────────────────────────────────────
    double              m_sampleRate     = 44100.0;
    int                 m_blockSize      = 512;
    std::atomic<bool>   m_prepared       { false };

    // Staggered FFT scheduling (batch across frames)
    std::atomic<int>    m_fftBatchGroup  { 0 };   // 0..3, rotate each analysis tick
    static constexpr int kFFTBatchGroups = 4;
    static constexpr int kChannelsPerGroup = (SpectraFlow::kMaxChannels + kFFTBatchGroups - 1)
                                              / kFFTBatchGroups;

    void runAnalysisCycle();
    void publishSnapshot();

    static BusesProperties createBusesLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectraFlowProcessor)
};
