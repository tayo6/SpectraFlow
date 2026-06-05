#include "PluginEditor.h"
#include "PluginProcessor.h"

SpectraFlowEditor::SpectraFlowEditor(SpectraFlowProcessor& processor)
    : AudioProcessorEditor(&processor),
      m_processor(processor),
      m_topBar(processor.getAPVTS())
{
    // ── Add components ───────────────────────────────────────────────────────
    addAndMakeVisible(m_topBar);
    addAndMakeVisible(m_display);
    addAndMakeVisible(m_insertPanel);

    // ── Wire TopBar view toggle → SpectrumDisplay ────────────────────────────
    m_topBar.onViewToggle = [this](bool isSpectrum)
    {
        m_display.setViewMode(isSpectrum
            ? SpectrumDisplay::ViewMode::Spectrum
            : SpectrumDisplay::ViewMode::Waveform);
    };

    // ── Wire InsertPanel colour/enable changes → SpectrumDisplay ─────────────
    m_insertPanel.onColourChanged = [this](int ch, juce::Colour c)
    {
        m_display.setChannelColour(ch, c);
    };

    m_insertPanel.onEnabledChanged = [this](int ch, bool enabled)
    {
        m_display.setChannelEnabled(ch, enabled);
    };

    // ── Window size ──────────────────────────────────────────────────────────
    setResizable(true, true);
    setResizeLimits(800, 480, 2560, 1440);
    setSize(1100, 620);

    // ── Start repaint timer at 60 Hz ─────────────────────────────────────────
    startTimerHz(60);
}

SpectraFlowEditor::~SpectraFlowEditor()
{
    stopTimer();
}

void SpectraFlowEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0D0E14));
}

void SpectraFlowEditor::resized()
{
    auto area = getLocalBounds();

    // Top bar
    m_topBar.setBounds(area.removeFromTop(38));

    // Right panel — 28% of remaining width, minimum 260px
    const int panelW = juce::jmax(260, static_cast<int>(area.getWidth() * 0.28f));
    m_insertPanel.setBounds(area.removeFromRight(panelW));

    // Spectrum display takes all remaining space
    m_display.setBounds(area);
}

void SpectraFlowEditor::timerCallback()
{
    // Pull latest analysis data from processor
    m_processor.swapAnalysisSnapshot();
    m_display.pushData(m_processor.getLatestAnalysisData());

    // Update channel levels in insert panel from rms data
    const auto& data = m_processor.getLatestAnalysisData();
    for (int i = 0; i < 12; ++i)
    {
        // Convert dB to 0..1
        const float rms = data[i].rmsLevel;
        const float norm = juce::jlimit(0.f, 1.f, (rms + 90.f) / 90.f);
        m_insertPanel.setChannelLevel(i, norm);
    }

    // CPU readout
    m_topBar.setCPU(0.f); // placeholder — real value from OS perf counter in Phase 3
}
