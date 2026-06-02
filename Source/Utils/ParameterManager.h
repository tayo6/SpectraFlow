#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
// ParameterManager
//
// Owns the APVTS parameter layout and provides typed accessors.
// Works with JUCE 7 AudioProcessorValueTreeState.
// ─────────────────────────────────────────────────────────────────────────────
class ParameterManager
{
public:
    static constexpr int kNumChannels = 12;

    ParameterManager() = default;
    ~ParameterManager() = default;

    // Build the APVTS layout — call once before constructing APVTS
    juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout();

    // Wire up raw parameter pointers after APVTS is constructed
    void connectToAPVTS(juce::AudioProcessorValueTreeState& apvts);

    // ── Global accessors ─────────────────────────────────────────────────────
    int   getFFTSizeIndex()    const noexcept;
    float getSmoothing()       const noexcept;
    float getOverlap()         const noexcept;
    int   getRefreshRateIndex()const noexcept;
    int   getFreqScaleIndex()  const noexcept;
    int   getWindowTypeIndex() const noexcept;
    float getGlobalOpacity()   const noexcept;
    float getDynamicRange()    const noexcept;

    // ── Per-channel accessors ────────────────────────────────────────────────
    bool  isChannelEnabled(int ch)    const noexcept;
    float getChannelGain(int ch)      const noexcept;
    float getChannelSmoothing(int ch) const noexcept;
    float getChannelOpacity(int ch)   const noexcept;
    bool  isChannelSoloed(int ch)     const noexcept;
    bool  isChannelMuted(int ch)      const noexcept;
    float getPeakHoldTime(int ch)     const noexcept;
    bool  isWaveformEnabled(int ch)   const noexcept;

    // ── Parameter ID helpers (static so GUI can use them too) ───────────────
    static juce::String globalID(const char* name)  { return juce::String(name); }
    static juce::String channelID(int ch, const char* name)
    {
        return juce::String("ch") + juce::String(ch) + "_" + juce::String(name);
    }

private:
    // Raw atomic float pointers — zero-cost access, no map lookup
    // Global
    std::atomic<float>* p_fftSizeIdx    = nullptr;
    std::atomic<float>* p_smoothing     = nullptr;
    std::atomic<float>* p_overlap       = nullptr;
    std::atomic<float>* p_refreshRate   = nullptr;
    std::atomic<float>* p_freqScale     = nullptr;
    std::atomic<float>* p_windowType    = nullptr;
    std::atomic<float>* p_globalOpacity = nullptr;
    std::atomic<float>* p_dynamicRange  = nullptr;

    // Per channel
    struct ChannelPtrs
    {
        std::atomic<float>* enabled      = nullptr;
        std::atomic<float>* gain         = nullptr;
        std::atomic<float>* smoothing    = nullptr;
        std::atomic<float>* opacity      = nullptr;
        std::atomic<float>* solo         = nullptr;
        std::atomic<float>* mute         = nullptr;
        std::atomic<float>* peakHold     = nullptr;
        std::atomic<float>* waveformOn   = nullptr;
    };
    std::array<ChannelPtrs, kNumChannels> m_chPtrs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterManager)
};
