#include "MelodyInternal.h"

#include <algorithm>
#include <cmath>

namespace melodee
{

using namespace detail;

namespace
{
    struct TransformInfo
    {
        const char* name;
        const char* description;
    };

    const std::vector<TransformInfo>& transformInfo()
    {
        static const std::vector<TransformInfo> list {
            { "New Rhythm", "Keeps the notes in order but writes a new rhythm for them." },
            { "New Notes", "Keeps the rhythm and the overall shape but chooses new notes." },
            { "Inversion", "Mirrors every interval: up becomes down." },
            { "Retrograde", "Plays the notes backwards over the same rhythm." },
            { "Retrograde Inversion", "Backwards and mirrored." },
            { "Embellish", "Adds passing tones and neighbour notes between the main notes." },
            { "Simplify", "Keeps the strong-beat notes and lets them ring." },
            { "Ornament", "Adds trills, mordents, turns and grace notes." },
            { "Syncopate", "Pushes strong-beat notes ahead of the beat (anticipations)." },
            { "Double Time", "Twice as fast, repeated to fill the progression." },
            { "Half Time", "Twice as slow." },
            { "Octave Up", "Moves the melody up an octave." },
            { "Octave Down", "Moves the melody down an octave." },
            { "Sequence", "Repeats the first bar's idea on every chord, moved to fit." },
            { "Answer Phrase", "Keeps the first half as a question and writes a resolving answer." },
            { "Harmonize in 3rds", "Adds a second voice a diatonic third above." },
            { "Harmonize in 6ths", "Adds a second voice a diatonic sixth below." },
            { "Swing", "Applies a swing feel to off-beat 8ths." },
            { "Humanize", "Adds subtle timing and velocity variation." },
            { "Fit To Chords", "Re-fits the notes to the current chords (use after changing chords)." },
        };
        return list;
    }

    class Transformer
    {
    public:
        Transformer (const Progression& progression, const Scale& keyScale, const MelodySettings& settings, std::uint32_t seed)
            : prog (progression), key (keyScale), s (settings), rng (seed),
              ctxs (analyseProgression (progression, keyScale)),
              bpb (std::max (1, settings.beatsPerBar)), total (progression.totalBeats),
              lo (settings.lowNote), hi (std::max (settings.highNote, settings.lowNote + 12))
        {
        }

        std::vector<NoteEvent> apply (std::vector<NoteEvent> notes, MelodyTransform t)
        {
            if (notes.empty() || prog.empty())
                return notes;

            std::stable_sort (notes.begin(), notes.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });

            switch (t)
            {
                case MelodyTransform::NewRhythm:           return newRhythm (notes);
                case MelodyTransform::NewPitches:          return newPitches (notes);
                case MelodyTransform::Inversion:           return finish (invert (notes));
                case MelodyTransform::Retrograde:          return finish (retrograde (notes));
                case MelodyTransform::RetrogradeInversion: return finish (invert (retrograde (notes)));
                case MelodyTransform::Embellish:           return embellish (notes);
                case MelodyTransform::Simplify:            return simplify (notes);
                case MelodyTransform::Ornament:            return ornament (notes);
                case MelodyTransform::Syncopate:           return syncopate (notes);
                case MelodyTransform::DoubleTime:          return doubleTime (notes);
                case MelodyTransform::HalfTime:            return halfTime (notes);
                case MelodyTransform::OctaveUp:            return shiftOctave (notes, 12);
                case MelodyTransform::OctaveDown:          return shiftOctave (notes, -12);
                case MelodyTransform::Sequence:            return sequence (notes);
                case MelodyTransform::AnswerPhrase:        return answer (notes);
                case MelodyTransform::HarmonizeThirds:     return harmonize (notes, 2, 0);
                case MelodyTransform::HarmonizeSixths:     return harmonize (notes, 2, -12);
                case MelodyTransform::Swing:               applySwing (notes, 0.6, false); return notes;
                case MelodyTransform::Humanize:            applyHumanize (notes, 0.6, rng); return notes;
                case MelodyTransform::FitToChords:         return finish (notes);
            }
            return notes;
        }

    private:
        const Progression& prog;
        const Scale& key;
        MelodySettings s;
        Rng rng;
        std::vector<ChordContext> ctxs;
        int bpb;
        double total;
        int lo, hi;

        const ChordContext& ctxAt (double t) const { return ctxs[static_cast<size_t> (contextIndexAt (prog, t))]; }

        double strengthAt (double t) const { return metricStrength (std::fmod (t, static_cast<double> (bpb)), bpb); }

        EmbellishContext embellishContext() const { return { prog, ctxs, bpb, total, lo, hi, pentatonicOf (key) }; }

        int keepInRange (int n) const
        {
            while (n < lo - 12) n += 12;
            while (n > hi + 12) n -= 12;
            return n;
        }

        /** Fits every note to the chord under it, then cleans up. */
        std::vector<NoteEvent> finish (std::vector<NoteEvent> notes)
        {
            for (size_t i = 0; i < notes.size(); ++i)
            {
                const int dir = i > 0 ? (notes[i].pitch >= notes[i - 1].pitch ? -1 : 1) : -1;
                notes[i].pitch = keepInRange (fitPitchToChord (notes[i].pitch, ctxAt (notes[i].start), strengthAt (notes[i].start), dir));
            }
            finaliseMonophonic (notes, total, 0, 127);
            return notes;
        }

        std::vector<NoteEvent> invert (std::vector<NoteEvent> notes)
        {
            const PitchMask mask = key.mask();
            const int pivot = notes.front().pitch;
            for (auto& n : notes)
                n.pitch = stepAlongMask (mask, pivot, -stepsBetween (mask, pivot, n.pitch));
            return notes;
        }

        std::vector<NoteEvent> retrograde (std::vector<NoteEvent> notes)
        {
            std::vector<int> pitches;
            for (const auto& n : notes)
                pitches.push_back (n.pitch);
            std::reverse (pitches.begin(), pitches.end());
            for (size_t i = 0; i < notes.size(); ++i)
                notes[i].pitch = pitches[i];
            return notes;
        }

        std::vector<NoteEvent> newRhythm (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> out;
            const int numBars = std::max (1, static_cast<int> (std::ceil (total / bpb - 1e-9)));
            size_t idx = 0;

            for (int b = 0; b < numBars && idx < notes.size(); ++b)
            {
                const double barStart = b * static_cast<double> (bpb);
                const double barLen = std::min (static_cast<double> (bpb), total - barStart);

                int count = 0;
                for (const auto& n : notes)
                    if (n.start >= barStart - 1e-6 && n.start < barStart + barLen - 1e-6)
                        ++count;
                count = std::clamp (count + rng.range (-1, 1), 1, static_cast<int> (barLen * 4));

                const int slotsTotal = static_cast<int> (std::lround (barLen * 4));
                std::vector<double> w (static_cast<size_t> (slotsTotal));
                for (int i = 0; i < slotsTotal; ++i)
                {
                    const double strength = metricStrength (i / 4.0, bpb);
                    w[static_cast<size_t> (i)] = std::pow (strength * (i % 2 == 1 ? 0.6 : 1.0) + (i % 4 == 2 ? 0.25 : 0.0), 1.4);
                }

                std::vector<int> slots;
                for (int k = 0; k < count; ++k)
                {
                    const int pick = rng.weighted (w);
                    if (pick < 0)
                        break;
                    slots.push_back (pick);
                    w[static_cast<size_t> (pick)] = 0.0;
                }
                std::sort (slots.begin(), slots.end());

                for (int sl : slots)
                {
                    if (idx >= notes.size())
                        break;
                    NoteEvent n = notes[idx++];
                    n.start = barStart + sl / 4.0;
                    n.length = 4.0; // trimmed by finish()
                    out.push_back (n);
                }
            }

            for (size_t i = 0; i < out.size(); ++i)
            {
                const double gap = (i + 1 < out.size() ? out[i + 1].start : total) - out[i].start;
                out[i].length = std::min (gap, 2.0) * 0.88;
            }

            return finish (out);
        }

        std::vector<NoteEvent> newPitches (std::vector<NoteEvent> notes)
        {
            int prev = -1;
            for (auto& n : notes)
            {
                const auto& h = ctxAt (n.start);
                const double strength = strengthAt (n.start);
                const int original = n.pitch;

                std::vector<int> cands;
                std::vector<double> weights;
                for (int c = original - 5; c <= original + 5; ++c)
                {
                    if (! maskHas (h.chordScale, c))
                        continue;

                    double w = maskHas (h.core, c) ? (strength >= 0.6 ? 5.0 : 1.5) : (strength >= 0.6 ? 0.2 : 1.0);
                    if (maskHas (h.avoid, c) && strength >= 0.6)
                        w *= 0.05;
                    if (c == original)
                        w *= 0.3; // prefer something new
                    if (prev >= 0)
                    {
                        const int a = std::abs (c - prev);
                        w *= a == 0 ? 0.3 : (a <= 2 ? 1.0 : (a <= 4 ? 0.6 : 0.25));
                    }

                    cands.push_back (c);
                    weights.push_back (w);
                }

                const int pick = rng.weighted (weights);
                if (pick >= 0)
                    n.pitch = cands[static_cast<size_t> (pick)];
                prev = n.pitch;
            }

            return finish (notes);
        }

        std::vector<NoteEvent> embellish (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> out;

            for (size_t i = 0; i < notes.size(); ++i)
            {
                auto n = notes[i];
                if (i + 1 >= notes.size())
                {
                    out.push_back (n);
                    break;
                }

                const auto& next = notes[i + 1];
                const double gap = next.start - n.start;
                const int interval = next.pitch - n.pitch;
                const auto& h = ctxAt (n.start);

                if (gap >= 0.5 && std::abs (interval) >= 3)
                {
                    // Passing tones in the last part of the gap.
                    const int dir = interval > 0 ? 1 : -1;
                    std::vector<int> passing;
                    for (int p = stepAlongMask (h.chordScale, n.pitch, dir); dir > 0 ? p < next.pitch : p > next.pitch; p = stepAlongMask (h.chordScale, p, dir))
                    {
                        passing.push_back (p);
                        if (passing.size() >= 3)
                            break;
                    }

                    const double unit = std::min (0.25, gap / (passing.size() + 1.0));
                    const double passStart = next.start - unit * passing.size();
                    n.length = std::max (0.1, std::min (n.length, passStart - n.start));
                    out.push_back (n);

                    for (size_t k = 0; k < passing.size(); ++k)
                        out.push_back ({ passing[k], passStart + k * unit, unit * 0.9, std::max (30, n.velocity - 12) });
                }
                else if (gap >= 1.0 && std::abs (interval) <= 2)
                {
                    // Neighbour note: step away and back.
                    const int neighbour = stepAlongMask (h.chordScale, n.pitch, rng.chance (0.5) ? 1 : -1);
                    const double nStart = n.start + gap * 0.5;
                    n.length = std::min (n.length, gap * 0.5);
                    out.push_back (n);
                    out.push_back ({ neighbour, nStart, gap * 0.45, std::max (30, n.velocity - 10) });
                }
                else
                {
                    out.push_back (n);
                }
            }

            finaliseMonophonic (out, total, 0, 127);
            return out;
        }

        std::vector<NoteEvent> simplify (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> out;
            int lastBar = -1;

            for (const auto& n : notes)
            {
                const int bar = static_cast<int> (n.start / bpb);
                const bool strong = strengthAt (n.start) >= 0.6;
                if (strong || bar != lastBar)
                {
                    NoteEvent k = n;
                    const auto& h = ctxAt (k.start);
                    if (! maskHas (h.core, k.pitch))
                        k.pitch = snapToMask (h.core, k.pitch);
                    out.push_back (k);
                    lastBar = bar;
                }
            }

            for (size_t i = 0; i < out.size(); ++i)
            {
                const double gap = (i + 1 < out.size() ? out[i + 1].start : total) - out[i].start;
                out[i].length = gap * 0.95;
            }

            finaliseMonophonic (out, total, 0, 127);
            return out;
        }

        std::vector<NoteEvent> ornament (std::vector<NoteEvent> notes)
        {
            const auto ec = embellishContext();
            addGraceNotes (notes, ec, rng, false);
            addOrnaments (notes, ec, rng, 1.6);
            finaliseMonophonic (notes, total, 0, 127);
            return notes;
        }

        std::vector<NoteEvent> syncopate (std::vector<NoteEvent> notes)
        {
            for (size_t i = 1; i < notes.size(); ++i)
            {
                auto& n = notes[i];
                if (strengthAt (n.start) < 0.62 || ! rng.chance (0.55))
                    continue;

                const double shift = rng.chance (0.6) ? 0.5 : 0.25;
                const double newStart = n.start - shift;
                if (newStart <= notes[i - 1].start + 0.2)
                    continue;

                n.length += shift;
                n.start = newStart;
            }

            finaliseMonophonic (notes, total, 0, 127);
            return notes;
        }

        std::vector<NoteEvent> doubleTime (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> out;
            const double half = total / 2.0;
            for (int copy = 0; copy < 2; ++copy)
            {
                for (auto n : notes)
                {
                    n.start = n.start / 2.0 + copy * half;
                    n.length /= 2.0;
                    out.push_back (n);
                }
            }
            return finish (out);
        }

        std::vector<NoteEvent> halfTime (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> out;
            for (auto n : notes)
            {
                n.start *= 2.0;
                n.length *= 2.0;
                if (n.start < total - 0.01)
                    out.push_back (n);
            }
            return finish (out);
        }

        std::vector<NoteEvent> shiftOctave (std::vector<NoteEvent> notes, int semis)
        {
            for (auto& n : notes)
                n.pitch = std::clamp (n.pitch + semis, 0, 127);
            return notes;
        }

        std::vector<NoteEvent> sequence (const std::vector<NoteEvent>& notes)
        {
            std::vector<NoteEvent> motif;
            for (const auto& n : notes)
                if (n.start < bpb - 1e-6)
                    motif.push_back (n);
            if (motif.empty())
                return notes;

            const PitchMask mask = key.mask();
            const int firstRoot = ctxAt (motif.front().start).root;
            const int numBars = std::max (1, static_cast<int> (std::ceil (total / bpb - 1e-9)));

            std::vector<NoteEvent> out;
            for (int b = 0; b < numBars; ++b)
            {
                const double barStart = b * static_cast<double> (bpb);
                int shiftSemis = mod12 (ctxAt (barStart + (motif.front().start)).root - firstRoot);
                if (shiftSemis > 6)
                    shiftSemis -= 12;
                const int base = 60 + mod12 (firstRoot);
                const int steps = stepsBetween (mask, base, base + shiftSemis);

                for (auto n : motif)
                {
                    n.start += barStart;
                    if (n.start >= total - 0.01)
                        continue;
                    n.pitch = stepAlongMask (mask, n.pitch, steps);
                    out.push_back (n);
                }
            }

            return finish (out);
        }

        std::vector<NoteEvent> answer (const std::vector<NoteEvent>& notes)
        {
            const int numBars = std::max (1, static_cast<int> (std::ceil (total / bpb - 1e-9)));
            const double half = std::max (1, numBars / 2) * static_cast<double> (bpb);

            std::vector<NoteEvent> question;
            for (const auto& n : notes)
                if (n.start < half - 1e-6)
                    question.push_back (n);
            if (question.empty())
                return notes;

            // The question ends open: move a final tonic to another chord tone.
            {
                auto& q = question.back();
                const auto& h = ctxAt (q.start);
                if (mod12 (q.pitch) == key.root())
                {
                    PitchMask others = static_cast<PitchMask> (h.stable & ~pcBit (key.root()));
                    if (others == 0)
                        others = static_cast<PitchMask> (key.mask() & ~pcBit (key.root()));
                    q.pitch = snapToMask (others, q.pitch + 1);
                }
            }

            std::vector<NoteEvent> out = question;
            for (auto n : question)
            {
                n.start += half;
                if (n.start < total - 0.01)
                    out.push_back (n);
            }

            out = finish (out);

            // The answer resolves: approach the tonic by step and land on it.
            if (out.size() >= 2)
            {
                auto& last = out.back();
                auto& before = out[out.size() - 2];
                int tonic = last.pitch;
                for (int d = 0; d <= 6; ++d)
                {
                    if (mod12 (last.pitch - d) == key.root()) { tonic = last.pitch - d; break; }
                    if (mod12 (last.pitch + d) == key.root()) { tonic = last.pitch + d; break; }
                }
                last.pitch = tonic;
                last.length = std::max (last.length, (total - last.start) * 0.95);
                if (before.start >= half)
                    before.pitch = stepAlongMask (key.mask(), tonic, rng.chance (0.5) ? 1 : -1);
            }

            return out;
        }

        std::vector<NoteEvent> harmonize (const std::vector<NoteEvent>& notes, int steps, int octave)
        {
            std::vector<NoteEvent> out = notes;
            for (const auto& n : notes)
            {
                const auto& h = ctxAt (n.start);
                NoteEvent v = n;
                v.pitch = std::clamp (stepAlongMask (h.chordScale, n.pitch, steps) + octave, 0, 127);
                v.velocity = std::max (25, n.velocity - 10);
                out.push_back (v);
            }
            std::stable_sort (out.begin(), out.end(), [] (const NoteEvent& a, const NoteEvent& b) {
                return a.start < b.start || (a.start == b.start && a.pitch < b.pitch);
            });
            return out;
        }
    };
} // namespace

const std::vector<std::string>& transformNames()
{
    static const std::vector<std::string> names = [] {
        std::vector<std::string> n;
        for (const auto& t : transformInfo())
            n.push_back (t.name);
        return n;
    }();
    return names;
}

std::string transformDescription (MelodyTransform transform)
{
    const auto& info = transformInfo();
    const auto idx = static_cast<size_t> (transform);
    return idx < info.size() ? info[idx].description : std::string();
}

int numTransforms() noexcept { return static_cast<int> (MelodyTransform::FitToChords) + 1; }

Melody transformMelody (const Melody& melody, MelodyTransform transform, const Progression& progression,
                        const Scale& key, const MelodySettings& settings, std::uint32_t seed)
{
    Transformer t (progression, key, settings, seed);

    Melody out = melody;
    out.notes = t.apply (melody.notes, transform);
    out.seed = seed;

    const auto& name = transformNames()[static_cast<size_t> (transform)];
    out.name = melody.name + " > " + name;
    out.description = name + " of \"" + melody.name + "\". " + transformDescription (transform);
    return out;
}

} // namespace melodee
