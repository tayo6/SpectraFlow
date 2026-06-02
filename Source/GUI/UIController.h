#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "ColorManager.h"
#include "SpectrumRenderer.h"
#include "WaveformRenderer.h"

class SpectraFlowProcessor;

// ─────────────────────────────────────────────────────────────────────────────
// ChannelStrip — compact per-channel enable/colour/gain controls
// ─────────────────────────────────────────────────────────────────────────────
class ChannelStrip : public juce::Component
{
public:
    explicit ChannelStrip(int channelIndex, juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getChannelIndex() const noexcept { return m_channelIdx; }
    void setChannelColour(juce::Colour c);

private:
    int   m_channelIdx;
    juce::Colour m_colour;

    juce::ToggleButton   m_enableButton;
    juce::Slider         m_gainSlider;
    juce::Label          m_nameLabel;
    juce::TextButton     m_muteButton;
    juce::TextButton     m_soloButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  m_enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  m_gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  m_muteAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  m_soloAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

// ─────────────────────────────────────────────────────────────────────────────
// GlobalControlBar — FFT size, window, scale, FPS controls
// ─────────────────────────────────────────────────────────────────────────────
class GlobalControlBar : public juce::Component
{
public:
    explicit GlobalControlBar(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::ComboBox m_fftSizeBox;
    juce::ComboBox m_windowBox;
    juce::ComboBox m_freqScaleBox;
    juce::ComboBox m_refreshRateBox;
    juce::Slider   m_smoothingSlider;
    juce::Slider   m_dynamicRangeSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_fftSizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_windowAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_freqScaleAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_refreshRateAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   m_smoothingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   m_dynRangeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalControlBar)
};

// ─────────────────────────────────────────────────────────────────────────────
// UIController — top-level UI layout and timer-driven repaint orchestration
// ─────────────────────────────────────────────────────────────────────────────
class UIController : public juce::Component,
                     private juce::Timer
{
public:
    explicit UIController(SpectraFlowProcessor& processor);
    ~UIController() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    ColorManager& getColorManager() noexcept { return m_colorManager; }

private:
    SpectraFlowProcessor& m_processor;

    // ── Sub-components ───────────────────────────────────────────────────────
    std::unique_ptr<GlobalControlBar>                    m_globalBar;
    std::array<std::unique_ptr<ChannelStrip>, 12>        m_channelStrips;

    // ── Rendering ────────────────────────────────────────────────────────────
    ColorManager      m_colorManager;
    SpectrumRenderer  m_spectrumRenderer;
    WaveformRenderer  m_waveformRenderer;

    // ── Layout rects (computed in resized) ───────────────────────────────────
    juce::Rectangle<int>  m_spectrumArea;
    juce::Rectangle<int>  m_waveformArea;
    juce::Rectangle<int>  m_channelStripArea;
    juce::Rectangle<int>  m_globalBarArea;

    // ── Grid overlay ─────────────────────────────────────────────────────────
    void paintGrid(juce::Graphics& g);
    void paintFrequencyLabels(juce::Graphics& g);
    void paintLevelLabels(juce::Graphics& g);
    void paintChannelLegend(juce::Graphics& g);
    void paintBackground(juce::Graphics& g);

    // ── Timer ─────────────────────────────────────────────────────────────────
    void timerCallback() override;
    int  getTargetFPS() const noexcept;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void syncColorManagerFromParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIController)
};
