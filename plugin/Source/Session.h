#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "melodee/Melodee.h"

namespace IDs
{
#define MELODEE_ID(name) inline const juce::Identifier name (#name);
    MELODEE_ID (MelodeeState)
    MELODEE_ID (Variations)
    MELODEE_ID (Variation)

    // Harmony
    MELODEE_ID (key)
    MELODEE_ID (scale)
    MELODEE_ID (progression)
    MELODEE_ID (beatsPerBar)
    MELODEE_ID (loops)
    MELODEE_ID (genre)
    MELODEE_ID (libraryIndex)
    MELODEE_ID (sevenths)

    // Melody generator
    MELODEE_ID (style)
    MELODEE_ID (density)
    MELODEE_ID (syncopation)
    MELODEE_ID (leaps)
    MELODEE_ID (chordTones)
    MELODEE_ID (repetition)
    MELODEE_ID (rests)
    MELODEE_ID (swing)
    MELODEE_ID (noteLength)
    MELODEE_ID (humanize)
    MELODEE_ID (contour)
    MELODEE_ID (form)
    MELODEE_ID (lowNote)
    MELODEE_ID (highNote)
    MELODEE_ID (endOnTonic)
    MELODEE_ID (variationCount)
    MELODEE_ID (seed)
    MELODEE_ID (transform)
    MELODEE_ID (selected)

    // Accompaniment
    MELODEE_ID (voicing)
    MELODEE_ID (chordRhythm)
    MELODEE_ID (chordCenter)
    MELODEE_ID (bass)
    MELODEE_ID (bassCenter)

    // Playback / output
    MELODEE_ID (playMelody)
    MELODEE_ID (playChords)
    MELODEE_ID (playBass)
    MELODEE_ID (soundOn)
    MELODEE_ID (syncToHost)
    MELODEE_ID (keyboardMode)
    MELODEE_ID (outputMode)
    MELODEE_ID (previewBpm)

    // Variation children
    MELODEE_ID (name)
    MELODEE_ID (description)
    MELODEE_ID (styleName)
    MELODEE_ID (notes)
#undef MELODEE_ID
}

/** Everything the user has set up: key, scale, progression, generator settings and the
    generated variations. Lives on the message thread; the processor turns it into a
    playback pattern whenever it changes. */
class Session : public juce::ChangeBroadcaster
{
public:
    enum Part
    {
        MelodyPart = 1,
        ChordsPart = 2,
        BassPart = 4,
        AllParts = MelodyPart | ChordsPart | BassPart
    };

    enum KeyboardMode
    {
        KeyboardThru = 0,
        KeyboardScaleLock,
        KeyboardChordTrigger
    };

    Session();

    //==========================================================================
    juce::var get (const juce::Identifier& id) const;
    void set (const juce::Identifier& id, const juce::var& value);

    int getInt (const juce::Identifier& id) const { return static_cast<int> (get (id)); }
    double getDouble (const juce::Identifier& id) const { return static_cast<double> (get (id)); }
    bool getBool (const juce::Identifier& id) const { return static_cast<bool> (get (id)); }

    //==========================================================================
    melodee::Scale scale() const;
    bool prefersFlats() const;

    const melodee::ParseResult& parsed() const noexcept { return parseResult; }
    const melodee::Progression& baseProgression() const noexcept { return parseResult.progression; }

    /** The progression repeated by the "loops" setting: what melodies are written over. */
    const melodee::Progression& progression() const noexcept { return loopedProgression; }

    melodee::MelodySettings melodySettings() const;
    melodee::AccompanimentSettings accompanimentSettings() const;
    melodee::MelodyStyle style() const;

    //==========================================================================
    const std::vector<melodee::Melody>& variations() const noexcept { return melodies; }
    int selectedIndex() const;
    void select (int index);
    const melodee::Melody* selectedMelody() const;

    void generate (bool append);
    void applyTransform (int transformIndex);
    void removeSelected();
    void clearVariations();
    void randomiseSeed();

    void loadLibraryProgression (int libraryIndex);
    void appendChord (const juce::String& token);

    //==========================================================================
    std::vector<melodee::NoteEvent> chordNotes() const;
    std::vector<melodee::NoteEvent> bassNotes() const;
    std::vector<std::vector<int>> chordVoicings() const;

    juce::MemoryBlock createMidi (int parts, double bpm) const;
    juce::String suggestedFileName (int parts) const;

    /** A short hint shown in the UI, e.g. after the chords changed. */
    juce::String getStatus() const { return status; }

    //==========================================================================
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement& xml);

    juce::CriticalSection& getLock() noexcept { return lock; }

private:
    void reparse();
    void transposeVariations (int semitones);
    void setStatus (const juce::String& s) { status = s; }

    juce::ValueTree state { IDs::MelodeeState };
    melodee::ParseResult parseResult;
    melodee::Progression loopedProgression;
    std::vector<melodee::Melody> melodies;
    juce::String status;
    mutable juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Session)
};
