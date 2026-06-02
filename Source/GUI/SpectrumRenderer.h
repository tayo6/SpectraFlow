#pragma once

#include <juce_graphics/juce_graphics.h>
#include <vector>
#include "ColorManager.h"

struct ChannelAnalysisData;

// ─────────────────────────────────────────────────────────────────────────────
// SpectrumRenderer
//
// Draws one or more overlapping FFT spectra into a juce::Graphics context.
// Called exclusively from the GUI (message) thread.
//
// Optimisations:
//   - Pre-allocated Path objects, cleared and reused each frame
//   - Log-frequency bin mapping cached; only rebuilt on size/scale change
//   - Dirty-region tracking — only repaints if data changed
//   - Filled spectrum, line spectrum, or hybrid modes
// ─────────────────────────────────────────────────────────────────────────────
class SpectrumRenderer
{
public:
    enum class DisplayMode
    {
        FilledSpectrum = 0,
        LineSpectrum,
        Hybrid
    };

    SpectrumRenderer();
    ~SpectrumRenderer() = default;

    // Call when the component bounds change
    void setBounds(juce::Rectangle<int> bounds);

    // Call when FFT size or sample rate changes
    void setParameters(int fftSize, double sampleRate,
                       float minFreq = 20.f, float maxFreq = 22050.f);

    void setDisplayMode(DisplayMode m)        { m_displayMode = m; }
    void setDynamicRange(float dB)            { m_dynamicRangeDB = dB; }
    void setFrequencyScale(int scaleIdx)      { m_freqScaleIdx = scaleIdx; }
    void setGlowEnabled(bool g)               { m_glowEnabled = g; }
    void setPeakHoldVisible(bool v)           { m_peakHoldVisible = v; }

    // Main draw call — renders all enabled channels into g
    void draw(juce::Graphics& g,
              const std::array<ChannelAnalysisData, 12>& data,
              const ColorManager& colors,
              const juce::Rectangle<int>& clipBounds);

private:
    // ── Cached geometry ──────────────────────────────────────────────────────
    juce::Rectangle<int>   m_bounds;
    int                    m_fftSize     = 2048;
    double                 m_sampleRate  = 44100.0;
    float                  m_minFreq     = 20.f;
    float                  m_maxFreq     = 22050.f;
    float                  m_dynamicRangeDB = 90.f;
    int                    m_freqScaleIdx   = 0;     // 0=Log
    DisplayMode            m_displayMode    = DisplayMode::FilledSpectrum;
    bool                   m_glowEnabled    = true;
    bool                   m_peakHoldVisible= true;

    // Bin → pixel X mapping (precomputed, length = fftSize/2)
    std::vector<float>     m_binToX;
    bool                   m_mappingDirty = true;

    // Reusable path objects — avoid heap allocation every frame
    juce::Path             m_spectrumPath;
    juce::Path             m_peakPath;
    juce::Path             m_glowPath;

    // ── Private helpers ──────────────────────────────────────────────────────
    void rebuildBinMapping();
    void buildSpectrumPath(const std::vector<float>& magnitudesDB,
                           juce::Path& path, bool filled) const;
    void buildPeakPath(const std::vector<float>& peakDB, juce::Path& path) const;

    float magnitudeToY(float dB) const noexcept;
    float freqToX(float freq)    const noexcept;
    float binIndexToX(int bin)   const noexcept;

    // Log / Mel / Bark frequency normalisation
    float normalizeFreq(float freq) const noexcept;
};
