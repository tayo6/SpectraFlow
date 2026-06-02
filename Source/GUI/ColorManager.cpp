#include "ColorManager.h"
#include <algorithm>
#include <cmath>

const juce::Colour ColorManager::kDefaultPalette[kNumChannels] =
{
    juce::Colour(0xFF4FC3F7),
    juce::Colour(0xFFFFB74D),
    juce::Colour(0xFFAED581),
    juce::Colour(0xFFE57373),
    juce::Colour(0xFFCE93D8),
    juce::Colour(0xFF80CBC4),
    juce::Colour(0xFFFFCC02),
    juce::Colour(0xFF80DEEA),
    juce::Colour(0xFFFFFFFF),
    juce::Colour(0xFFEF9A9A),
    juce::Colour(0xFFBCAAA4),
    juce::Colour(0xFFA5D6A7),
};

ColorManager::ColorManager()
{
    resetToDefaults();
}

void ColorManager::resetToDefaults()
{
    for (int i = 0; i < kNumChannels; ++i)
    {
        m_channels[i].primary   = kDefaultPalette[i];
        m_channels[i].secondary = kDefaultPalette[i].brighter(0.6f);
        m_channels[i].opacity   = 1.0f;
        m_channels[i].mode      = ColorMode::Solid;
    }
}

void ColorManager::setChannelColour(int ch, juce::Colour c)
{
    if (ch >= 0 && ch < kNumChannels)
    {
        m_channels[ch].primary   = c;
        m_channels[ch].secondary = c.brighter(0.5f);
    }
}

void ColorManager::setChannelOpacity(int ch, float opacity)
{
    if (ch >= 0 && ch < kNumChannels)
        m_channels[ch].opacity = juce::jlimit(0.f, 1.f, opacity);
}

void ColorManager::setColorMode(int ch, ColorMode mode)
{
    if (ch >= 0 && ch < kNumChannels)
        m_channels[ch].mode = mode;
}

juce::Colour ColorManager::getPrimaryColour(int ch) const noexcept
{
    if (ch < 0 || ch >= kNumChannels) return juce::Colours::white;
    return m_channels[ch].primary.withAlpha(m_channels[ch].opacity);
}

juce::Colour ColorManager::getGlowColour(int ch) const noexcept
{
    if (ch < 0 || ch >= kNumChannels) return juce::Colours::white;
    return m_channels[ch].secondary.withAlpha(m_channels[ch].opacity * 0.4f);
}

juce::Colour ColorManager::getSpectrumColour(int ch,
                                               float normalizedBin,
                                               float magnitudeDB) const noexcept
{
    if (ch < 0 || ch >= kNumChannels) return juce::Colours::white;

    const auto& cc = m_channels[ch];

    switch (cc.mode)
    {
        case ColorMode::Solid:
            return cc.primary.withAlpha(cc.opacity);

        case ColorMode::Gradient:
        {
            // Frequency gradient: base colour at low end, brightened at high
            const float t = std::pow(normalizedBin, 0.5f);
            return cc.primary.interpolatedWith(cc.secondary, t).withAlpha(cc.opacity);
        }

        case ColorMode::Heatmap:
        {
            // Map dB to 0..1 intensity (assumes -90..0 dB range)
            const float intensity = juce::jlimit(0.f, 1.f, (magnitudeDB + 90.f) / 90.f);
            return heatmapColour(intensity).withAlpha(cc.opacity);
        }

        case ColorMode::SpectralGlow:
        {
            // Primary tinted by frequency
            const float hueShift = normalizedBin * 0.15f;
            const float hue = std::fmod(cc.primary.getHue() + hueShift, 1.f);
            return juce::Colour::fromHSL(hue,
                                          cc.primary.getSaturation(),
                                          cc.primary.getLightness(),
                                          cc.opacity);
        }

        case ColorMode::Adaptive:
        {
            // Loudness-reactive brightness
            const float bright = juce::jlimit(0.f, 1.f, (magnitudeDB + 60.f) / 60.f);
            return cc.primary.withBrightness(0.4f + bright * 0.6f).withAlpha(cc.opacity);
        }
    }
    return cc.primary;
}

juce::ColourGradient ColorManager::buildSpectrumGradient(int ch,
                                                           juce::Rectangle<float> bounds) const
{
    const juce::Colour base  = getPrimaryColour(ch);
    const juce::Colour clear = base.withAlpha(0.f);

    juce::ColourGradient g(base,  bounds.getBottomLeft(),
                           clear, bounds.getTopLeft(), false);
    g.addColour(0.7, base.withAlpha(base.getAlpha() * 0.6f));
    return g;
}

// Static heatmap: blue → cyan → green → yellow → red
juce::Colour ColorManager::heatmapColour(float t) noexcept
{
    t = juce::jlimit(0.f, 1.f, t);

    // 5-stop gradient
    const juce::Colour stops[] = {
        juce::Colour(0xFF0D0221),  // near-black (silence)
        juce::Colour(0xFF0038FF),  // blue
        juce::Colour(0xFF00EFFF),  // cyan
        juce::Colour(0xFF00FF66),  // green
        juce::Colour(0xFFFFFF00),  // yellow
        juce::Colour(0xFFFF0000),  // red (full)
    };
    constexpr int nStops = 6;
    const float scaled = t * (nStops - 1);
    const int   idx    = static_cast<int>(scaled);
    const float frac   = scaled - static_cast<float>(idx);
    const int   next   = std::min(idx + 1, nStops - 1);
    return stops[idx].interpolatedWith(stops[next], frac);
}
