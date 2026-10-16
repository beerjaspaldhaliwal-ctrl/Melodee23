#include "PreviewSynth.h"

#include <cmath>

namespace
{
    struct AnySound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class Voice : public juce::SynthesiserVoice
    {
    public:
        explicit Voice (PreviewSynth::Timbre t) : timbre (t)
        {
            juce::ADSR::Parameters p;
            switch (timbre)
            {
                case PreviewSynth::Timbre::Lead: p = { 0.005f, 0.25f, 0.55f, 0.18f }; gain = 0.16f; break;
                case PreviewSynth::Timbre::Keys: p = { 0.004f, 1.40f, 0.30f, 0.35f }; gain = 0.10f; break;
                case PreviewSynth::Timbre::Bass: p = { 0.004f, 0.35f, 0.70f, 0.08f }; gain = 0.24f; break;
            }
            adsr.setParameters (p);
        }

        bool canPlaySound (juce::SynthesiserSound*) override { return true; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            frequency = juce::MidiMessage::getMidiNoteInHertz (midiNote);
            level = 0.25f + 0.75f * velocity;
            phase = 0.0;
            modPhase = 0.0;
            brightness = 1.0f;
            filterState = 0.0f;
            adsr.setSampleRate (getSampleRate());
            adsr.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff)
            {
                adsr.noteOff();
            }
            else
            {
                adsr.reset();
                clearCurrentNote();
            }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
        {
            if (! isVoiceActive())
                return;

            const double sr = getSampleRate();
            const double inc = juce::MathConstants<double>::twoPi * frequency / sr;
            const float brightnessDecay = std::pow (0.0005f, 1.0f / static_cast<float> (sr * 0.9));
            const float cutoff = juce::jlimit (0.02f, 0.9f, static_cast<float> (frequency * 6.0 / sr));

            for (int i = 0; i < numSamples; ++i)
            {
                float s = 0.0f;
                switch (timbre)
                {
                    case PreviewSynth::Timbre::Lead:
                    {
                        // Band-limited-ish saw from a few harmonics, softened with a one-pole filter.
                        float saw = 0.0f;
                        for (int h = 1; h <= 6; ++h)
                            if (frequency * h < sr * 0.45)
                                saw += static_cast<float> (std::sin (phase * h)) / static_cast<float> (h);
                        filterState += cutoff * 2.0f * (saw - filterState);
                        s = 0.7f * filterState + 0.3f * static_cast<float> (std::sin (phase));
                        break;
                    }
                    case PreviewSynth::Timbre::Keys:
                    {
                        // Two-operator FM electric piano: bright attack that mellows out.
                        const float mod = static_cast<float> (std::sin (modPhase)) * 1.6f * brightness;
                        s = static_cast<float> (std::sin (phase + mod));
                        brightness *= brightnessDecay;
                        modPhase += inc;
                        break;
                    }
                    case PreviewSynth::Timbre::Bass:
                    {
                        const float raw = static_cast<float> (std::sin (phase) + 0.35 * std::sin (2.0 * phase));
                        s = std::tanh (1.6f * raw);
                        break;
                    }
                }

                phase += inc;
                if (phase > juce::MathConstants<double>::twoPi * 64.0)
                {
                    phase = std::fmod (phase, juce::MathConstants<double>::twoPi);
                    modPhase = std::fmod (modPhase, juce::MathConstants<double>::twoPi);
                }

                const float sample = s * level * gain * adsr.getNextSample();
                for (int ch = 0; ch < out.getNumChannels(); ++ch)
                    out.addSample (ch, startSample + i, sample);

                if (! adsr.isActive())
                {
                    clearCurrentNote();
                    break;
                }
            }
        }

    private:
        PreviewSynth::Timbre timbre;
        juce::ADSR adsr;
        double frequency = 440.0, phase = 0.0, modPhase = 0.0;
        float level = 1.0f, gain = 0.15f, brightness = 1.0f, filterState = 0.0f;
    };

    void setUp (juce::Synthesiser& synth, PreviewSynth::Timbre timbre, int voices)
    {
        synth.addSound (new AnySound());
        for (int i = 0; i < voices; ++i)
            synth.addVoice (new Voice (timbre));
    }
} // namespace

PreviewSynth::PreviewSynth()
{
    setUp (lead, Timbre::Lead, 8);
    setUp (keys, Timbre::Keys, 16);
    setUp (bass, Timbre::Bass, 4);
}

void PreviewSynth::prepare (double sampleRate, int maxBlockSize)
{
    for (auto* s : { &lead, &keys, &bass })
        s->setCurrentPlaybackSampleRate (sampleRate);

    for (auto* m : { &leadMidi, &keysMidi, &bassMidi })
        m->ensureSize (static_cast<size_t> (juce::jmax (2048, maxBlockSize * 4)));
}

void PreviewSynth::render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int numSamples)
{
    leadMidi.clear();
    keysMidi.clear();
    bassMidi.clear();

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int ch = msg.getChannel();
        auto& target = ch == 2 ? keysMidi : (ch == 3 ? bassMidi : leadMidi);
        target.addEvent (msg, metadata.samplePosition);
    }

    lead.renderNextBlock (buffer, leadMidi, 0, numSamples);
    keys.renderNextBlock (buffer, keysMidi, 0, numSamples);
    bass.renderNextBlock (buffer, bassMidi, 0, numSamples);
}

void PreviewSynth::allNotesOff()
{
    for (auto* s : { &lead, &keys, &bass })
        s->allNotesOff (0, false);
}
