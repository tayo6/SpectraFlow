#include "UIController.h"
#include "../PluginProcessor.h"
#include <cmath>

using namespace SpectraFlow;

// ═════════════════════════════════════════════════════════════════════════════
// ChannelStrip
// ═════════════════════════════════════════════════════════════════════════════

ChannelStrip::ChannelStrip(int channelIndex,
                            juce::AudioProcessorValueTreeState& apvts)
    : m_channelIdx(channelIndex),
      m_colour(juce::Colours::cyan)
{
    const juce::String chStr(channelIndex);

    // Name label
    m_nameLabel.setText(kBusNames[channelIndex], juce::dontSendNotification);
    m_nameLabel.setFont(juce::Font(10.f, juce::Font::bold));
    m_nameLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    m_nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_nameLabel);

    // Enable toggle
    m_enableButton.setButtonText("ON");
    m_enableButton.setToggleState(channelIndex == 0, juce::dontSendNotification);
    addAndMakeVisible(m_enableButton);

    // Mute button
    m_muteButton.setButtonText("M");
    addAndMakeVisible(m_muteButton);

    // Solo button
    m_soloButton.setButtonText("S");
    addAndMakeVisible(m_soloButton);

    // Gain slider — vertical
    m_gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_gainSlider.setRange(-24.0, 24.0, 0.1);
    addAndMakeVisible(m_gainSlider);

    // APVTS attachments
    m_enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParameterManager::channelID(channelIndex, "enabled"), m_enableButton);

    m_gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParameterManager::channelID(channelIndex, "gain"), m_gainSlider);

    m_muteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParameterManager::channelID(channelIndex, "mute"), m_muteButton);

    m_soloAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParameterManager::channelID(channelIndex, "solo"), m_soloButton);
}

void ChannelStrip::setChannelColour(juce::Colour c)
{
    m_colour = c;
    m_nameLabel.setColour(juce::Label::textColourId, c);
    repaint();
}

void ChannelStrip::paint(juce::Graphics& g)
{
    // Background
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), 3.f);

    // Colour accent bar at top
    g.setColour(m_colour);
    g.fillRect(getLocalBounds().withHeight(3));
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced(2);
    m_nameLabel  .setBounds(area.removeFromTop(14));
    m_enableButton.setBounds(area.removeFromTop(16));

    auto btnRow = area.removeFromTop(16);
    m_muteButton.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(1));
    m_soloButton.setBounds(btnRow.reduced(1));

    m_gainSlider.setBounds(area.reduced(2));
}

// ═════════════════════════════════════════════════════════════════════════════
// GlobalControlBar
// ═════════════════════════════════════════════════════════════════════════════

GlobalControlBar::GlobalControlBar(juce::AudioProcessorValueTreeState& apvts)
{
    auto setupCombo = [&](juce::ComboBox& box, const juce::StringArray& items)
    {
        for (int i = 0; i < items.size(); ++i)
            box.addItem(items[i], i + 1);
        box.setSelectedItemIndex(0, juce::dontSendNotification);
        addAndMakeVisible(box);
    };

    setupCombo(m_fftSizeBox,    { "1024", "2048", "4096", "8192" });
    setupCombo(m_windowBox,     { "Hann", "Hamming", "Blackman", "Kaiser", "Flat Top" });
    setupCombo(m_freqScaleBox,  { "Log", "Linear", "Mel", "Bark" });
    setupCombo(m_refreshRateBox,{ "30 FPS", "60 FPS", "Adaptive", "120 FPS" });

    m_smoothingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_smoothingSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_smoothingSlider.setRange(0.0, 0.99, 0.01);
    addAndMakeVisible(m_smoothingSlider);

    m_dynamicRangeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_dynamicRangeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_dynamicRangeSlider.setRange(60.0, 144.0, 6.0);
    addAndMakeVisible(m_dynamicRangeSlider);

    m_fftSizeAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalFFTSize"), m_fftSizeBox);
    m_windowAttach     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalWindowType"), m_windowBox);
    m_freqScaleAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalFreqScale"), m_freqScaleBox);
    m_refreshRateAttach= std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParameterManager::globalID("globalRefreshRate"), m_refreshRateBox);
    m_smoothingAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParameterManager::globalID("globalSmoothing"), m_smoothingSlider);
    m_dynRangeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParameterManager::globalID("globalDynamicRange"), m_dynamicRangeSlider);
}

void GlobalControlBar::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRect(getLocalBounds());

    // Section labels
    const juce::Font labelFont(10.f);
    g.setFont(labelFont);
    g.setColour(juce::Colours::white.withAlpha(0.5f));

    auto b = getLocalBounds().reduced(4, 2);
    const int colW = b.getWidth() / 6;
    g.drawText("FFT",    b.getX() + 0 * colW, b.getY(), colW, 12, juce::Justification::centred);
    g.drawText("Window", b.getX() + 1 * colW, b.getY(), colW, 12, juce::Justification::centred);
    g.drawText("Scale",  b.getX() + 2 * colW, b.getY(), colW, 12, juce::Justification::centred);
    g.drawText("FPS",    b.getX() + 3 * colW, b.getY(), colW, 12, juce::Justification::centred);
    g.drawText("Smooth", b.getX() + 4 * colW, b.getY(), colW, 12, juce::Justification::centred);
    g.drawText("Range",  b.getX() + 5 * colW, b.getY(), colW, 12, juce::Justification::centred);
}

void GlobalControlBar::resized()
{
    auto b = getLocalBounds().reduced(4, 0);
    b.removeFromTop(14);   // label row

    const int colW = b.getWidth() / 6;
    m_fftSizeBox       .setBounds(b.removeFromLeft(colW).reduced(2, 1));
    m_windowBox        .setBounds(b.removeFromLeft(colW).reduced(2, 1));
    m_freqScaleBox     .setBounds(b.removeFromLeft(colW).reduced(2, 1));
    m_refreshRateBox   .setBounds(b.removeFromLeft(colW).reduced(2, 1));
    m_smoothingSlider  .setBounds(b.removeFromLeft(colW).reduced(2, 1));
    m_dynamicRangeSlider.setBounds(b.reduced(2, 1));
}

// ═════════════════════════════════════════════════════════════════════════════
// UIController
// ═════════════════════════════════════════════════════════════════════════════

UIController::UIController(SpectraFlowProcessor& processor)
    : m_processor(processor)
{
    setSize(1200, 700);

    // Global control bar
    m_globalBar = std::make_unique<GlobalControlBar>(processor.getAPVTS());
    addAndMakeVisible(*m_globalBar);

    // Channel strips
    for (int i = 0; i < kMaxChannels; ++i)
    {
        m_channelStrips[i] = std::make_unique<ChannelStrip>(i, processor.getAPVTS());
        addAndMakeVisible(*m_channelStrips[i]);
    }

    m_colorManager.resetToDefaults();

    // Apply default colours to strips
    for (int i = 0; i < kMaxChannels; ++i)
        m_channelStrips[i]->setChannelColour(m_colorManager.getPrimaryColour(i));

    startTimerHz(getTargetFPS());
}

UIController::~UIController()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
// resized
// ─────────────────────────────────────────────────────────────────────────────
void UIController::resized()
{
    auto area = getLocalBounds();

    // Global controls at top
    m_globalBarArea = area.removeFromTop(56);
    m_globalBar->setBounds(m_globalBarArea);

    // Channel strips at bottom
    const int stripHeight = 110;
    m_channelStripArea = area.removeFromBottom(stripHeight);

    const int stripW = m_channelStripArea.getWidth() / kMaxChannels;
    for (int i = 0; i < kMaxChannels; ++i)
    {
        m_channelStrips[i]->setBounds(
            m_channelStripArea.getX() + i * stripW,
            m_channelStripArea.getY(),
            stripW,
            stripHeight);
    }

    // Remaining area: spectrum on top, waveform on bottom
    const int waveformH = std::max(60, static_cast<int>(area.getHeight() * 0.2f));
    m_waveformArea  = area.removeFromBottom(waveformH);
    m_spectrumArea  = area;

    m_spectrumRenderer.setBounds(m_spectrumArea);
    m_waveformRenderer.setBounds(m_waveformArea);
}

// ─────────────────────────────────────────────────────────────────────────────
// timerCallback — fetch latest data then trigger a repaint
// ─────────────────────────────────────────────────────────────────────────────
void UIController::timerCallback()
{
    m_processor.swapAnalysisSnapshot();

    // Sync dynamic range and scale params → renderer
    auto& params = m_processor.getParameterManager();
    m_spectrumRenderer.setDynamicRange(params.getDynamicRange());
    m_spectrumRenderer.setFrequencyScale(params.getFreqScaleIndex());

    // Adaptive FPS: if most channels are silent, slow the timer
    const int targetFPS = getTargetFPS();
    if (std::abs(getTimerInterval() - 1000 / targetFPS) > 5)
        startTimerHz(targetFPS);

    repaint(m_spectrumArea);
    repaint(m_waveformArea);
}

// ─────────────────────────────────────────────────────────────────────────────
// paint
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paint(juce::Graphics& g)
{
    paintBackground(g);
    paintGrid(g);

    const auto& data = m_processor.getLatestAnalysisData();

    // Spectrum
    g.saveState();
    g.reduceClipRegion(m_spectrumArea);
    m_spectrumRenderer.draw(g, data, m_colorManager, m_spectrumArea);
    g.restoreState();

    // Waveform
    g.saveState();
    g.reduceClipRegion(m_waveformArea);
    m_waveformRenderer.draw(g, data, m_colorManager);
    g.restoreState();

    paintFrequencyLabels(g);
    paintLevelLabels(g);
    paintChannelLegend(g);
}

// ─────────────────────────────────────────────────────────────────────────────
// paintBackground
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paintBackground(juce::Graphics& g)
{
    // Dark gradient background
    juce::ColourGradient bg(
        juce::Colour(0xFF0A0A14), 0.f, 0.f,
        juce::Colour(0xFF050510), 0.f, static_cast<float>(getHeight()),
        false);
    g.setGradientFill(bg);
    g.fillRect(getLocalBounds());

    // Spectrum area slightly lighter
    g.setColour(juce::Colour(0xFF0E0E1E));
    g.fillRect(m_spectrumArea);
}

// ─────────────────────────────────────────────────────────────────────────────
// paintGrid
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paintGrid(juce::Graphics& g)
{
    if (m_spectrumArea.isEmpty()) return;

    g.setColour(juce::Colours::white.withAlpha(0.05f));

    // Horizontal dB lines
    const auto& params = m_processor.getParameterManager();
    const float dynRange = params.getDynamicRange();
    const float gridStep = 12.f;
    const int   numLines = static_cast<int>(dynRange / gridStep);

    for (int i = 0; i <= numLines; ++i)
    {
        const float norm = static_cast<float>(i) / static_cast<float>(numLines);
        const int y = m_spectrumArea.getY()
                    + static_cast<int>(norm * m_spectrumArea.getHeight());
        g.drawHorizontalLine(y,
            static_cast<float>(m_spectrumArea.getX()),
            static_cast<float>(m_spectrumArea.getRight()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paintFrequencyLabels
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paintFrequencyLabels(juce::Graphics& g)
{
    if (m_spectrumArea.isEmpty()) return;

    const float freqMarkers[] = { 20.f, 50.f, 100.f, 200.f, 500.f,
                                   1000.f, 2000.f, 5000.f, 10000.f, 20000.f };

    g.setFont(juce::Font(9.f));
    g.setColour(juce::Colours::white.withAlpha(0.3f));

    const float logMin  = std::log10(20.f);
    const float logMax  = std::log10(20000.f);
    const float logRange = logMax - logMin;
    const float w        = static_cast<float>(m_spectrumArea.getWidth());
    const int   bottom   = m_spectrumArea.getBottom();

    for (float freq : freqMarkers)
    {
        const float norm = (std::log10(freq) - logMin) / logRange;
        const int x = m_spectrumArea.getX() + static_cast<int>(norm * w);

        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawVerticalLine(x, static_cast<float>(m_spectrumArea.getY()),
                              static_cast<float>(bottom));

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        juce::String label = (freq >= 1000.f)
            ? juce::String(static_cast<int>(freq / 1000)) + "k"
            : juce::String(static_cast<int>(freq));
        g.drawText(label, x - 12, bottom + 2, 24, 10,
                   juce::Justification::centred);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paintLevelLabels
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paintLevelLabels(juce::Graphics& g)
{
    if (m_spectrumArea.isEmpty()) return;

    g.setFont(juce::Font(9.f));
    g.setColour(juce::Colours::white.withAlpha(0.3f));

    const auto& params = m_processor.getParameterManager();
    const float dynRange = params.getDynamicRange();
    const float gridStep = 12.f;
    const int   numLines = static_cast<int>(dynRange / gridStep);

    for (int i = 0; i <= numLines; ++i)
    {
        const float norm  = static_cast<float>(i) / static_cast<float>(numLines);
        const int   y     = m_spectrumArea.getY()
                          + static_cast<int>(norm * m_spectrumArea.getHeight());
        const float dBVal = -norm * dynRange;

        g.drawText(juce::String(static_cast<int>(dBVal)) + " dB",
                   m_spectrumArea.getX() + 2, y - 5, 40, 10,
                   juce::Justification::left);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paintChannelLegend
// ─────────────────────────────────────────────────────────────────────────────
void UIController::paintChannelLegend(juce::Graphics& g)
{
    const auto& data = m_processor.getLatestAnalysisData();

    const int legendX = m_spectrumArea.getRight() - 90;
    int legendY = m_spectrumArea.getY() + 6;

    g.setFont(juce::Font(9.f));

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (!data[ch].enabled) continue;

        const juce::Colour col = m_colorManager.getPrimaryColour(ch);
        g.setColour(col);
        g.fillRect(legendX, legendY + 2, 8, 8);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawText(kBusNames[ch], legendX + 12, legendY, 76, 12,
                   juce::Justification::left);
        legendY += 14;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getTargetFPS
// ─────────────────────────────────────────────────────────────────────────────
int UIController::getTargetFPS() const noexcept
{
    const int idx = m_processor.getParameterManager().getRefreshRateIndex();

    // 0=30, 1=60, 2=adaptive, 3=120
    const int fpsList[] = { 30, 60, 60, 120 };
    if (idx >= 0 && idx < 4) return fpsList[idx];
    return 60;
}
