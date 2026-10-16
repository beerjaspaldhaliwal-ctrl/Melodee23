#pragma once

#include "melodee/Chords.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** A chord placed in time. Times are in quarter-note beats. */
struct ChordEvent
{
    Chord chord;
    double start = 0.0;
    double length = 4.0;
    std::string token;   // what the user typed for this chord
};

struct Progression
{
    std::vector<ChordEvent> chords;
    double totalBeats = 0.0;
    int beatsPerBar = 4;

    bool empty() const noexcept { return chords.empty(); }

    /** Index of the chord sounding at `beat`, or -1. */
    int indexAt (double beat) const noexcept;

    const ChordEvent* at (double beat) const noexcept;

    int numBars() const noexcept;

    /** Chord names separated by spaces, with ":bars" when a chord is not one bar long. */
    std::string toChordNames() const;

    /** Roman numerals in `key`, separated by spaces. */
    std::string toRomanNumerals (const KeyContext& key) const;

    /** Builds a progression from a list of chords, each lasting `barsEach` bars. */
    static Progression fromChords (const std::vector<Chord>& chords, int beatsPerBar = 4, double barsEach = 1.0);
};

struct ParseResult
{
    Progression progression;
    std::vector<std::string> errors;

    bool ok() const noexcept { return errors.empty() && ! progression.empty(); }
};

/** Parses a progression typed as chord symbols or roman numerals, e.g.
      "Am F C G"                 one bar each
      "vi IV I V"                roman numerals in the given key
      "ii7-V7-Imaj7"             dashes work as separators
      "| Dm7 G7 | Cmaj7 |"       bars: chords in a bar share it equally
      "Am:2 F:2 C G"             ":n" = n bars (inside |bars| it is a relative weight)
      "% "                       repeats the previous chord (or bar)
      "Am F C G x2"              repeats everything before it
      "V/V", "V7/vi"             secondary chords;  "I/3", "V/7" = inversion (bass on that scale degree)
*/
ParseResult parseProgression (std::string_view text, const KeyContext& key, int beatsPerBar = 4);

/** Parses one roman numeral such as "bVImaj7", "viiø7", "V7/V", "I/3", "#iv°7". */
std::optional<Chord> parseRomanNumeral (std::string_view token, const KeyContext& key);

/** Parses one chord token: roman numeral or chord symbol. */
std::optional<Chord> parseChordToken (std::string_view token, const KeyContext& key);

} // namespace melodee
