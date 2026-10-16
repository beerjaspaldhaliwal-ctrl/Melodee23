#pragma once

#include "melodee/Theory.h"

#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** A named scale: a set of semitone offsets from the root, starting at 0. */
struct ScaleDef
{
    std::string name;
    std::string category;
    std::vector<int> intervals;   // ascending, first element 0
    std::string aliases;          // other names, comma separated
    std::string description;
    bool minorTonality = false;   // roman numerals are read in a minor context

    PitchMask mask() const noexcept;
};

/** Every scale Melodee knows, grouped by category in display order. */
const std::vector<ScaleDef>& allScales();

/** Category names in display order. */
const std::vector<std::string>& scaleCategories();

/** Finds a scale by name or alias (case-insensitive). Returns -1 if not found. */
int findScale (std::string_view nameOrAlias);

/** A scale placed on a root note. */
class Scale
{
public:
    Scale() = default;
    Scale (int rootPc, int scaleIndex);
    Scale (int rootPc, const ScaleDef& def);

    int root() const noexcept { return rootPc; }
    int index() const noexcept { return defIndex; }
    const ScaleDef& def() const;
    const std::vector<int>& intervals() const noexcept { return steps; }

    /** Absolute pitch classes in the scale. */
    PitchMask mask() const noexcept { return pcMask; }
    bool contains (int pitch) const noexcept { return maskHas (pcMask, pitch); }
    int size() const noexcept { return static_cast<int> (steps.size()); }
    bool isMinor() const noexcept { return minor; }

    KeyContext keyContext() const noexcept;

    /** Display name, e.g. "Eb Dorian". */
    std::string name (bool preferFlats) const;

    /** Note names of the scale, e.g. "C D E F G A B". */
    std::string noteNames (bool preferFlats) const;

    /** Moves `midiNote` by `steps` scale degrees (notes not in the scale snap first). */
    int stepFrom (int midiNote, int steps) const noexcept;

    /** Nearest MIDI note in the scale to `midiNote` (ties go down). */
    int snap (int midiNote) const noexcept;

    /** Degree index (0-based) of a pitch, or -1 if it is not in the scale. */
    int degreeOf (int pitch) const noexcept;

private:
    int rootPc = 0;
    int defIndex = 0;
    std::vector<int> steps { 0, 2, 4, 5, 7, 9, 11 };
    PitchMask pcMask = 0xAB5; // C major
    bool minor = false;
};

/** Steps a MIDI note `steps` notes along any pitch-class mask. */
int stepAlongMask (PitchMask mask, int midiNote, int steps) noexcept;

/** Nearest note to `midiNote` whose pitch class is in `mask`. Prefers `preferDirection` (-1/+1) on ties. */
int snapToMask (PitchMask mask, int midiNote, int preferDirection = -1) noexcept;

} // namespace melodee
