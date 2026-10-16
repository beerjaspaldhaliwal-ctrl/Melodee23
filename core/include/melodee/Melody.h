#pragma once

#include "melodee/Progression.h"
#include "melodee/Scales.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** One note. Times are in quarter-note beats from the start of the progression. */
struct NoteEvent
{
    int pitch = 60;
    double start = 0.0;
    double length = 1.0;
    int velocity = 100;
};

struct Melody
{
    std::string name;
    std::string description;
    std::string style;
    std::uint32_t seed = 0;
    std::vector<NoteEvent> notes;
};

/** Melody-writing approaches. Each has its own rhythm vocabulary, contour, phrasing and tricks. */
enum class MelodyStyle
{
    Lyrical,            // singable top line: steps, chord tones on strong beats
    PopHook,            // catchy, syncopated, repetitive hook
    ChordTones,         // guide tones: 3rds and 7ths on the beat
    Arpeggio,           // broken-chord patterns
    ScalarRuns,         // flowing scale runs between chord tones
    MotifDevelopment,   // one idea: sequenced, inverted, reversed
    CallResponse,       // question and answer phrases
    Bebop,              // swung 8ths, enclosures, chromatic approach notes
    Blues,              // blue notes, slides, call and response
    Pentatonic,         // pentatonic riffs
    Trap,               // dark, repetitive, triplet rolls and octave jumps
    RnBNeoSoul,         // extensions, laid-back runs and melisma
    EDMLead,            // syncopated pluck / lead patterns
    LoFi,               // lazy swing, sparse, 7ths and 9ths
    Ballad,             // long expressive notes
    Classical,          // periodic phrases, sequences, cadences
    Latin,              // dembow / tresillo rhythms
    Afrobeats,          // syncopated, pentatonic, call and response
    RockRiff,           // riff-based, pentatonic and blues
    Cinematic,          // long, wide, rising
    Ambient,            // sparse floating tones
    CounterMelody,      // moves against the chords' top voice
    Ornamental,         // grace notes, trills, mordents, turns
    Mixed,              // a different style for every variation
};

enum class Contour { Auto, Arch, Valley, Rising, Falling, Wave, Flat };

enum class PhraseForm { Auto, Repeat, AABA, ABAB, AABB, ABAC, CallResponse, Through };

/** Generator settings. The 0..1 "amount" controls are relative to the chosen style:
    0.5 = the style's own default, lower/higher = less/more than the style normally uses. */
struct MelodySettings
{
    int beatsPerBar = 4;
    int lowNote = 60;           // C4
    int highNote = 84;          // C6

    double density = 0.5;       // notes per bar
    double syncopation = 0.5;   // off-beat emphasis
    double leaps = 0.5;         // interval size
    double chordTones = 0.5;    // how strongly strong beats land on chord tones
    double repetition = 0.5;    // motif reuse
    double rests = 0.5;         // space between notes
    double swing = 0.5;         // swing feel
    double noteLength = 0.5;    // staccato (0) .. legato (1)

    double humanize = 0.15;     // absolute 0..1: timing and velocity variation

    Contour contour = Contour::Auto;
    PhraseForm form = PhraseForm::Auto;
    bool endOnTonic = true;
};

const std::vector<std::string>& melodyStyleNames();       // indexed by MelodyStyle
std::string melodyStyleDescription (MelodyStyle style);
MelodyStyle melodyStyleFromName (std::string_view name);  // case-insensitive; Mixed if unknown
int numMelodyStyles() noexcept;                           // including Mixed

const std::vector<std::string>& contourNames();           // indexed by Contour
const std::vector<std::string>& phraseFormNames();        // indexed by PhraseForm

/** Generates one melody over the progression. The same seed always gives the same melody. */
Melody generateMelody (const Progression& progression, const Scale& key, const MelodySettings& settings,
                       MelodyStyle style, std::uint32_t seed);

/** Generates `count` distinct variations. With MelodyStyle::Mixed each variation uses a different style. */
std::vector<Melody> generateVariations (const Progression& progression, const Scale& key, const MelodySettings& settings,
                                        MelodyStyle style, int count, std::uint32_t baseSeed);

//==============================================================================
/** Ways to make a new variation out of an existing melody. */
enum class MelodyTransform
{
    NewRhythm,           // same notes, new rhythm
    NewPitches,          // same rhythm, new notes
    Inversion,           // mirror the intervals
    Retrograde,          // play it backwards
    RetrogradeInversion, // backwards and mirrored
    Embellish,           // add passing and neighbour tones
    Simplify,            // keep the strong-beat notes
    Ornament,            // grace notes, mordents, trills, turns
    Syncopate,           // push notes ahead of the beat
    DoubleTime,          // twice as fast, repeated to fill
    HalfTime,            // twice as slow
    OctaveUp,
    OctaveDown,
    Sequence,            // repeat the first bar's idea on every chord
    AnswerPhrase,        // keep the first half, write a resolving answer
    HarmonizeThirds,     // add a diatonic third above
    HarmonizeSixths,     // add a diatonic sixth below
    Swing,               // apply a swing feel
    Humanize,            // timing and velocity feel
    FitToChords,         // re-fit the notes to the current chords (after changing chords)
};

const std::vector<std::string>& transformNames();          // indexed by MelodyTransform
std::string transformDescription (MelodyTransform transform);
int numTransforms() noexcept;

Melody transformMelody (const Melody& melody, MelodyTransform transform, const Progression& progression,
                        const Scale& key, const MelodySettings& settings, std::uint32_t seed);

} // namespace melodee
