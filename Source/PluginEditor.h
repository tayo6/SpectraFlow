#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/SpectrumDisplay.h"
#include "GUI/InsertPanel.h"
#include "GUI/TopBar.h"

class SpectraFlowProcessor;

class SpectraFlowEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit SpectraFlowEditor(SpectraFlowProcessor& processor);
    ~SpectraFlowEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    SpectraFlowProcessor& m_processor;
    TopBar                m_topBar;
    SpectrumDisplay       m_display;
    InsertPanel           m_insertPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectraFlowEditor)
};
