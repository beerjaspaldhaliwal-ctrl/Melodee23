#include "PluginProcessor.h"

#include "PluginEditor.h"

MelodeeProcessor::MelodeeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter (volume = new juce::AudioParameterFloat (juce::ParameterID { "volume", 1 }, "Preview Volume",
                                                          juce::NormalisableRange<float> (-48.0f, 6.0f, 0.1f), -6.0f,
                                                          juce::AudioParameterFloatAttributes().withLabel ("dB")));

    session.addChangeListener (this);
    syncFlags();
    rebuildPattern();
}

MelodeeProcessor::~MelodeeProcessor()
{
    session.removeChangeListener (this);
}

//==============================================================================
void MelodeeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    sequencer.prepare (sampleRate);
    synth.prepare (sampleRate, samplesPerBlock);

    const auto capacity = static_cast<size_t> (juce::jmax (4096, samplesPerBlock * 8));
    scratchIn.ensureSize (capacity);
    scratchOut.ensureSize (capacity);
}

bool MelodeeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MelodeeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    scratchIn.clear();
    scratchIn.addEvents (midi, 0, numSamples, 0);
    midi.clear();
    scratchOut.clear();

    // 1. Your keyboard (thru, scale lock or chord trigger)
    sequencer.processKeyboard (scratchIn, scratchOut, keyboardMode.load());

    // 2. The looping pattern
    juce::Optional<juce::AudioPlayHead::PositionInfo> position;
    if (auto* hostPlayHead = getPlayHead())
        position = hostPlayHead->getPosition();

    sequencer.process (scratchOut, numSamples, position, syncToHost.load(), previewing.load(), previewBpm.load());

    // 3. Clicked chords / notes from the editor
    {
        const juce::SpinLock::ScopedTryLockType tl (auditionLock);
        if (tl.isLocked())
        {
            int kept = 0;
            for (int i = 0; i < numAuditions; ++i)
            {
                auto a = auditions[static_cast<size_t> (i)];
                if (! a.started)
                {
                    scratchOut.addEvent (juce::MidiMessage::noteOn (a.channel, a.note, static_cast<juce::uint8> (96)), 0);
                    a.started = true;
                }

                if (a.samplesLeft < numSamples)
                {
                    scratchOut.addEvent (juce::MidiMessage::noteOff (a.channel, a.note), juce::jmax (0, a.samplesLeft));
                    continue;
                }

                a.samplesLeft -= numSamples;
                auditions[static_cast<size_t> (kept++)] = a;
            }
            numAuditions = kept;
        }
    }

    // 4. The built-in sound
    if (soundOn.load())
    {
        synth.render (buffer, scratchOut, numSamples);
        buffer.applyGain (juce::Decibels::decibelsToGain (volume->get()));
    }

    // 5. MIDI out to the host (separate channels, or everything on channel 1)
    if (outputMode.load() == 1)
    {
        for (const auto m : scratchOut)
        {
            auto msg = m.getMessage();
            msg.setChannel (1);
            midi.addEvent (msg, m.samplePosition);
        }
    }
    else
    {
        midi.swapWith (scratchOut);
    }
}

//==============================================================================
void MelodeeProcessor::setPreviewing (bool shouldPreview)
{
    if (shouldPreview && ! previewing.load())
        sequencer.restartPreview();
    previewing = shouldPreview;
}

void MelodeeProcessor::audition (const std::vector<int>& notes, int channel, double seconds)
{
    const juce::SpinLock::ScopedLockType sl (auditionLock);
    const int samples = static_cast<int> (seconds * currentSampleRate);

    for (int n : notes)
    {
        if (numAuditions >= static_cast<int> (auditions.size()))
            break;
        auditions[static_cast<size_t> (numAuditions++)] = { juce::jlimit (0, 127, n), juce::jlimit (1, 16, channel), samples, false };
    }
}

//==============================================================================
void MelodeeProcessor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    syncFlags();
    rebuildPattern();
}

void MelodeeProcessor::syncFlags()
{
    soundOn = session.getBool (IDs::soundOn);
    syncToHost = session.getBool (IDs::syncToHost);
    keyboardMode = session.getInt (IDs::keyboardMode);
    outputMode = session.getInt (IDs::outputMode);
    previewBpm = session.getDouble (IDs::previewBpm);
}

void MelodeeProcessor::rebuildPattern()
{
    Pattern p;
    const auto& prog = session.progression();
    p.length = prog.totalBeats;

    auto addNotes = [&p] (const std::vector<melodee::NoteEvent>& notes, int channel) {
        for (const auto& n : notes)
        {
            const double on = juce::jmax (0.0, n.start);
            if (on >= p.length)
                continue;
            const double off = juce::jlimit (on + 0.01, p.length - 0.0005, n.start + n.length);
            p.events.push_back ({ on, channel, n.pitch, n.velocity, true });
            p.events.push_back ({ off, channel, n.pitch, 0, false });
        }
    };

    if (session.getBool (IDs::playMelody))
        if (const auto* m = session.selectedMelody())
            addNotes (m->notes, 1);
    if (session.getBool (IDs::playChords))
        addNotes (session.chordNotes(), 2);
    if (session.getBool (IDs::playBass))
        addNotes (session.bassNotes(), 3);

    std::stable_sort (p.events.begin(), p.events.end(), [] (const PatternEvent& a, const PatternEvent& b) {
        if (a.beat < b.beat)
            return true;
        if (b.beat < a.beat)
            return false;
        return ! a.on && b.on; // note-offs first
    });

    p.chordVoicings = session.chordVoicings();
    p.scaleMask = session.scale().mask();

    sequencer.setPattern (std::move (p));
}

//==============================================================================
juce::AudioProcessorEditor* MelodeeProcessor::createEditor()
{
    return new MelodeeEditor (*this);
}

void MelodeeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = session.toXml())
    {
        xml->setAttribute ("volume", static_cast<double> (volume->get()));
        copyXmlToBinary (*xml, destData);
    }
}

void MelodeeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasAttribute ("volume"))
            *volume = static_cast<float> (xml->getDoubleAttribute ("volume", -6.0));
        session.fromXml (*xml);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MelodeeProcessor();
}
