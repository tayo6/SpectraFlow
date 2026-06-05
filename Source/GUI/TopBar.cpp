#include "TopBar.h"
#include "../Utils/ParameterManager.h"

namespace {
    const juce::Colour kBg      { 0xFF0E0F16 };
    const juce::Colour kBorder  { 0xFF1E2130 };
    const juce::Colour kText    { 0xFFDDE0EC };
    const juce::Colour kMuted   { 0xFF525870 };
    const juce::Colour kAccent  { 0xFF4AF0C4 };
}

juce::Font TopBar::monoFont(float sz)
{ return juce::Font(juce::Font::getDefaultMonospacedFontName(), sz, juce::Font::plain); }

juce::Font TopBar::uiFont(float sz, bool bold)
{ return juce::Font(sz, bold ? juce::Font::bold : juce::Font::plain); }

TopBar::TopBar(juce::AudioProcessorValueTreeState& apvts)
{
    // Logo
    m_logoLabel.setText("SpectraFlow", juce::dontSendNotification);
    m_logoLabel.setFont(uiFont(11.f, true));
    m_logoLabel.setColour(juce::Label::textColourId, kAccent);
    m_logoLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_logoLabel);

    // View toggle buttons
    auto styleToggle = [&](juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? kAccent.withAlpha(0.18f)
                             : juce::Colour(0xFF181B26));
        btn.setColour(juce::TextButton::buttonOnColourId,  kAccent.withAlpha(0.18f));
        btn.setColour(juce::TextButton::textColourOffId,   active ? kAccent : kMuted);
        btn.setColour(juce::TextButton::textColourOnId,    kAccent);
        btn.setFont(monoFont(9.f));
        btn.addListener(this);
        addAndMakeVisible(btn);
    };
    styleToggle(m_specBtn, true);
    styleToggle(m_waveBtn, false);

    // FFT size combo
    m_fftBox.addItem("1024",  1);
    m_fftBox.addItem("2048",  2);
    m_fftBox.addItem("4096",  3);
    m_fftBox.addItem("8192",  4);
    m_fftBox.setSelectedId(2, juce::dontSendNotification);
    m_fftBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF181B26));
    m_fftBox.setColour(juce::ComboBox::textColourId,       kText);
    m_fftBox.setColour(juce::ComboBox::outlineColourId,    kBorder);
    m_fftBox.setColour(juce::ComboBox::arrowColourId,      kMuted);
    m_fftBox.addListener(this);
    addAndMakeVisible(m_fftBox);

    // Window combo
    m_windowBox.addItem("Hann",     1);
    m_windowBox.addItem("Hamming",  2);
    m_windowBox.addItem("Blackman", 3);
    m_windowBox.addItem("Kaiser",   4);
    m_windowBox.addItem("Flat Top", 5);
    m_windowBox.setSelectedId(1, juce::dontSendNotification);
    m_windowBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF181B26));
    m_windowBox.setColour(juce::ComboBox::textColourId,       kText);
    m_windowBox.setColour(juce::ComboBox::outlineColourId,    kBorder);
    m_windowBox.setColour(juce::ComboBox::arrowColourId,      kMuted);
    m_windowBox.addListener(this);
    addAndMakeVisible(m_windowBox);

    // CPU label
    m_cpuLabel.setText("CPU: 0.0%", juce::dontSendNotification);
    m_cpuLabel.setFont(monoFont(9.f));
    m_cpuLabel.setColour(juce::Label::textColourId, kMuted);
    m_cpuLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(m_cpuLabel);

    // APVTS attachments
    m_fftAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalFFTSize"), m_fftBox);
    m_winAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalWindowType"), m_windowBox);
}

TopBar::~TopBar()
{
    m_specBtn.removeListener(this);
    m_waveBtn.removeListener(this);
    m_fftBox.removeListener(this);
    m_windowBox.removeListener(this);
}

void TopBar::paint(juce::Graphics& g)
{
    g.setColour(kBg);
    g.fillRect(getLocalBounds());

    g.setColour(kBorder);
    g.drawHorizontalLine(getHeight() - 1, 0.f, static_cast<float>(getWidth()));

    // Section labels above combos
    g.setFont(monoFont(8.f));
    g.setColour(kMuted);

    auto drawLabel = [&](const juce::String& text, juce::Rectangle<int> above)
    {
        g.drawText(text, above.withY(2).withHeight(10),
                   juce::Justification::centred);
    };

    // Draw labels above FFT and Window combos (done in resized so we use stored bounds)
}

void TopBar::resized()
{
    auto area = getLocalBounds().reduced(10, 0);

    m_logoLabel.setBounds(area.removeFromLeft(100).reduced(0, 8));
    area.removeFromLeft(8);

    // Toggle group
    m_specBtn.setBounds(area.removeFromLeft(76).reduced(0, 7));
    m_waveBtn.setBounds(area.removeFromLeft(76).reduced(0, 7));
    area.removeFromLeft(12);

    // FFT size
    area.removeFromLeft(2);
    m_fftBox.setBounds(area.removeFromLeft(68).reduced(0, 8));
    area.removeFromLeft(8);

    // Window
    m_windowBox.setBounds(area.removeFromLeft(80).reduced(0, 8));

    // CPU on far right
    m_cpuLabel.setBounds(area.removeFromRight(80).reduced(0, 8));
}

void TopBar::buttonClicked(juce::Button* b)
{
    if (b == &m_specBtn)
    {
        m_specActive = true;
        m_specBtn.setColour(juce::TextButton::buttonColourId,  kAccent.withAlpha(0.18f));
        m_specBtn.setColour(juce::TextButton::textColourOffId, kAccent);
        m_waveBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF181B26));
        m_waveBtn.setColour(juce::TextButton::textColourOffId, kMuted);
        if (onViewToggle) onViewToggle(true);
    }
    else if (b == &m_waveBtn)
    {
        m_specActive = false;
        m_waveBtn.setColour(juce::TextButton::buttonColourId,  kAccent.withAlpha(0.18f));
        m_waveBtn.setColour(juce::TextButton::textColourOffId, kAccent);
        m_specBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF181B26));
        m_specBtn.setColour(juce::TextButton::textColourOffId, kMuted);
        if (onViewToggle) onViewToggle(false);
    }
}

void TopBar::comboBoxChanged(juce::ComboBox*) {}

void TopBar::setCPU(float pct)
{
    m_cpuLabel.setText("CPU  " + juce::String(pct, 1) + "%",
                       juce::dontSendNotification);
    m_cpuLabel.setColour(juce::Label::textColourId,
                         pct > 60.f ? juce::Colour(0xFFFF6B6B) : kMuted);
}
