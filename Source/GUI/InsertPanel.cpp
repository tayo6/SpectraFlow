#include "InsertPanel.h"

// ─── Colours ────────────────────────────────────────────────────────────────
namespace SF {
    static const juce::Colour kBg       { 0xFF0F1018 };
    static const juce::Colour kBg2      { 0xFF13151F };
    static const juce::Colour kBg3      { 0xFF191C28 };
    static const juce::Colour kBorder   { 0x14FFFFFF };
    static const juce::Colour kBorder2  { 0x22FFFFFF };
    static const juce::Colour kText     { 0xFFDDE0EC };
    static const juce::Colour kMuted    { 0xFF525870 };
    static const juce::Colour kAccent   { 0xFF4AF0C4 };

    static const juce::Colour kPalette[12] = {
        juce::Colour(0xFF4FC3F7), juce::Colour(0xFFFFB74D),
        juce::Colour(0xFFAED581), juce::Colour(0xFFF06292),
        juce::Colour(0xFFCE93D8), juce::Colour(0xFF4DD0E1),
        juce::Colour(0xFFFFF176), juce::Colour(0xFF80CBC4),
        juce::Colour(0xFFE0E0E0), juce::Colour(0xFFEF9A9A),
        juce::Colour(0xFFBCAAA4), juce::Colour(0xFFA5D6A7),
    };

    static juce::Font monoFont(float size = 10.f)
    {
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain);
    }
    static juce::Font uiFont(float size = 11.f, bool bold = false)
    {
        return juce::Font(size, bold ? juce::Font::bold : juce::Font::plain);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// InsertSlot
// ═════════════════════════════════════════════════════════════════════════════

InsertSlot::InsertSlot(int index, const State& s)
    : m_index(index), m_state(s)
{
    // Swatch button — filled with channel colour
    m_swatchBtn.setButtonText({});
    m_swatchBtn.addListener(this);
    addAndMakeVisible(m_swatchBtn);

    // Number
    m_numLabel.setText(juce::String(index + 1).paddedLeft('0', 2),
                       juce::dontSendNotification);
    m_numLabel.setFont(SF::monoFont(8.f));
    m_numLabel.setColour(juce::Label::textColourId, SF::kMuted);
    m_numLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_numLabel);

    // Name label — editable
    m_nameLabel.setText(s.sourceName, juce::dontSendNotification);
    m_nameLabel.setFont(SF::uiFont(10.f));
    m_nameLabel.setColour(juce::Label::textColourId,
                          s.sourceName == "— empty —" ? SF::kMuted : SF::kText);
    m_nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    m_nameLabel.setColour(juce::Label::backgroundWhenEditingColourId, SF::kBg3);
    m_nameLabel.setColour(juce::Label::outlineWhenEditingColourId,
                          s.colour.withAlpha(0.6f));
    m_nameLabel.setEditable(true, true);
    m_nameLabel.addListener(this);
    addAndMakeVisible(m_nameLabel);

    // Toggle
    m_toggleBtn.setButtonText(s.enabled ? "ON" : "OFF");
    m_toggleBtn.setColour(juce::TextButton::buttonColourId,
                          s.enabled
                            ? s.colour.withAlpha(0.25f)
                            : SF::kBg3);
    m_toggleBtn.setColour(juce::TextButton::textColourOffId,
                          s.enabled ? s.colour : SF::kMuted);
    m_toggleBtn.addListener(this);
    addAndMakeVisible(m_toggleBtn);

    // Remove
    m_removeBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x97")); // ×
    m_removeBtn.setColour(juce::TextButton::buttonColourId,  SF::kBg3);
    m_removeBtn.setColour(juce::TextButton::textColourOffId, SF::kMuted);
    m_removeBtn.addListener(this);
    addAndMakeVisible(m_removeBtn);

    setState(s);
}

InsertSlot::~InsertSlot()
{
    m_swatchBtn.removeListener(this);
    m_toggleBtn.removeListener(this);
    m_removeBtn.removeListener(this);
    m_nameLabel.removeListener(this);
}

void InsertSlot::setState(const State& s)
{
    m_state = s;

    m_nameLabel.setText(s.sourceName, juce::dontSendNotification);
    m_nameLabel.setColour(juce::Label::textColourId,
                          s.sourceName == "— empty —" ? SF::kMuted : s.colour);
    m_nameLabel.setColour(juce::Label::outlineWhenEditingColourId,
                          s.colour.withAlpha(0.6f));

    m_toggleBtn.setButtonText(s.enabled ? "ON" : "OFF");
    m_toggleBtn.setColour(juce::TextButton::buttonColourId,
                          s.enabled ? s.colour.withAlpha(0.22f) : SF::kBg3);
    m_toggleBtn.setColour(juce::TextButton::textColourOffId,
                          s.enabled ? s.colour : SF::kMuted);

    repaint();
}

void InsertSlot::setLevel(float normalised)
{
    m_level = juce::jlimit(0.f, 1.f, normalised);
    repaint();
}

// ── paint ─────────────────────────────────────────────────────────────────────
void InsertSlot::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced(1.f);

    // Slot background
    const bool active = m_state.enabled && m_state.sourceName != "— empty —";
    g.setColour(active
        ? m_state.colour.withAlpha(0.06f).overlaidWith(SF::kBg2)
        : SF::kBg2);
    g.fillRoundedRectangle(b, 5.f);

    // Border — coloured when active
    g.setColour(active
        ? m_state.colour.withAlpha(0.30f)
        : SF::kBorder);
    g.drawRoundedRectangle(b, 5.f, 1.f);

    // Colour swatch (vertical bar on the left)
    const float swX = b.getX() + 6.f;
    const float swY = b.getY() + 6.f;
    const float swW = 8.f;
    const float swH = b.getHeight() - 12.f;
    g.setColour(m_state.colour);
    g.fillRoundedRectangle(swX, swY, swW, swH, 2.f);

    // Level meter bar at the bottom of the slot
    const float meterH = 2.f;
    const float meterY = b.getBottom() - meterH - 5.f;
    const float meterX = swX + swW + 6.f;
    const float meterW = b.getRight() - meterX - 8.f;

    // Meter background
    g.setColour(SF::kBg3);
    g.fillRoundedRectangle(meterX, meterY, meterW, meterH, 1.f);

    // Meter fill
    if (m_level > 0.f && active)
    {
        g.setColour(m_state.colour.withAlpha(0.8f));
        g.fillRoundedRectangle(meterX, meterY, meterW * m_level, meterH, 1.f);
    }
}

void InsertSlot::resized()
{
    auto area = getLocalBounds().reduced(2);

    // Skip swatch area on left
    area.removeFromLeft(18);

    // Number on left
    m_numLabel.setBounds(area.removeFromLeft(20));

    // Controls on right
    m_removeBtn.setBounds(area.removeFromRight(20).reduced(0, 6));
    area.removeFromRight(3);
    m_toggleBtn.setBounds(area.removeFromRight(34).reduced(0, 6));
    area.removeFromRight(5);

    // Name in the middle
    area.removeFromBottom(8); // space for meter
    m_nameLabel.setBounds(area.reduced(2, 4));

    // Swatch button covers the left swatch area
    m_swatchBtn.setBounds(getLocalBounds().reduced(2)
                          .removeFromLeft(18)
                          .reduced(4, 5));
}

void InsertSlot::buttonClicked(juce::Button* b)
{
    if (b == &m_swatchBtn)  openColourPicker();
    if (b == &m_toggleBtn)  { if (onToggle)  onToggle(m_index, !m_state.enabled); }
    if (b == &m_removeBtn)  { if (onRemove)  onRemove(m_index); }
}

void InsertSlot::labelTextChanged(juce::Label* l)
{
    if (l == &m_nameLabel)
    {
        m_state.sourceName = l->getText().isEmpty() ? "— empty —" : l->getText();
        m_nameLabel.setColour(juce::Label::textColourId,
            m_state.sourceName == "— empty —" ? SF::kMuted : m_state.colour);
        if (onNameChanged) onNameChanged(m_index, m_state.sourceName);
    }
}

void InsertSlot::openColourPicker()
{
    auto* picker = new juce::ColourSelector(
        juce::ColourSelector::showColourAtTop |
        juce::ColourSelector::showSliders     |
        juce::ColourSelector::showColourspace);

    picker->setName("Pick channel colour");
    picker->setCurrentColour(m_state.colour);
    picker->setSize(300, 380);

    picker->addChangeListener(
        juce::ChangeBroadcaster::Listener::createFromLambda(
            [this, picker]()
            {
                m_state.colour = picker->getCurrentColour();
                setState(m_state);
                if (onColourChanged) onColourChanged(m_index, m_state.colour);
            }));

    juce::CallOutBox::launchAsynchronously(
        std::unique_ptr<juce::Component>(picker),
        getScreenBounds(), nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// InsertPanel
// ═════════════════════════════════════════════════════════════════════════════

InsertPanel::InsertPanel()
{
    // Add button
    m_addBtn.setButtonText("+ Add insert");
    m_addBtn.setColour(juce::TextButton::buttonColourId,  SF::kBg3);
    m_addBtn.setColour(juce::TextButton::textColourOffId, SF::kAccent);
    m_addBtn.addListener(this);
    addAndMakeVisible(m_addBtn);

    // Viewport for scrollable slot list
    m_viewport.setViewedComponent(&m_slotContainer, false);
    m_viewport.setScrollBarsShown(true, false);
    m_viewport.setScrollBarThickness(4);
    m_viewport.getVerticalScrollBar()
              .setColour(juce::ScrollBar::thumbColourId, SF::kBorder2);
    addAndMakeVisible(m_viewport);

    // Start with one empty slot
    addInsert();
}

InsertPanel::~InsertPanel()
{
    m_addBtn.removeListener(this);
}

void InsertPanel::paint(juce::Graphics& g)
{
    // Panel background
    g.setColour(SF::kBg);
    g.fillRect(getLocalBounds());

    // Left border line
    g.setColour(SF::kBorder);
    g.drawVerticalLine(0, 0.f, static_cast<float>(getHeight()));

    // Header
    const juce::Rectangle<int> header(0, 0, getWidth(), 34);
    g.setColour(SF::kBg2);
    g.fillRect(header);
    g.setColour(SF::kBorder);
    g.drawHorizontalLine(34, 0.f, static_cast<float>(getWidth()));

    g.setFont(SF::monoFont(9.f));
    g.setColour(SF::kMuted);
    g.drawText("INSERTS", header.reduced(12, 0), juce::Justification::centredLeft);

    g.setFont(SF::monoFont(8.f));
    g.drawText(juce::String(m_slots.size()) + " / " + juce::String(kMaxInserts),
               header.reduced(12, 0), juce::Justification::centredRight);
}

void InsertPanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(34);   // header

    m_addBtn.setBounds(area.removeFromBottom(36).reduced(8, 5));
    m_viewport.setBounds(area);
    layoutSlots();
}

void InsertPanel::layoutSlots()
{
    const int slotH = 56;
    const int gap   = 4;
    const int total = static_cast<int>(m_slots.size()) * (slotH + gap);
    const int w     = m_viewport.getWidth();

    m_slotContainer.setBounds(0, 0, w, std::max(total, m_viewport.getHeight()));

    int y = gap;
    for (auto& slot : m_slots)
    {
        slot->setBounds(4, y, w - 8, slotH);
        y += slotH + gap;
    }
}

void InsertPanel::buttonClicked(juce::Button* b)
{
    if (b == &m_addBtn) addInsert();
}

void InsertPanel::addInsert()
{
    if (static_cast<int>(m_slots.size()) >= kMaxInserts) return;

    const int idx = static_cast<int>(m_slots.size());

    InsertSlot::State s;
    s.sourceName = "— empty —";
    s.colour     = SF::kPalette[idx % 12];
    s.enabled    = true;

    auto slot = std::make_unique<InsertSlot>(idx, s);

    slot->onRemove = [this](int i) { removeInsert(i); };

    slot->onToggle = [this](int i, bool en)
    {
        if (i < static_cast<int>(m_slots.size()))
        {
            auto st = m_slots[i]->getState();
            st.enabled = en;
            m_slots[i]->setState(st);
            if (onEnabledChanged) onEnabledChanged(i, en);
        }
    };

    slot->onColourChanged = [this](int i, juce::Colour c)
    {
        if (onColourChanged) onColourChanged(i, c);
    };

    slot->onNameChanged = [](int, juce::String) {};

    m_slotContainer.addAndMakeVisible(*slot);
    m_slots.push_back(std::move(slot));

    layoutSlots();
    repaint();

    // Scroll to bottom so new slot is visible
    m_viewport.setViewPositionProportionately(0.0, 1.0);
}

void InsertPanel::removeInsert(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(m_slots.size())) return;

    m_slotContainer.removeChildComponent(m_slots[idx].get());
    m_slots.erase(m_slots.begin() + idx);

    // Re-index remaining slots
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        auto st = m_slots[i]->getState();
        m_slots[i]->setState(st);   // triggers repaint with new index visuals
    }

    layoutSlots();
    repaint();

    if (onEnabledChanged) onEnabledChanged(idx, false);
}

void InsertPanel::setChannelLevel(int ch, float level)
{
    if (ch >= 0 && ch < static_cast<int>(m_slots.size()))
        m_slots[ch]->setLevel(level);
}

void InsertPanel::setChannelColour(int ch, juce::Colour c)
{
    if (ch >= 0 && ch < static_cast<int>(m_slots.size()))
    {
        auto st = m_slots[ch]->getState();
        st.colour = c;
        m_slots[ch]->setState(st);
    }
}

void InsertPanel::setChannelEnabled(int ch, bool e)
{
    if (ch >= 0 && ch < static_cast<int>(m_slots.size()))
    {
        auto st = m_slots[ch]->getState();
        st.enabled = e;
        m_slots[ch]->setState(st);
    }
}
