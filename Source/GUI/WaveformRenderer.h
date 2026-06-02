#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>
#include "ColorManager.h"

struct ChannelAnalysisData;

// ─────────────────────────────────────────────────────────────────────────────
// WaveformRenderer
//
// Draws multichannel waveform snapshots into a juce::Graphics context.
// Optimised to reuse Path objects and skip invisible channels.
// ─────────────────────────────────────────────────────────────────────────────
class WaveformRenderer
{
public:
    WaveformRenderer();
    ~WaveformRenderer() = default;

    void setBounds(juce::Rectangle<int> bounds);

    void draw(juce::Graphics& g,
              const std::array<ChannelAnalysisData, 12>& data,
              const ColorManager& colors);

    void setAmplitudeScale(float scale) { m_amplitudeScale = scale; }
    void setWaveformHeight(float ratio) { m_heightRatio    = ratio; }  // 0..1 of bounds

private:
    juce::Rectangle<int>  m_bounds;
    juce::Path            m_wavePath;
    float                 m_amplitudeScale = 1.f;
    float                 m_heightRatio    = 0.25f;  // waveform takes bottom 25%

    void buildWavePath(const std::vector<float>& waveform,
                       juce::Path& path,
                       juce::Rectangle<float> drawArea) const;
};
