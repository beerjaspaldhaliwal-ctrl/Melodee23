#include "MelodeeLookAndFeel.h"

MelodeeLookAndFeel::MelodeeLookAndFeel()
{
    setColourScheme ({ Colours::background, Colours::panel, Colours::panelLight, Colours::outline, Colours::text,
                       Colours::melody, juce::Colours::white, Colours::melody.withAlpha (0.35f), Colours::text });

    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::TextButton::buttonColourId, Colours::panelLight);
    setColour (juce::TextButton::buttonOnColourId, Colours::melody);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, Colours::panelLight);
    setColour (juce::ComboBox::outlineColourId, Colours::outline);
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::arrowColourId, Colours::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panelLight);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::melody.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, Colours::panelLight);
    setColour (juce::TextEditor::outlineColourId, Colours::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, Colours::melody);
    setColour (juce::TextEditor::textColourId, Colours::text);
    setColour (juce::TextEditor::highlightColourId, Colours::melody.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Colours::melody);
    setColour (juce::ListBox::backgroundColourId, Colours::panel);
    setColour (juce::ListBox::outlineColourId, Colours::outline);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::Slider::thumbColourId, Colours::melody);
    setColour (juce::Slider::trackColourId, Colours::melody.withAlpha (0.6f));
    setColour (juce::Slider::backgroundColourId, Colours::panelLight);
    setColour (juce::Slider::rotarySliderFillColourId, Colours::melody);
    setColour (juce::Slider::rotarySliderOutlineColourId, Colours::panelLight);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, Colours::outline);
    setColour (juce::TooltipWindow::backgroundColourId, Colours::panelLight);
    setColour (juce::TooltipWindow::textColourId, Colours::text);
    setColour (juce::TooltipWindow::outlineColourId, Colours::outline);
    setColour (juce::ToggleButton::textColourId, Colours::text);
    setColour (juce::ToggleButton::tickColourId, Colours::melody);
}

void MelodeeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (2.5f, radius * 0.16f);
    const float arcRadius = radius - lineW * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto fill = slider.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::panelLight);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Bipolar knobs (0.5 = style default) fill from the top.
    const bool bipolar = slider.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (fill);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float dotR = lineW * 0.75f;
    const juce::Point<float> dot (centre.x + (arcRadius - lineW * 1.6f) * std::sin (angle),
                                  centre.y - (arcRadius - lineW * 1.6f) * std::cos (angle));
    g.setColour (Colours::text);
    g.fillEllipse (juce::Rectangle<float> (dotR * 2.0f, dotR * 2.0f).withCentre (dot));
}

void MelodeeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                               bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto base = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : backgroundColour;

    if (isButtonDown)
        base = base.brighter (0.15f);
    else if (isMouseOver)
        base = base.brighter (0.08f);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (Colours::outline.withAlpha (button.getToggleState() ? 0.0f : 1.0f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void MelodeeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isMouseOver, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float box = juce::jmin (16.0f, bounds.getHeight() - 4.0f);
    auto tick = juce::Rectangle<float> (box, box).withCentre ({ bounds.getX() + box * 0.5f + 2.0f, bounds.getCentreY() });

    const auto accent = button.findColour (juce::ToggleButton::tickColourId);
    g.setColour (button.getToggleState() ? accent : Colours::panelLight.brighter (isMouseOver ? 0.1f : 0.0f));
    g.fillRoundedRectangle (tick, 4.0f);
    g.setColour (Colours::outline);
    g.drawRoundedRectangle (tick, 4.0f, 1.0f);

    if (button.getToggleState())
    {
        juce::Path p;
        p.startNewSubPath (tick.getX() + box * 0.25f, tick.getCentreY());
        p.lineTo (tick.getX() + box * 0.45f, tick.getBottom() - box * 0.28f);
        p.lineTo (tick.getRight() - box * 0.22f, tick.getY() + box * 0.28f);
        g.setColour (juce::Colours::white);
        g.strokePath (p, juce::PathStrokeType (2.0f));
    }

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (juce::FontOptions (13.5f));
    g.drawFittedText (button.getButtonText(), bounds.withTrimmedLeft (box + 8.0f).toNearestInt(), juce::Justification::centredLeft, 1);
}

void MelodeeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Colours::melody : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    juce::Path arrow;
    const float ax = static_cast<float> (width) - 14.0f;
    const float ay = static_cast<float> (height) * 0.5f;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.strokePath (arrow, juce::PathStrokeType (1.6f));
}

juce::Font MelodeeLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::FontOptions (13.5f); }

juce::Font MelodeeLookAndFeel::getPopupMenuFont() { return juce::FontOptions (14.0f); }

juce::Font MelodeeLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions (juce::jmin (15.0f, static_cast<float> (buttonHeight) * 0.5f), juce::Font::bold);
}

juce::Label* MelodeeLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (juce::FontOptions (12.5f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
SectionLabel::SectionLabel (const juce::String& text)
{
    setText (text.toUpperCase(), juce::dontSendNotification);
    setFont (juce::FontOptions (11.5f, juce::Font::bold));
    setColour (juce::Label::textColourId, Colours::textDim);
    setJustificationType (juce::Justification::centredLeft);
    setMinimumHorizontalScale (0.7f);
}

//==============================================================================
Knob::Knob (const juce::String& caption, const juce::String& tooltip)
{
    slider.setRange (0.0, 1.0, 0.01);
    slider.setDoubleClickReturnValue (true, 0.5);
    slider.setTooltip (tooltip);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    addAndMakeVisible (slider);

    label.setText (caption, juce::dontSendNotification);
    label.setFont (juce::FontOptions (11.5f));
    label.setColour (juce::Label::textColourId, Colours::textDim);
    label.setJustificationType (juce::Justification::centred);
    label.setMinimumHorizontalScale (0.6f);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (15));
    slider.setBounds (r);
}
