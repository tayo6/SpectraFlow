#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/UIController.h"

class SpectraFlowProcessor;

// ─────────────────────────────────────────────────────────────────────────────
// SpectraFlowEditor
//
// Thin wrapper that hosts the UIController component.
// Keeps the editor/processor boundary clean.
// ─────────────────────────────────────────────────────────────────────────────
class SpectraFlowEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpectraFlowEditor(SpectraFlowProcessor& processor);
    ~SpectraFlowEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SpectraFlowProcessor& m_processor;
    UIController          m_ui;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectraFlowEditor)
};