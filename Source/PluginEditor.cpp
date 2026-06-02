#include "PluginEditor.h"
#include "PluginProcessor.h"

SpectraFlowEditor::SpectraFlowEditor(SpectraFlowProcessor& processor)
    : AudioProcessorEditor(&processor),
      m_processor(processor),
      m_ui(processor)
{
    addAndMakeVisible(m_ui);

    // Resizable editor — min 800x500, max 2560x1440
    setResizable(true, true);
    setResizeLimits(800, 500, 2560, 1440);
    setSize(1200, 700);
}

SpectraFlowEditor::~SpectraFlowEditor() = default;

void SpectraFlowEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SpectraFlowEditor::resized()
{
    m_ui.setBounds(getLocalBounds());
}