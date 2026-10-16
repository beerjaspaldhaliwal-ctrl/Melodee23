#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** A set of pitch classes stored as a 12-bit mask (bit 0 = C, bit 11 = B). */
using PitchMask = std::uint16_t;

inline int mod12 (int x) noexcept { return ((x % 12) + 12) % 12; }

inline PitchMask pcBit (int pc) noexcept { return static_cast<PitchMask> (1u << mod12 (pc)); }

inline bool maskHas (PitchMask mask, int pc) noexcept { return (mask & pcBit (pc)) != 0; }

int maskCount (PitchMask mask) noexcept;

/** Transposes every pitch class in a mask up by `semitones`. */
PitchMask maskTranspose (PitchMask mask, int semitones) noexcept;

/** Parses a note name at the start of `text` ("C", "F#", "Bb", "E♭", "Cb", "B#"...).
    Returns the pitch class 0-11, or -1. `consumed` receives the number of bytes used,
    and `wasFlat` whether the spelling used a flat. */
int parseNoteName (std::string_view text, size_t& consumed, bool* wasFlat = nullptr);

/** Name of a pitch class using sharps or flats. */
std::string pitchClassName (int pc, bool preferFlats);

/** Name of a MIDI note with octave, using the convention C4 = 60 (middle C). */
std::string midiNoteName (int midiNote, bool preferFlats);

/** Whether a key is conventionally written with flats (F, Bb, Eb, Ab, Db major; D, G, C, F, Bb, Eb minor). */
bool keyUsesFlats (int tonicPc, bool minor) noexcept;

/** Replaces common music Unicode symbols (♭ ♯ ♮ ° ø Δ – —) with ASCII equivalents. */
std::string normaliseMusicText (std::string_view text);

std::string toLower (std::string_view text);
std::string trim (std::string_view text);
bool startsWith (std::string_view text, std::string_view prefix) noexcept;
bool equalsIgnoreCase (std::string_view a, std::string_view b) noexcept;

/** A key: tonic pitch class plus whether roman numerals are read in a minor context. */
struct KeyContext
{
    int tonic = 0;
    bool minor = false;
    bool preferFlats = false;
};

} // namespace melodee
