#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/** A small built-in synth so you can hear ideas without routing MIDI:
    channel 2 = soft electric piano (chords), channel 3 = bass, everything else = lead. */
class PreviewSynth
{
public:
    enum class Timbre { Lead, Keys, Bass };

    PreviewSynth();

    void prepare (double sampleRate, int maxBlockSize);
    void render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int numSamples);
    void allNotesOff();

private:
    juce::Synthesiser lead, keys, bass;
    juce::MidiBuffer leadMidi, keysMidi, bassMidi;
};
