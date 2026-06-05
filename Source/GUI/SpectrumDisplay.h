#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

struct ChannelAnalysisData;

// ─────────────────────────────────────────────────────────────────────────────
// SpectrumDisplay
//
// The main visualiser component — occupies ~70% width of the plugin window.
// Renders a Pro-Q4-style spectrum: per-channel filled gradient + vivid stroke.
// Also renders waveform mode (oscilloscope).
// ─────────────────────────────────────────────────────────────────────────────
class SpectrumDisplay : public juce::Component
{
public:
    enum class ViewMode { Spectrum, Waveform };

    SpectrumDisplay();
    ~SpectrumDisplay() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Called from editor timer — hand in latest snapshot
    void pushData(const std::array<ChannelAnalysisData, 12>& data);

    void setViewMode(ViewMode m)       { m_mode = m; repaint(); }
    void setDynamicRange(float dB)     { m_dynRange = dB; }
    void setSampleRate(double sr)      { m_sampleRate = sr; }

    // Per-channel colour (set by InsertPanel colour picker)
    void setChannelColour(int ch, juce::Colour c);
    void setChannelEnabled(int ch, bool e);

private:
    // ── drawing helpers ──────────────────────────────────────────────────────
    void drawBackground(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawFreqLabels(juce::Graphics& g);
    void drawDBLabels(juce::Graphics& g);
    void drawSpectrumChannel(juce::Graphics& g, int ch, juce::Rectangle<float> area);
    void drawWaveformChannel(juce::Graphics& g, int ch, juce::Rectangle<float> area);

    // ── coordinate helpers ───────────────────────────────────────────────────
    float freqToX(float freq, float areaW) const noexcept;
    float dbToY(float dB, float areaH)     const noexcept;

    // ── state ────────────────────────────────────────────────────────────────
    ViewMode   m_mode      = ViewMode::Spectrum;
    float      m_dynRange  = 90.f;
    double     m_sampleRate = 44100.0;

    static constexpr int kCh = 12;
    std::array<juce::Colour, kCh> m_colours;
    std::array<bool,          kCh> m_enabled;

    // Local copy of analysis data (only updated on GUI thread via pushData)
    std::array<std::vector<float>, kCh> m_magnitudes;
    std::array<std::vector<float>, kCh> m_peakHold;
    std::array<std::vector<float>, kCh> m_waveform;
    std::array<float, kCh>              m_rmsLevel;

    // Reusable path object
    juce::Path m_path;

    // Layout constants
    static constexpr float kLeftMargin   = 38.f;
    static constexpr float kBottomMargin = 20.f;
    static constexpr float kMinFreq      = 20.f;
    static constexpr float kMaxFreq      = 22000.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
