#include "BusManager.h"
#include "../PluginProcessor.h"

// 12 visually distinct colours, one per channel
const juce::Colour BusManager::kDefaultColours[12] =
{
    juce::Colour(0xFF4FC3F7),  // Bass         — cyan-blue
    juce::Colour(0xFFFFB74D),  // Lead Vocal   — amber
    juce::Colour(0xFFAED581),  // Backup Vocal — light green
    juce::Colour(0xFFE57373),  // Drums        — red
    juce::Colour(0xFFCE93D8),  // Synth        — lavender
    juce::Colour(0xFF80CBC4),  // FX           — teal
    juce::Colour(0xFFFFCC02),  // Percussion   — yellow
    juce::Colour(0xFF80DEEA),  // Pads         — light cyan
    juce::Colour(0xFFFFFFFF),  // Master       — white
    juce::Colour(0xFFEF9A9A),  // Reference    — pink
    juce::Colour(0xFFBCAAA4),  // Sidechain    — warm grey
    juce::Colour(0xFFA5D6A7),  // Utility      — mint green
};

BusManager::BusManager(int numChannels)
{
    m_channels.resize(numChannels);

    for (int i = 0; i < numChannels && i < 12; ++i)
    {
        m_channels[i].name         = SpectraFlow::kBusNames[i];
        m_channels[i].defaultColour = kDefaultColours[i];
        m_channels[i].isActive     = (i == 0);  // only bus 0 active by default
        m_channels[i].busIndex     = i;
    }
}

void BusManager::refreshBusMapping(juce::AudioProcessor& processor)
{
    const int numBuses = processor.getBusCount(true);

    for (int ch = 0; ch < static_cast<int>(m_channels.size()); ++ch)
    {
        if (ch < numBuses)
        {
            auto* bus = processor.getBus(true, ch);
            m_channels[ch].isActive = (bus != nullptr && bus->isEnabled());
            m_channels[ch].busIndex = ch;
        }
        else
        {
            m_channels[ch].isActive = false;
            m_channels[ch].busIndex = -1;
        }
    }
}
