#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "melodee/Melodee.h"

class MelodeeProcessor;

/** The parsed progression as chips (chord name + roman numeral). Click a chip to hear it. */
class ChordStrip : public juce::Component
{
public:
    explicit ChordStrip (MelodeeProcessor& processor);

    void refresh();
    void setPlayhead (double beat);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> chipBounds (size_t index) const;

    MelodeeProcessor& processor;
    melodee::Progression progression;
    melodee::KeyContext key;
    std::vector<std::vector<int>> voicings;
    double playhead = -1.0;
    int highlighted = -1;
};

/** Buttons for the chords that belong to the current scale. Click to add one to the progression. */
class ScaleChordsBar : public juce::Component
{
public:
    explicit ScaleChordsBar (MelodeeProcessor& processor);

    void refresh();
    void resized() override;

private:
    MelodeeProcessor& processor;
    juce::ToggleButton seventhsToggle { "7ths" };
    juce::OwnedArray<juce::TextButton> buttons;
    std::vector<melodee::Chord> chords;
};
