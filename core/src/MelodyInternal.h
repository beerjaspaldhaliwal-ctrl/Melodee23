#pragma once

#include "melodee/Harmony.h"
#include "melodee/Melody.h"
#include "melodee/Random.h"

#include <cstdint>
#include <vector>

namespace melodee::detail
{

enum StyleFlag : std::uint32_t
{
    FlagArpeggio     = 1u << 0,
    FlagBebop        = 1u << 1,
    FlagBlueNotes    = 1u << 2,
    FlagOrnaments    = 1u << 3,
    FlagRuns         = 1u << 4,
    FlagPentatonic   = 1u << 5,
    FlagRolls        = 1u << 6,
    FlagOctaveJumps  = 1u << 7,
    FlagCounter      = 1u << 8,
    FlagSequence     = 1u << 9,
    FlagLongTones    = 1u << 10,
    FlagCallResponse = 1u << 11,
    FlagMotifDevelop = 1u << 12,
    FlagGraceNotes   = 1u << 13,
    FlagScalarRuns   = 1u << 14,
    FlagSwing16      = 1u << 15,
};

enum class Grid { Sixteenth, Eighth, Triplet, Mixed };

struct StyleProfile
{
    const char* name;
    const char* description;

    // Defaults for the 0..1 amounts.
    double density, syncopation, leaps, chordTones, repetition, rests, swing, noteLength;

    Grid grid;
    double repeatNote;     // liking for repeated pitches
    double extensions;     // liking for 9ths / 11ths / 13ths on strong beats
    int rangeSemis;        // preferred melodic span
    Contour contour;
    PhraseForm form;
    std::uint32_t flags;
    int velocity;
    std::vector<const char*> patterns; // 16-step bar rhythms for 4/4 ("x" = note)
};

const StyleProfile& styleProfile (MelodyStyle style);

/** Styles used, in order, for MelodyStyle::Mixed. */
const std::vector<MelodyStyle>& mixedStyleCycle();

struct Effective
{
    double density, syncopation, leaps, chordTones, repetition, rests, swing, articulation;
};

Effective effectiveSettings (const StyleProfile& p, const MelodySettings& s);

/** 1.0 on the downbeat, ~0.8 mid-bar, ~0.65 beats, ~0.42 off-beat 8ths, ~0.2 16ths. */
double metricStrength (double posInBar, int beatsPerBar) noexcept;

/** Index of the chord context at time t (never out of range when contexts is not empty). */
int contextIndexAt (const Progression& prog, double t) noexcept;

/** Moves a pitch so it fits the chord: chord tones on strong beats, chord-scale notes elsewhere. */
int fitPitchToChord (int pitch, const ChordContext& ctx, double strength, int preferDirection) noexcept;

/** Shifts off-beat 8ths (or 16ths) later by up to a triplet feel. */
void applySwing (std::vector<NoteEvent>& notes, double amount, bool sixteenths);

void applyHumanize (std::vector<NoteEvent>& notes, double amount, Rng& rng);

/** Sorts, trims overlaps (monophonic), keeps notes inside [lo, hi] and inside the progression. */
void finaliseMonophonic (std::vector<NoteEvent>& notes, double totalBeats, int lo, int hi);

/** Pentatonic subset of the key: major or minor pentatonic built on the tonic. */
PitchMask pentatonicOf (const Scale& key) noexcept;

/** Blue notes of the key: b3, b5, b7 above the tonic. */
PitchMask blueNotesOf (const Scale& key) noexcept;

/** The chord voicings' top voice for each chord (used by counter-melodies). */
std::vector<int> topVoices (const Progression& prog, int center);

/** What the embellishment passes need to know about the harmony. */
struct EmbellishContext
{
    const Progression& prog;
    const std::vector<ChordContext>& ctxs;
    int beatsPerBar;
    double total;
    int lo, hi;
    PitchMask pentaMask;

    const ChordContext& at (double t) const { return ctxs[static_cast<size_t> (contextIndexAt (prog, t))]; }
};

/** On-beat grace notes before long strong-beat notes (chromatic from below for blues). */
void addGraceNotes (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng, bool blues);

/** Trills, mordents and turns on long notes. */
void addOrnaments (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng, double amount);

/** Vocal-style runs into the next note at the end of long notes. */
void addRuns (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng);

/** Trap-style 16th-triplet / 32nd rolls. */
void addRolls (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng);

} // namespace melodee::detail
