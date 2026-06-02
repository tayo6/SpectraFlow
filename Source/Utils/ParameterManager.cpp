#include "ParameterManager.h"

using APVTS  = juce::AudioProcessorValueTreeState;
using Layout = APVTS::ParameterLayout;

// ─────────────────────────────────────────────────────────────────────────────
// buildParameterLayout
// ─────────────────────────────────────────────────────────────────────────────
Layout ParameterManager::buildParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ── Global parameters ────────────────────────────────────────────────────

    // FFT Size: 0=1024, 1=2048, 2=4096, 3=8192
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        globalID("globalFFTSize"), "FFT Size",
        juce::StringArray { "1024", "2048", "4096", "8192" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        globalID("globalSmoothing"), "Smoothing",
        juce::NormalisableRange<float>(0.f, 0.99f, 0.01f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        globalID("globalOverlap"), "FFT Overlap",
        juce::NormalisableRange<float>(0.f, 0.875f, 0.125f), 0.5f));

    // Refresh rate: 0=30fps, 1=60fps, 2=adaptive, 3=120fps
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        globalID("globalRefreshRate"), "Refresh Rate",
        juce::StringArray { "30 FPS", "60 FPS", "Adaptive", "120 FPS" }, 1));

    // Frequency scale: 0=Log, 1=Linear, 2=Mel, 3=Bark
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        globalID("globalFreqScale"), "Freq Scale",
        juce::StringArray { "Logarithmic", "Linear", "Mel", "Bark" }, 0));

    // Window type: 0=Hann, 1=Hamming, 2=Blackman, 3=Kaiser, 4=FlatTop
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        globalID("globalWindowType"), "Window",
        juce::StringArray { "Hann", "Hamming", "Blackman", "Kaiser", "Flat Top" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        globalID("globalOpacity"), "Global Opacity",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        globalID("globalDynamicRange"), "Dynamic Range (dB)",
        juce::NormalisableRange<float>(60.f, 144.f, 6.f), 90.f));

    // ── Per-channel parameters ───────────────────────────────────────────────
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        const juce::String chStr = juce::String(ch);

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            channelID(ch, "enabled"), "Ch" + chStr + " Enable",
            ch == 0));   // only channel 0 on by default

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            channelID(ch, "gain"), "Ch" + chStr + " Gain (dB)",
            juce::NormalisableRange<float>(-24.f, 24.f, 0.1f), 0.f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            channelID(ch, "smoothing"), "Ch" + chStr + " Smoothing",
            juce::NormalisableRange<float>(0.f, 0.99f, 0.01f), 0.75f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            channelID(ch, "opacity"), "Ch" + chStr + " Opacity",
            juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            channelID(ch, "solo"), "Ch" + chStr + " Solo", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            channelID(ch, "mute"), "Ch" + chStr + " Mute", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            channelID(ch, "peakHold"), "Ch" + chStr + " Peak Hold (s)",
            juce::NormalisableRange<float>(0.f, 10.f, 0.1f), 2.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            channelID(ch, "waveformOn"), "Ch" + chStr + " Waveform", false));
    }

    return { params.begin(), params.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
// connectToAPVTS
// ─────────────────────────────────────────────────────────────────────────────
void ParameterManager::connectToAPVTS(APVTS& apvts)
{
    auto get = [&](const juce::String& id) -> std::atomic<float>*
    {
        auto* p = apvts.getRawParameterValue(id);
        jassert(p != nullptr);
        return p;
    };

    p_fftSizeIdx    = get(globalID("globalFFTSize"));
    p_smoothing     = get(globalID("globalSmoothing"));
    p_overlap       = get(globalID("globalOverlap"));
    p_refreshRate   = get(globalID("globalRefreshRate"));
    p_freqScale     = get(globalID("globalFreqScale"));
    p_windowType    = get(globalID("globalWindowType"));
    p_globalOpacity = get(globalID("globalOpacity"));
    p_dynamicRange  = get(globalID("globalDynamicRange"));

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        m_chPtrs[ch].enabled    = get(channelID(ch, "enabled"));
        m_chPtrs[ch].gain       = get(channelID(ch, "gain"));
        m_chPtrs[ch].smoothing  = get(channelID(ch, "smoothing"));
        m_chPtrs[ch].opacity    = get(channelID(ch, "opacity"));
        m_chPtrs[ch].solo       = get(channelID(ch, "solo"));
        m_chPtrs[ch].mute       = get(channelID(ch, "mute"));
        m_chPtrs[ch].peakHold   = get(channelID(ch, "peakHold"));
        m_chPtrs[ch].waveformOn = get(channelID(ch, "waveformOn"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Global accessors
// ─────────────────────────────────────────────────────────────────────────────
int   ParameterManager::getFFTSizeIndex()     const noexcept { return p_fftSizeIdx    ? static_cast<int>(p_fftSizeIdx->load())    : 1; }
float ParameterManager::getSmoothing()        const noexcept { return p_smoothing     ? p_smoothing->load()     : 0.8f; }
float ParameterManager::getOverlap()          const noexcept { return p_overlap       ? p_overlap->load()       : 0.5f; }
int   ParameterManager::getRefreshRateIndex() const noexcept { return p_refreshRate   ? static_cast<int>(p_refreshRate->load())   : 1; }
int   ParameterManager::getFreqScaleIndex()   const noexcept { return p_freqScale     ? static_cast<int>(p_freqScale->load())     : 0; }
int   ParameterManager::getWindowTypeIndex()  const noexcept { return p_windowType    ? static_cast<int>(p_windowType->load())    : 0; }
float ParameterManager::getGlobalOpacity()    const noexcept { return p_globalOpacity ? p_globalOpacity->load() : 1.0f; }
float ParameterManager::getDynamicRange()     const noexcept { return p_dynamicRange  ? p_dynamicRange->load()  : 90.f; }

// ─────────────────────────────────────────────────────────────────────────────
// Per-channel accessors
// ─────────────────────────────────────────────────────────────────────────────
bool  ParameterManager::isChannelEnabled(int ch)    const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].enabled    ? m_chPtrs[ch].enabled->load()    > 0.5f : false; }
float ParameterManager::getChannelGain(int ch)      const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].gain       ? m_chPtrs[ch].gain->load()       : 0.f; }
float ParameterManager::getChannelSmoothing(int ch) const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].smoothing  ? m_chPtrs[ch].smoothing->load()  : 0.75f; }
float ParameterManager::getChannelOpacity(int ch)   const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].opacity    ? m_chPtrs[ch].opacity->load()    : 1.f; }
bool  ParameterManager::isChannelSoloed(int ch)     const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].solo       ? m_chPtrs[ch].solo->load()       > 0.5f : false; }
bool  ParameterManager::isChannelMuted(int ch)      const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].mute       ? m_chPtrs[ch].mute->load()       > 0.5f : false; }
float ParameterManager::getPeakHoldTime(int ch)     const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].peakHold   ? m_chPtrs[ch].peakHold->load()   : 2.f; }
bool  ParameterManager::isWaveformEnabled(int ch)   const noexcept { return ch >= 0 && ch < kNumChannels && m_chPtrs[ch].waveformOn ? m_chPtrs[ch].waveformOn->load() > 0.5f : false; }
