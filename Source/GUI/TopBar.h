#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

// ─────────────────────────────────────────────────────────────────────────────
// TopBar — thin header bar (Pro-Q4 style)
//
// Contains:
//   • Logo / plugin name
//   • Spectrum / Waveform toggle
//   • FFT size selector
//   • Window type selector
//   • CPU readout
// ─────────────────────────────────────────────────────────────────────────────
class TopBar : public juce::Component,
               public juce::Button::Listener,
               public juce::ComboBox::Listener
{
public:
    std::function<void(bool)> onViewToggle;   // true = spectrum, false = waveform

    explicit TopBar(juce::AudioProcessorValueTreeState& apvts);
    ~TopBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setCPU(float percent);

private:
    void buttonClicked(juce::Button* b) override;
    void comboBoxChanged(juce::ComboBox* c) override;

    juce::TextButton m_specBtn  { "SPECTRUM" };
    juce::TextButton m_waveBtn  { "WAVEFORM" };
    juce::ComboBox   m_fftBox;
    juce::ComboBox   m_windowBox;
    juce::Label      m_cpuLabel;
    juce::Label      m_logoLabel;

    bool m_specActive = true;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_fftAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_winAttach;

    static juce::Font monoFont(float sz = 10.f);
    static juce::Font uiFont  (float sz = 11.f, bool bold = false);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};
