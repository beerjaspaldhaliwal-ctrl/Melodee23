#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PreviewSynth.h"
#include "Sequencer.h"
#include "Session.h"

#include <array>
#include <atomic>

class MelodeeProcessor : public juce::AudioProcessor,
                         private juce::ChangeListener
{
public:
    MelodeeProcessor();
    ~MelodeeProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Melodee"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    Session& getSession() noexcept { return session; }
    Sequencer& getSequencer() noexcept { return sequencer; }

    bool isPreviewing() const noexcept { return previewing.load(); }
    void setPreviewing (bool shouldPreview);

    /** Plays notes on the built-in synth (and MIDI out) for a moment: used when clicking chords. */
    void audition (const std::vector<int>& notes, int channel, double seconds);

    double getBpm() const noexcept { return sequencer.getHostBpm(); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuildPattern();
    void syncFlags();

    Session session;
    Sequencer sequencer;
    PreviewSynth synth;

    juce::AudioParameterFloat* volume = nullptr;

    std::atomic<bool> previewing { false };
    std::atomic<bool> soundOn { true };
    std::atomic<bool> syncToHost { true };
    std::atomic<int> keyboardMode { 0 };
    std::atomic<int> outputMode { 0 };
    std::atomic<double> previewBpm { 110.0 };

    juce::MidiBuffer scratchIn, scratchOut;

    // Auditions are queued by the UI and played by the audio thread.
    struct Audition
    {
        int note = 60, channel = 1, samplesLeft = 0;
        bool started = false;
    };
    juce::SpinLock auditionLock;
    std::array<Audition, 32> auditions {};
    int numAuditions = 0;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelodeeProcessor)
};
