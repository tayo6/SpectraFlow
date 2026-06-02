#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// BusManager
//
// Tracks which JUCE buses are active and maps them to analysis channel indices.
// Also owns per-channel metadata (name, color hint, etc.) that the GUI uses.
// ─────────────────────────────────────────────────────────────────────────────
class BusManager
{
public:
    struct ChannelInfo
    {
        std::string  name;
        juce::Colour defaultColour;
        bool         isActive   = false;
        int          busIndex   = -1;   // JUCE bus index (-1 = not mapped)
    };

    explicit BusManager(int numChannels);
    ~BusManager() = default;

    // Call from prepareToPlay to refresh bus mapping
    void refreshBusMapping(juce::AudioProcessor& processor);

    const ChannelInfo& getChannelInfo(int ch) const noexcept
    {
        jassert(ch >= 0 && ch < static_cast<int>(m_channels.size()));
        return m_channels[ch];
    }

    int getNumChannels() const noexcept { return static_cast<int>(m_channels.size()); }

private:
    std::vector<ChannelInfo> m_channels;

    // Default colours for each of the 12 channels — visually distinct palette
    static const juce::Colour kDefaultColours[12];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BusManager)
};
