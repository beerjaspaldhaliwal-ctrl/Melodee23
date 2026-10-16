#include "MelodyInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace melodee::detail
{

namespace
{
    void sortByStart (std::vector<NoteEvent>& notes)
    {
        std::stable_sort (notes.begin(), notes.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });
    }
} // namespace

void addGraceNotes (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng, bool blues)
{
    const double probability = blues ? 0.3 : 0.15;
    const double grace = 0.1;
    const double bar = static_cast<double> (ec.beatsPerBar);

    std::vector<NoteEvent> extra;
    for (auto& n : notes)
    {
        if (n.length < 0.5 || metricStrength (std::fmod (n.start, bar), ec.beatsPerBar) < 0.6 || ! rng.chance (probability))
            continue;

        const auto& h = ec.at (n.start);
        NoteEvent g = n;
        g.length = grace;
        g.velocity = std::max (30, n.velocity - 18);
        g.pitch = blues ? n.pitch - 1 : stepAlongMask (h.chordScale, n.pitch, rng.chance (0.5) ? 1 : -1);

        n.start += grace;
        n.length -= grace;
        extra.push_back (g);
    }

    notes.insert (notes.end(), extra.begin(), extra.end());
    sortByStart (notes);
}

void addOrnaments (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng, double amount)
{
    std::vector<NoteEvent> out;
    const double unit = 0.125;

    for (const auto& n : notes)
    {
        if (n.length < 0.75)
        {
            out.push_back (n);
            continue;
        }

        const auto& h = ec.at (n.start);
        const int upper = stepAlongMask (h.chordScale, n.pitch, 1);
        const int lower = stepAlongMask (h.chordScale, n.pitch, -1);
        const double r = rng.uniform() / std::max (0.1, amount);

        auto add = [&] (int pitch, double start, double length, int velocity) {
            NoteEvent o = n;
            o.pitch = pitch;
            o.start = start;
            o.length = length;
            o.velocity = std::clamp (velocity, 25, 127);
            out.push_back (o);
        };

        if (r < 0.12)
        {
            // Trill for the first part, then hold.
            const double trillEnd = n.start + n.length * 0.6;
            double t = n.start;
            bool up = false;
            while (t + unit < trillEnd)
            {
                add (up ? upper : n.pitch, t, unit, n.velocity - (up ? 10 : 4));
                up = ! up;
                t += unit;
            }
            add (n.pitch, t, n.start + n.length - t, n.velocity);
        }
        else if (r < 0.32)
        {
            // Mordent: note, upper neighbour, note.
            add (n.pitch, n.start, unit, n.velocity);
            add (upper, n.start + unit, unit, n.velocity - 12);
            add (n.pitch, n.start + 2 * unit, n.length - 2 * unit, n.velocity);
        }
        else if (r < 0.44)
        {
            // Turn: upper, note, lower, note.
            add (upper, n.start, unit, n.velocity - 8);
            add (n.pitch, n.start + unit, unit, n.velocity - 10);
            add (lower, n.start + 2 * unit, unit, n.velocity - 10);
            add (n.pitch, n.start + 3 * unit, n.length - 3 * unit, n.velocity);
        }
        else
        {
            out.push_back (n);
        }
    }

    notes = std::move (out);
}

void addRuns (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng)
{
    std::vector<NoteEvent> out;

    for (size_t i = 0; i < notes.size(); ++i)
    {
        auto n = notes[i];
        const bool hasNext = i + 1 < notes.size();
        const int nextPitch = hasNext ? notes[i + 1].pitch : n.pitch - 5;
        const double gapToNext = (hasNext ? notes[i + 1].start : ec.total) - n.start;

        if (n.length >= 1.0 && gapToNext >= 1.0 && std::abs (nextPitch - n.pitch) >= 2 && rng.chance (0.35))
        {
            const auto& h = ec.at (n.start);
            const PitchMask runMask = rng.chance (0.5) ? static_cast<PitchMask> ((ec.pentaMask & h.chordScale) | h.core) : h.chordScale;
            const int count = rng.range (3, 5);
            const double unit = rng.chance (0.5) ? 0.125 : 1.0 / 6.0;
            const double runStart = n.start + n.length - count * unit;

            if (runStart > n.start + 0.25)
            {
                const int dir = nextPitch > n.pitch ? 1 : -1;
                n.length = runStart - n.start;
                out.push_back (n);

                int pitch = n.pitch;
                for (int k = 0; k < count; ++k)
                {
                    pitch = stepAlongMask (runMask, pitch, dir);
                    if ((dir > 0 && pitch >= nextPitch) || (dir < 0 && pitch <= nextPitch))
                        pitch = stepAlongMask (runMask, pitch, -dir * 2);

                    NoteEvent r = n;
                    r.pitch = pitch;
                    r.start = runStart + k * unit;
                    r.length = unit * 0.95;
                    r.velocity = std::max (30, n.velocity - 14 + k * 2);
                    out.push_back (r);
                }
                continue;
            }
        }

        out.push_back (n);
    }

    notes = std::move (out);
}

void addRolls (std::vector<NoteEvent>& notes, const EmbellishContext& ec, Rng& rng)
{
    std::vector<NoteEvent> out;
    int lastRollBar = -1;

    for (const auto& n : notes)
    {
        const int bar = static_cast<int> (n.start / ec.beatsPerBar);
        const double pos = n.start - bar * ec.beatsPerBar;

        if (bar != lastRollBar && pos > 0.01 && n.length >= 0.45 && rng.chance (0.3))
        {
            lastRollBar = bar;
            const bool triplets = rng.chance (0.6);
            const int count = triplets ? 3 : 4;
            const double unit = triplets ? 1.0 / 6.0 : 0.125;
            const bool octaveFlip = rng.chance (0.3) && n.pitch + 12 <= ec.hi + 5;

            for (int k = 0; k < count; ++k)
            {
                NoteEvent r = n;
                r.start = n.start + k * unit;
                r.length = k == count - 1 ? std::max (unit, n.length - k * unit) : unit * 0.9;
                r.velocity = std::clamp (n.velocity - 20 + k * 8, 30, 127);
                if (octaveFlip && k == count - 1)
                    r.pitch += 12;
                out.push_back (r);
            }
            continue;
        }

        out.push_back (n);
    }

    notes = std::move (out);
}

} // namespace melodee::detail
