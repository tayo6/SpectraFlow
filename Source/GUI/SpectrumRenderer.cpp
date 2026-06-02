#include "SpectrumRenderer.h"
#include "../PluginProcessor.h"
#include <cmath>
#include <algorithm>

SpectrumRenderer::SpectrumRenderer()
{
    setParameters(2048, 44100.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// setBounds
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::setBounds(juce::Rectangle<int> bounds)
{
    if (m_bounds != bounds)
    {
        m_bounds       = bounds;
        m_mappingDirty = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// setParameters
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::setParameters(int fftSize, double sampleRate,
                                      float minFreq, float maxFreq)
{
    m_fftSize     = fftSize;
    m_sampleRate  = sampleRate;
    m_minFreq     = minFreq;
    m_maxFreq     = std::min(maxFreq, static_cast<float>(sampleRate * 0.5));
    m_mappingDirty = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// rebuildBinMapping — precompute X position for every FFT bin
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::rebuildBinMapping()
{
    if (!m_mappingDirty) return;
    m_mappingDirty = false;

    const int numBins = m_fftSize / 2;
    m_binToX.resize(numBins);

    const float width    = static_cast<float>(m_bounds.getWidth());
    const float normMin  = normalizeFreq(m_minFreq);
    const float normMax  = normalizeFreq(m_maxFreq);
    const float normRange = normMax - normMin;

    for (int bin = 0; bin < numBins; ++bin)
    {
        const float freq = static_cast<float>(bin) * static_cast<float>(m_sampleRate)
                           / static_cast<float>(m_fftSize);
        const float norm = (normalizeFreq(freq) - normMin) / normRange;
        m_binToX[bin] = static_cast<float>(m_bounds.getX()) + norm * width;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// draw — main render call
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::draw(juce::Graphics& g,
                             const std::array<ChannelAnalysisData, 12>& data,
                             const ColorManager& colors,
                             const juce::Rectangle<int>& /*clipBounds*/)
{
    rebuildBinMapping();

    // Check if any channel is soloed
    bool anySolo = false;
    for (int ch = 0; ch < 12; ++ch)
        if (data[ch].enabled && colors.getPrimaryColour(ch).getAlpha() > 0)
        {
            // We track solo state via opacity (0 = muted for now)
            // Full solo logic lives in UIController
        }
    (void)anySolo;

    // Draw each channel back-to-front for natural overlay
    for (int ch = 11; ch >= 0; --ch)
    {
        const auto& ch_data = data[ch];
        if (!ch_data.enabled || ch_data.magnitudeDB.empty())
            continue;

        const juce::Colour primaryCol = colors.getPrimaryColour(ch);
        if (primaryCol.getAlpha() == 0) continue;

        // ── Glow pass (drawn first, wider stroke, lower alpha) ────────────
        if (m_glowEnabled)
        {
            m_glowPath.clear();
            buildSpectrumPath(ch_data.magnitudeDB, m_glowPath, false);

            const juce::Colour glowCol = colors.getGlowColour(ch);
            g.setColour(glowCol);
            juce::PathStrokeType glowStroke(4.f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded);
            g.strokePath(m_glowPath, glowStroke);
        }

        // ── Filled spectrum ───────────────────────────────────────────────
        if (m_displayMode == DisplayMode::FilledSpectrum ||
            m_displayMode == DisplayMode::Hybrid)
        {
            m_spectrumPath.clear();
            buildSpectrumPath(ch_data.magnitudeDB, m_spectrumPath, true);

            // Vertical gradient fill
            const auto gradient = colors.buildSpectrumGradient(
                ch, m_bounds.toFloat());
            g.setGradientFill(gradient);
            g.fillPath(m_spectrumPath);
        }

        // ── Line spectrum ─────────────────────────────────────────────────
        if (m_displayMode == DisplayMode::LineSpectrum ||
            m_displayMode == DisplayMode::Hybrid)
        {
            m_spectrumPath.clear();
            buildSpectrumPath(ch_data.magnitudeDB, m_spectrumPath, false);

            g.setColour(primaryCol);
            juce::PathStrokeType lineStroke(1.5f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded);
            g.strokePath(m_spectrumPath, lineStroke);
        }

        // ── Peak hold markers ─────────────────────────────────────────────
        if (m_peakHoldVisible && !ch_data.peakHold.empty())
        {
            m_peakPath.clear();
            buildPeakPath(ch_data.peakHold, m_peakPath);

            g.setColour(primaryCol.withAlpha(primaryCol.getFloatAlpha() * 0.7f));
            juce::PathStrokeType peakStroke(1.0f);
            g.strokePath(m_peakPath, peakStroke);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildSpectrumPath
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::buildSpectrumPath(const std::vector<float>& magnitudesDB,
                                          juce::Path& path,
                                          bool filled) const
{
    if (m_binToX.empty() || magnitudesDB.empty()) return;

    const float bottom  = static_cast<float>(m_bounds.getBottom());
    const int   numBins = std::min(static_cast<int>(magnitudesDB.size()),
                                   static_cast<int>(m_binToX.size()));

    if (filled) path.startNewSubPath(m_binToX[0], bottom);

    bool started = false;

    for (int bin = 1; bin < numBins; ++bin)
    {
        const float x = m_binToX[bin];
        const float y = magnitudeToY(magnitudesDB[bin]);

        if (!started)
        {
            if (filled)
                path.lineTo(x, y);
            else
                path.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    if (filled && started)
    {
        path.lineTo(m_binToX[numBins - 1], bottom);
        path.closeSubPath();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildPeakPath — short horizontal dashes at each peak-hold position
// ─────────────────────────────────────────────────────────────────────────────
void SpectrumRenderer::buildPeakPath(const std::vector<float>& peakDB,
                                      juce::Path& path) const
{
    if (m_binToX.empty() || peakDB.empty()) return;

    const int numBins = std::min(static_cast<int>(peakDB.size()),
                                 static_cast<int>(m_binToX.size()));
    const float dashHalf = 2.f;

    // Only draw every Nth bin to reduce path complexity
    const int stride = std::max(1, numBins / 256);

    for (int bin = 1; bin < numBins; bin += stride)
    {
        const float x = m_binToX[bin];
        const float y = magnitudeToY(peakDB[bin]);
        if (y < static_cast<float>(m_bounds.getBottom()) - 2.f)
        {
            path.startNewSubPath(x - dashHalf, y);
            path.lineTo         (x + dashHalf, y);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate helpers
// ─────────────────────────────────────────────────────────────────────────────
float SpectrumRenderer::magnitudeToY(float dB) const noexcept
{
    const float top    = static_cast<float>(m_bounds.getY());
    const float bottom = static_cast<float>(m_bounds.getBottom());
    const float height = bottom - top;

    // Map [0, -dynamicRange] dB → [top, bottom]
    const float norm = juce::jlimit(0.f, 1.f, -dB / m_dynamicRangeDB);
    return top + norm * height;
}

float SpectrumRenderer::normalizeFreq(float freq) const noexcept
{
    switch (m_freqScaleIdx)
    {
        default:
        case 0: // Logarithmic
            return (freq > 0.f) ? std::log10(freq) : 0.f;

        case 1: // Linear
            return freq;

        case 2: // Mel
            return 2595.f * std::log10(1.f + freq / 700.f);

        case 3: // Bark
        {
            const float f_kHz = freq / 1000.f;
            return 13.f * std::atan(0.76f * f_kHz)
                 + 3.5f * std::atan((f_kHz / 7.5f) * (f_kHz / 7.5f));
        }
    }
}

float SpectrumRenderer::freqToX(float freq) const noexcept
{
    const float width    = static_cast<float>(m_bounds.getWidth());
    const float normMin  = normalizeFreq(m_minFreq);
    const float normMax  = normalizeFreq(m_maxFreq);
    const float norm     = (normalizeFreq(freq) - normMin) / (normMax - normMin);
    return static_cast<float>(m_bounds.getX()) + norm * width;
}

float SpectrumRenderer::binIndexToX(int bin) const noexcept
{
    if (bin >= 0 && bin < static_cast<int>(m_binToX.size()))
        return m_binToX[bin];
    return 0.f;
}
