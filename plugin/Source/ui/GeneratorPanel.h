#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MelodeeLookAndFeel.h"

class MelodeeProcessor;

/** Melody style, the shaping knobs, Generate, the list of variations and "vary this". */
class GeneratorPanel : public juce::Component
{
public:
    explicit GeneratorPanel (MelodeeProcessor& processor);
    ~GeneratorPanel() override;

    void refresh();
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    class VariationModel;

    void bindKnob (Knob& knob, const juce::Identifier& id);
    void updateRangeLabel();

    MelodeeProcessor& processor;

    SectionLabel styleTitle { "Melody style" };
    juce::ComboBox styleBox;
    juce::Label styleDescription;

    Knob density { "Density", "How many notes per bar (centre = the style's normal amount)" };
    Knob syncopation { "Syncopation", "Off-beat, pushed rhythms" };
    Knob leaps { "Leaps", "Small steps (left) or big jumps (right)" };
    Knob chordTones { "Chord Tones", "How strongly strong beats land on chord notes" };
    Knob repetition { "Repetition", "How often the motif comes back (hooks love repetition)" };
    Knob rests { "Space", "Rests and short notes between phrases" };
    Knob swing { "Swing", "Swing / shuffle feel" };
    Knob noteLength { "Length", "Staccato (left) to legato (right)" };
    Knob humanize { "Humanize", "Timing and velocity variation" };

    juce::ComboBox contourBox, formBox;
    juce::Label rangeLabel;
    juce::Slider rangeSlider { juce::Slider::TwoValueHorizontal, juce::Slider::NoTextBox };

    juce::Label countLabel;
    juce::Slider countSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label seedLabel;
    juce::Slider seedSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::TextButton diceButton { "Dice" };
    juce::ToggleButton tonicToggle { "End on tonic" };

    juce::TextButton generateButton { "GENERATE" };
    juce::TextButton moreButton { "+ More" };

    SectionLabel variationsTitle { "Variations" };
    juce::ListBox variationList;
    std::unique_ptr<VariationModel> variationModel;

    juce::ComboBox transformBox;
    juce::TextButton varyButton { "Vary" };
    juce::TextButton deleteButton { "Delete" };
    juce::TextButton clearButton { "Clear" };

    bool refreshing = false;
};
