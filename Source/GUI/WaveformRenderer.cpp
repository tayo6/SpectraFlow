#include "WaveformRenderer.h"
#include "../PluginProcessor.h"
#include <algorithm>

WaveformRenderer::WaveformRenderer() = default;

void WaveformRenderer::setBounds(juce::Rectangle<int> bounds)
{
    m_bounds = bounds;
}

void WaveformRenderer::draw(juce::Graphics& g,
                              const std::array<ChannelAnalysisData, 12>& data,
                              const ColorManager& colors)
{
    if (m_bounds.isEmpty()) return;

    const float fullH   = static_cast<float>(m_bounds.getHeight());
    const float areaH   = fullH * m_heightRatio;
    const float areaTop = static_cast<float>(m_bounds.getBottom()) - areaH;

    const juce::Rectangle<float> drawArea(
        static_cast<float>(m_bounds.getX()),
        areaTop,
        static_cast<float>(m_bounds.getWidth()),
        areaH);

    // Thin separator line
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(static_cast<int>(areaTop),
                         drawArea.getX(), drawArea.getRight());

    for (int ch = 0; ch < 12; ++ch)
    {
        const auto& ch_data = data[ch];
        if (!ch_data.enabled || ch_data.waveform.empty())
            continue;

        const juce::Colour col = colors.getPrimaryColour(ch);
        if (col.getAlpha() == 0) continue;

        m_wavePath.clear();
        buildWavePath(ch_data.waveform, m_wavePath, drawArea);

        g.setColour(col.withAlpha(col.getFloatAlpha() * 0.8f));
        juce::PathStrokeType stroke(1.0f, juce::PathStrokeType::curved);
        g.strokePath(m_wavePath, stroke);
    }
}

void WaveformRenderer::buildWavePath(const std::vector<float>& waveform,
                                      juce::Path& path,
                                      juce::Rectangle<float> drawArea) const
{
    const int n = static_cast<int>(waveform.size());
    if (n < 2) return;

    const float xStep  = drawArea.getWidth() / static_cast<float>(n - 1);
    const float midY   = drawArea.getCentreY();
    const float halfH  = drawArea.getHeight() * 0.5f * m_amplitudeScale;

    path.startNewSubPath(drawArea.getX(),
                         midY - waveform[0] * halfH);

    for (int i = 1; i < n; ++i)
    {
        const float x = drawArea.getX() + i * xStep;
        const float y = midY - juce::jlimit(-1.f, 1.f, waveform[i]) * halfH;
        path.lineTo(x, y);
    }
}
