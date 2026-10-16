#pragma once

#include "melodee/Progression.h"
#include "melodee/Scales.h"

#include <vector>

namespace melodee
{

/** What notes work over one chord, given the key. */
struct ChordContext
{
    int root = 0;
    PitchMask chordTones = 0;   // every chord tone
    PitchMask core = 0;         // root, 3rd/sus, 5th, 6th/7th
    PitchMask stable = 0;       // root, 3rd, 5th
    PitchMask colour = 0;       // 9ths, 11ths, 13ths available over the chord
    PitchMask chordScale = 0;   // the scale to use over this chord
    PitchMask avoid = 0;        // scale notes a half step above a chord tone (weak on strong beats)
};

/** The 7-note scale (mode of major, melodic minor, harmonic minor/major, or a symmetric scale)
    that contains every chord tone and shares the most notes with the key. 0 if none fits. */
PitchMask bestFitChordScale (PitchMask chordMask, PitchMask keyMask, int chordRoot);

/** Analyses one chord against the key. `keepKeyNotes` keeps every key note (used for blues,
    where the minor 3rd over a major chord is the point). */
ChordContext analyseChord (const Chord& chord, const Scale& key, bool keepKeyNotes = false);

std::vector<ChordContext> analyseProgression (const Progression& progression, const Scale& key, bool keepKeyNotes = false);

/** Repeats a progression `times` times end to end. */
Progression repeatProgression (const Progression& progression, int times);

/** Number of `mask` notes between two MIDI notes (signed, positive when b is above a). */
int stepsBetween (PitchMask mask, int a, int b) noexcept;

} // namespace melodee
