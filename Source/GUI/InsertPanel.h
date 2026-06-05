#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
// InsertSlot — one routed channel row
//
// Contains:
//   • Coloured swatch (opens native colour picker)
//   • Insert number label
//   • Source name text editor (type the track/bus name)
//   • Enable toggle
//   • Remove button
// ─────────────────────────────────────────────────────────────────────────────
class InsertSlot : public juce::Component,
                   public juce::Button::Listener,
                   public juce::Label::Listener
{
public:
    struct State
    {
        juce::String sourceName { "— empty —" };
        juce::Colour colour     { juce::Colour(0xFF4FC3F7) };
        bool         enabled    { true };
    };

    std::function<void(int)>              onRemove;
    std::function<void(int, bool)>        onToggle;
    std::function<void(int, juce::Colour)>onColourChanged;
    std::function<void(int, juce::String)>onNameChanged;

    InsertSlot(int index, const State& s);
    ~InsertSlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setState(const State& s);
    const State& getState() const noexcept { return m_state; }
    int  getIndex()         const noexcept { return m_index; }

    // Level meter — called from timer
    void setLevel(float normalised);  // 0..1

private:
    void buttonClicked(juce::Button* b) override;
    void labelTextChanged(juce::Label* l) override;
    void openColourPicker();

    int   m_index;
    State m_state;

    juce::TextButton m_swatchBtn;      // colour swatch
    juce::Label      m_numLabel;       // "01"
    juce::Label      m_nameLabel;      // editable source name
    juce::TextButton m_toggleBtn;      // ON/OFF
    juce::TextButton m_removeBtn;      // ×
    float            m_level = 0.f;    // 0..1 for meter

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InsertSlot)
};

// ─────────────────────────────────────────────────────────────────────────────
// InsertPanel — right-side panel housing all insert slots + add button
// ─────────────────────────────────────────────────────────────────────────────
class InsertPanel : public juce::Component,
                    public juce::Button::Listener
{
public:
    static constexpr int kMaxInserts = 12;

    // Fired when colour or enabled state changes — editor wires these to
    // SpectrumDisplay
    std::function<void(int, juce::Colour)> onColourChanged;
    std::function<void(int, bool)>         onEnabledChanged;

    InsertPanel();
    ~InsertPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Level feed from processor (0..1 per channel)
    void setChannelLevel(int ch, float level);

    // Sync enabled/colour from APVTS
    void setChannelColour(int ch, juce::Colour c);
    void setChannelEnabled(int ch, bool e);

    int  getNumInserts() const noexcept { return static_cast<int>(m_slots.size()); }

private:
    void buttonClicked(juce::Button* b) override;
    void addInsert();
    void removeInsert(int idx);
    void layoutSlots();

    std::vector<std::unique_ptr<InsertSlot>> m_slots;
    juce::TextButton                          m_addBtn;
    juce::Viewport                            m_viewport;
    juce::Component                           m_slotContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InsertPanel)
};
