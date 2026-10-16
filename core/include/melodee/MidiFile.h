#pragma once

#include "melodee/Melody.h"

#include <cstdint>
#include <string>
#include <vector>

namespace melodee
{

struct MidiTrack
{
    std::string name;
    int channel = 0;          // 0-15
    int program = -1;         // General MIDI program, -1 = none
    std::vector<NoteEvent> notes;
};

/** Writes a Standard MIDI File (type 1) with a tempo/time-signature track followed by `tracks`. */
std::vector<std::uint8_t> writeMidiFile (const std::vector<MidiTrack>& tracks, double bpm, int beatsPerBar, int ticksPerQuarter = 960);

/** Writes the file to disk. Returns false on failure. */
bool saveMidiFile (const std::string& path, const std::vector<MidiTrack>& tracks, double bpm, int beatsPerBar);

/** Minimal reader used by the tests: returns the notes of every track (times in beats). */
std::vector<MidiTrack> readMidiFile (const std::vector<std::uint8_t>& data);

} // namespace melodee
