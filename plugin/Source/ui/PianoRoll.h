#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "melodee/Melodee.h"

class MelodeeProcessor;

/** Shows the loop: chord regions, bass, chords and the selected melody, with a playhead. */
class PianoRoll : public juce::Component
{
public:
    explicit PianoRoll (MelodeeProcessor& processor);

    /** Pulls the current notes from the session. */
    void refresh();

    void setPlayhead (double beat);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> noteArea() const;
    float xForBeat (double beat) const;
    float yForPitch (int pitch) const;
    int pitchForY (float y) const;

    MelodeeProcessor& processor;

    std::vector<melodee::NoteEvent> melody, chords, bass;
    std::vector<melodee::ChordEvent> chordEvents;
    double total = 16.0;
    int beatsPerBar = 4;
    int lowPitch = 36, highPitch = 84;
    bool showMelody = true, showChords = true, showBass = true, flats = false;
    double playhead = -1.0;
    juce::String melodyName;
};
