#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Colours
{
    inline const juce::Colour background { 0xff121218 };
    inline const juce::Colour panel { 0xff1a1a23 };
    inline const juce::Colour panelLight { 0xff242431 };
    inline const juce::Colour outline { 0xff30303f };
    inline const juce::Colour text { 0xffeae9f3 };
    inline const juce::Colour textDim { 0xff9897ad };
    inline const juce::Colour melody { 0xff9b8cff };   // violet
    inline const juce::Colour chords { 0xff3fd2c0 };   // teal
    inline const juce::Colour bass { 0xfff3a85e };     // orange
    inline const juce::Colour warning { 0xffff7a7a };
}

class MelodeeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MelodeeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOver, bool isButtonDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isMouseOver, bool isButtonDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

/** A small section heading. */
class SectionLabel : public juce::Label
{
public:
    explicit SectionLabel (const juce::String& text);
};

/** A knob with a caption underneath. */
class Knob : public juce::Component
{
public:
    Knob (const juce::String& caption, const juce::String& tooltip);

    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    void resized() override;

private:
    juce::Label label;
};
