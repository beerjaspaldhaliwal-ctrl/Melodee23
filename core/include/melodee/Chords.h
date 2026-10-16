#pragma once

#include "melodee/Scales.h"
#include "melodee/Theory.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** A chord: root, optional slash bass, and its intervals above the root.
    Intervals of 12 or more are extensions (9 = 14, 11 = 17, 13 = 21). */
struct Chord
{
    int root = 0;
    int bass = -1;               // -1: the root is in the bass
    std::vector<int> intervals { 0, 4, 7 };
    std::string suffix;          // canonical suffix, e.g. "m7b5", "maj9", "7#9"
    bool flats = false;          // spell the root (and bass) with flats
    bool noChord = false;        // "N.C."

    std::string name() const;
    int bassPc() const noexcept { return bass >= 0 ? bass : root; }

    /** All chord tones as pitch classes. */
    PitchMask mask() const noexcept;

    /** Root, 3rd (or sus tone), 5th, 6th/7th: the tones a melody should land on. */
    PitchMask coreMask() const noexcept;

    /** 9ths, 11ths, 13ths and altered tones. */
    PitchMask extensionMask() const noexcept;

    bool hasInterval (int semitones) const noexcept;
    bool isMinor() const noexcept;      // has a minor 3rd and no major 3rd
    bool isDominant() const noexcept;   // major 3rd + minor 7th
    bool hasSeventh() const noexcept;

    bool operator== (const Chord& other) const noexcept
    {
        return root == other.root && bass == other.bass && intervals == other.intervals && noChord == other.noChord;
    }
};

/** Parses a chord suffix ("m7b5", "maj9", "7(#9)", "sus4", "add9"...).
    Returns the intervals and fills `canonicalSuffix`, or std::nullopt if not understood. */
std::optional<std::vector<int>> parseChordSuffix (std::string_view suffix, std::string& canonicalSuffix);

/** Parses a full chord symbol such as "C#m7", "Bbmaj9", "D/F#", "G7alt", "F#ø7", "N.C.". */
std::optional<Chord> parseChordSymbol (std::string_view symbol);

/** Builds a chord from a root and a suffix. */
std::optional<Chord> makeChord (int rootPc, std::string_view suffix, bool flats = false);

/** Names a set of intervals (relative pitch classes) using the most common chord suffix,
    or returns std::nullopt if the shape is not a recognised chord. */
std::optional<std::string> identifyChordSuffix (PitchMask relativeMask);

/** Roman numeral for a chord in a key, e.g. "ii7", "V7", "bVImaj7", "viiø7". */
std::string romanNumeralFor (const Chord& chord, const KeyContext& key);

/** Chords built by stacking every other scale note on each degree (triads or 7th chords). */
std::vector<Chord> diatonicChords (const Scale& scale, bool sevenths);

/** All chord suffixes Melodee offers in menus, in a sensible order. */
const std::vector<std::string>& commonChordSuffixes();

} // namespace melodee
