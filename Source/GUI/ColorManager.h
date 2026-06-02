#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
// ColorManager
//
// Provides per-channel primary colour, glow colour, and opacity.
// Also computes frequency-reactive colour shifts for the heatmap mode.
// ─────────────────────────────────────────────────────────────────────────────
class ColorManager
{
public:
    enum class ColorMode
    {
        Solid = 0,
        Gradient,
        Heatmap,
        SpectralGlow,
        Adaptive
    };

    static constexpr int kNumChannels = 12;

    ColorManager();
    ~ColorManager() = default;

    // ── Per-channel colour API ───────────────────────────────────────────────
    void setChannelColour(int ch, juce::Colour c);
    void setChannelOpacity(int ch, float opacity);
    void setColorMode(int ch, ColorMode mode);

    juce::Colour getPrimaryColour(int ch)    const noexcept;
    juce::Colour getGlowColour(int ch)       const noexcept;
    juce::Colour getSpectrumColour(int ch, float normalizedBin, float magnitudeDB) const noexcept;
    float        getOpacity(int ch)          const noexcept;

    // ── Gradient helpers ─────────────────────────────────────────────────────
    // Returns a gradient from base colour fading toward transparent
    juce::ColourGradient buildSpectrumGradient(int ch, juce::Rectangle<float> bounds) const;

    // Heatmap: maps intensity [0..1] → colour (blue → cyan → green → yellow → red)
    static juce::Colour heatmapColour(float intensity) noexcept;

    // ── Defaults ─────────────────────────────────────────────────────────────
    void resetToDefaults();

private:
    struct ChannelColor
    {
        juce::Colour primary   = juce::Colours::cyan;
        juce::Colour secondary = juce::Colours::white;
        float        opacity   = 1.0f;
        ColorMode    mode      = ColorMode::Solid;
    };

    std::array<ChannelColor, kNumChannels> m_channels;

    // Default palette — same as BusManager but owned here for rendering
    static const juce::Colour kDefaultPalette[kNumChannels];
};
