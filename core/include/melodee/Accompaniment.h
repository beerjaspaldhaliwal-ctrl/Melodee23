#pragma once

#include "melodee/Melody.h"
#include "melodee/Progression.h"

#include <cstdint>
#include <string>
#include <vector>

namespace melodee
{

enum class VoicingStyle
{
    Close,      // root position, stacked within an octave
    VoiceLed,   // inversions chosen for the smoothest movement
    Open,       // spread across two octaves
    Drop2,      // jazz drop-2
    Drop3,      // jazz drop-3
    Shell,      // root + 3rd + 7th
    Rootless,   // 3-5-7-9 / 7-9-3-5 jazz piano
    Power,      // root + 5th + octave
    Spread,     // cinematic: low root, wide upper structure
    Lush,       // voice-led with an added 9th
    Quartal,    // stacked 4ths built from the chord scale
};

enum class ChordRhythm
{
    Sustain,        // one hit per chord
    BarHits,        // re-strike every bar
    HalfNotes,
    Quarters,
    Eighths,
    Offbeats,       // ska / house off-beat stabs
    PopSyncopation, // 3+3+2
    Charleston,
    Tresillo,       // dembow / reggaeton
    ArpUp,
    ArpUpDown,
    Alberti,
    Strum,          // guitar-like down/up strums
    PianoBallad,    // bass on 1, chord + arpeggiated fill
    Gospel,         // anticipated pushes
    HouseStabs,     // short off-beat stabs
    TranceGate,     // 16th-note gate
    Reggae,         // skank on 2 and 4
};

enum class BassStyle
{
    Off,
    Roots,          // sustained roots
    RootFifth,      // root and fifth halves
    Octaves,        // disco octave 8ths
    Walking,        // jazz walking quarters with approach notes
    Pumping,        // driving 8th-note roots
    Trap808,        // long 808 notes with syncopated re-hits
    Reggae,         // one-drop
    Funk,           // syncopated 16th funk
    Arpeggio,       // root-3rd-5th patterns
    Tresillo,       // 3+3+2 bass
};

struct AccompanimentSettings
{
    int beatsPerBar = 4;
    VoicingStyle voicing = VoicingStyle::VoiceLed;
    ChordRhythm rhythm = ChordRhythm::Sustain;
    int chordCenter = 60;     // voicings gather around this note
    BassStyle bass = BassStyle::Roots;
    int bassCenter = 38;      // around D2
    int velocity = 88;
    double swing = 0.0;       // 0..1
    double humanize = 0.1;    // 0..1
};

const std::vector<std::string>& voicingNames();     // indexed by VoicingStyle
const std::vector<std::string>& chordRhythmNames(); // indexed by ChordRhythm
const std::vector<std::string>& bassStyleNames();   // indexed by BassStyle

/** One voicing (MIDI notes, low to high) per chord. */
std::vector<std::vector<int>> voiceProgression (const Progression& progression, VoicingStyle style, int center);

std::vector<NoteEvent> renderChords (const Progression& progression, const AccompanimentSettings& settings, std::uint32_t seed);

std::vector<NoteEvent> renderBass (const Progression& progression, const AccompanimentSettings& settings, std::uint32_t seed);

} // namespace melodee
