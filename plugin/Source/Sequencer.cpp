#include "Sequencer.h"

#include "melodee/Scales.h"

#include <cmath>

Sequencer::Sequencer()
{
    keyMapSize.fill (0);
    keyMapChannel.fill (1);
}

void Sequencer::setPattern (Pattern&& newPattern)
{
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        std::swap (pattern, newPattern);
        patternChanged = true;
        uiLength = pattern.length;
    }
    // newPattern now holds the old data and is freed here, off the audio thread.
}

void Sequencer::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    wasRunning = false;
    for (auto& ch : active)
        ch.fill (false);
}

void Sequencer::noteOn (juce::MidiBuffer& out, int channel, int note, int velocity, int sample)
{
    channel = juce::jlimit (1, 16, channel);
    note = juce::jlimit (0, 127, note);
    auto& isOn = active[static_cast<size_t> (channel - 1)][static_cast<size_t> (note)];
    if (isOn)
        out.addEvent (juce::MidiMessage::noteOff (channel, note), sample);
    out.addEvent (juce::MidiMessage::noteOn (channel, note, static_cast<juce::uint8> (juce::jlimit (1, 127, velocity))), sample);
    isOn = true;
}

void Sequencer::noteOff (juce::MidiBuffer& out, int channel, int note, int sample)
{
    channel = juce::jlimit (1, 16, channel);
    note = juce::jlimit (0, 127, note);
    auto& isOn = active[static_cast<size_t> (channel - 1)][static_cast<size_t> (note)];
    if (! isOn)
        return;
    out.addEvent (juce::MidiMessage::noteOff (channel, note), sample);
    isOn = false;
}

void Sequencer::allNotesOff (juce::MidiBuffer& out, int sampleOffset)
{
    for (int ch = 0; ch < 16; ++ch)
        for (int n = 0; n < 128; ++n)
            if (active[static_cast<size_t> (ch)][static_cast<size_t> (n)])
                noteOff (out, ch + 1, n, sampleOffset);
}

void Sequencer::process (juce::MidiBuffer& out, int numSamples, const juce::Optional<juce::AudioPlayHead::PositionInfo>& position,
                         bool syncToHost, bool preview, double previewBpm)
{
    const juce::SpinLock::ScopedTryLockType tl (lock);
    if (! tl.isLocked())
        return;

    if (patternChanged)
    {
        allNotesOff (out, 0);
        patternChanged = false;
    }

    const bool playing = position.hasValue() && position->getIsPlaying();
    hostPlaying = playing;

    double bpm = previewBpm;
    if (position.hasValue())
        if (auto b = position->getBpm())
            if (*b > 1.0)
                bpm = *b;
    hostBpm = bpm;

    double startBeat = 0.0;
    bool running = false;

    if (syncToHost && playing && position->getPpqPosition().hasValue())
    {
        startBeat = *position->getPpqPosition();
        running = true;
    }
    else if (preview)
    {
        if (restartRequested.exchange (false) || ! wasRunning)
            internalBeat = 0.0;
        startBeat = internalBeat;
        running = true;
    }

    if (! running || pattern.length <= 0.0)
    {
        if (wasRunning)
            allNotesOff (out, 0);
        wasRunning = false;
        uiBeat = -1.0;
        return;
    }

    const double beatsPerSample = bpm / (60.0 * sampleRate);
    const double endBeat = startBeat + numSamples * beatsPerSample;

    // The host jumped (loop, scrub, restart): silence what is sounding.
    if (wasRunning && std::abs (startBeat - expectedBeat) > 0.02)
        allNotesOff (out, 0);

    render (out, startBeat, endBeat, beatsPerSample, numSamples);

    expectedBeat = endBeat;
    wasRunning = true;
    internalBeat = endBeat;

    const double loopPos = std::fmod (endBeat, pattern.length);
    uiBeat = loopPos < 0.0 ? loopPos + pattern.length : loopPos;
}

void Sequencer::render (juce::MidiBuffer& out, double startBeat, double endBeat, double beatsPerSample, int numSamples)
{
    const double length = pattern.length;
    const auto& events = pattern.events;
    if (events.empty())
        return;

    double seg = startBeat;
    int guard = 0;

    while (seg < endBeat - 1e-12 && guard++ < 64)
    {
        const double base = std::floor (seg / length) * length;
        const double segEnd = std::min (endBeat, base + length);
        const double from = seg - base;
        const double to = segEnd - base;

        auto it = std::lower_bound (events.begin(), events.end(), from,
                                    [] (const PatternEvent& e, double b) { return e.beat < b; });

        for (; it != events.end() && it->beat < to; ++it)
        {
            const int sample = juce::jlimit (0, numSamples - 1, static_cast<int> ((base + it->beat - startBeat) / beatsPerSample));
            if (it->on)
                noteOn (out, it->channel, it->note, it->velocity, sample);
            else
                noteOff (out, it->channel, it->note, sample);
        }

        seg = segEnd;
    }
}

void Sequencer::processKeyboard (const juce::MidiBuffer& in, juce::MidiBuffer& out, int mode)
{
    const juce::SpinLock::ScopedTryLockType tl (lock);

    for (const auto metadata : in)
    {
        const auto msg = metadata.getMessage();
        const int pos = metadata.samplePosition;

        if (! msg.isNoteOnOrOff() || mode == 0 || ! tl.isLocked())
        {
            out.addEvent (msg, pos);
            continue;
        }

        const int key = msg.getNoteNumber();

        if (msg.isNoteOn())
        {
            auto& mapped = keyMap[static_cast<size_t> (key)];
            int& count = keyMapSize[static_cast<size_t> (key)];

            // Release anything this key was already holding.
            for (int i = 0; i < count; ++i)
                out.addEvent (juce::MidiMessage::noteOff (keyMapChannel[static_cast<size_t> (key)], mapped[static_cast<size_t> (i)]), pos);
            count = 0;

            if (mode == 1)
            {
                const int snapped = juce::jlimit (0, 127, melodee::snapToMask (pattern.scaleMask, key));
                mapped[0] = snapped;
                count = 1;
                keyMapChannel[static_cast<size_t> (key)] = msg.getChannel();
                out.addEvent (juce::MidiMessage::noteOn (msg.getChannel(), snapped, msg.getVelocity()), pos);
            }
            else if (mode == 2 && ! pattern.chordVoicings.empty())
            {
                // White keys pick chords: C = chord 1, D = chord 2, ... and keep counting up the keyboard.
                static constexpr int whiteIndex[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
                const int pc = key % 12;
                int white = whiteIndex[pc];
                if (white < 0)
                    white = whiteIndex[(pc + 11) % 12]; // black keys repeat the chord below
                const int index = (white + 7 * (key / 12)) % static_cast<int> (pattern.chordVoicings.size());
                const auto& voicing = pattern.chordVoicings[static_cast<size_t> (index)];

                keyMapChannel[static_cast<size_t> (key)] = 2;
                for (int n : voicing)
                {
                    if (count >= 8)
                        break;
                    mapped[static_cast<size_t> (count++)] = n;
                    out.addEvent (juce::MidiMessage::noteOn (2, juce::jlimit (0, 127, n), msg.getVelocity()), pos);
                }
            }
            else
            {
                out.addEvent (msg, pos);
            }
        }
        else
        {
            auto& mapped = keyMap[static_cast<size_t> (key)];
            int& count = keyMapSize[static_cast<size_t> (key)];
            if (count == 0)
            {
                out.addEvent (msg, pos);
                continue;
            }
            for (int i = 0; i < count; ++i)
                out.addEvent (juce::MidiMessage::noteOff (keyMapChannel[static_cast<size_t> (key)], mapped[static_cast<size_t> (i)]), pos);
            count = 0;
        }
    }
}
