#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <vector>

/** One MIDI event in the looping pattern. Beats are quarter notes from the loop start. */
struct PatternEvent
{
    double beat = 0.0;
    int channel = 1;     // 1-16
    int note = 60;
    int velocity = 100;
    bool on = true;
};

/** Everything the audio thread needs: the looping events plus data for the keyboard modes. */
struct Pattern
{
    std::vector<PatternEvent> events;            // sorted by beat, note-offs first
    double length = 0.0;                         // loop length in beats
    std::vector<std::vector<int>> chordVoicings; // for chord-trigger mode
    juce::uint16 scaleMask = 0x0fff;             // for scale-lock mode
};

/** Plays a Pattern in sync with the host (or a free-running preview clock) and
    handles the keyboard modes. All process* calls are real-time safe. */
class Sequencer
{
public:
    Sequencer();

    /** Message thread: swaps in a new pattern. The old one is destroyed on this thread. */
    void setPattern (Pattern&& newPattern);

    void prepare (double sampleRate);

    /** Audio thread. Adds pattern events for this block to `out`. */
    void process (juce::MidiBuffer& out, int numSamples, const juce::Optional<juce::AudioPlayHead::PositionInfo>& position,
                  bool syncToHost, bool preview, double previewBpm);

    /** Audio thread. Applies the keyboard mode to incoming notes. */
    void processKeyboard (const juce::MidiBuffer& in, juce::MidiBuffer& out, int mode);

    /** Audio thread. Note-offs for everything currently sounding. */
    void allNotesOff (juce::MidiBuffer& out, int sampleOffset);

    /** Position inside the loop for the UI, or -1 when stopped. */
    double getCurrentBeat() const noexcept { return uiBeat.load(); }
    double getLoopLength() const noexcept { return uiLength.load(); }
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }
    double getHostBpm() const noexcept { return hostBpm.load(); }

    /** Message thread: restart the preview clock from the top. */
    void restartPreview() noexcept { restartRequested = true; }

private:
    void render (juce::MidiBuffer& out, double startBeat, double endBeat, double beatsPerSample, int numSamples);
    void noteOn (juce::MidiBuffer& out, int channel, int note, int velocity, int sample);
    void noteOff (juce::MidiBuffer& out, int channel, int note, int sample);

    juce::SpinLock lock;
    Pattern pattern;
    bool patternChanged = false;

    double sampleRate = 44100.0;
    double internalBeat = 0.0;
    double expectedBeat = 0.0;
    bool wasRunning = false;
    std::atomic<bool> restartRequested { false };

    std::array<std::array<bool, 128>, 16> active {};

    // keyboard modes: what each incoming note turned into
    std::array<std::array<int, 8>, 128> keyMap {};
    std::array<int, 128> keyMapSize {};
    std::array<int, 128> keyMapChannel {};

    std::atomic<double> uiBeat { -1.0 };
    std::atomic<double> uiLength { 0.0 };
    std::atomic<bool> hostPlaying { false };
    std::atomic<double> hostBpm { 120.0 };
};
