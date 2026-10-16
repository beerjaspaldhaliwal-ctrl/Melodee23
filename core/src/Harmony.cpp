#include "melodee/Harmony.h"

#include <algorithm>

namespace melodee
{

namespace
{
    struct Candidate
    {
        PitchMask mask;
        int penalty;
    };

    PitchMask maskOf (std::initializer_list<int> iv)
    {
        PitchMask m = 0;
        for (int i : iv)
            m |= pcBit (i);
        return m;
    }

    const std::vector<Candidate>& candidates()
    {
        static const std::vector<Candidate> list = [] {
            std::vector<Candidate> out;

            const PitchMask parents[] = {
                maskOf ({ 0, 2, 4, 5, 7, 9, 11 }), // major
                maskOf ({ 0, 2, 3, 5, 7, 9, 11 }), // melodic minor
                maskOf ({ 0, 2, 3, 5, 7, 8, 11 }), // harmonic minor
                maskOf ({ 0, 2, 4, 5, 7, 8, 11 }), // harmonic major
            };
            const int penalties[] = { 0, 2, 3, 4 };

            for (int p = 0; p < 4; ++p)
                for (int t = 0; t < 12; ++t)
                    out.push_back ({ maskTranspose (parents[p], t), penalties[p] });

            for (int t = 0; t < 2; ++t)
                out.push_back ({ maskTranspose (maskOf ({ 0, 2, 4, 6, 8, 10 }), t), 6 });
            for (int t = 0; t < 3; ++t)
                out.push_back ({ maskTranspose (maskOf ({ 0, 1, 3, 4, 6, 7, 9, 10 }), t), 6 });

            return out;
        }();
        return list;
    }
} // namespace

PitchMask bestFitChordScale (PitchMask chordMask, PitchMask keyMask, int chordRoot)
{
    PitchMask best = 0;
    int bestScore = -1000;

    for (const auto& c : candidates())
    {
        if ((chordMask & ~c.mask) != 0)
            continue;

        int score = maskCount (static_cast<PitchMask> (c.mask & keyMask)) * 10 - c.penalty;

        // Prefer scales whose notes around the root are also in the key (keeps modes "in key").
        if (maskHas (c.mask, chordRoot + 2) && maskHas (keyMask, chordRoot + 2))
            score += 1;

        if (score > bestScore)
        {
            bestScore = score;
            best = c.mask;
        }
    }

    return best;
}

ChordContext analyseChord (const Chord& chord, const Scale& key, bool keepKeyNotes)
{
    ChordContext ctx;
    const PitchMask keyMask = key.mask();

    if (chord.noChord)
    {
        ctx.root = key.root();
        ctx.chordTones = ctx.core = ctx.stable = pcBit (key.root());
        ctx.chordScale = keyMask;
        return ctx;
    }

    ctx.root = chord.root;
    ctx.chordTones = chord.mask();
    ctx.core = chord.coreMask();

    for (int i : chord.intervals)
        if (i == 0 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6 || i == 7 || i == 8)
            ctx.stable |= pcBit (chord.root + i);

    if (keepKeyNotes)
    {
        ctx.chordScale = static_cast<PitchMask> (keyMask | ctx.chordTones);
    }
    else if ((ctx.chordTones & ~keyMask) == 0)
    {
        ctx.chordScale = keyMask; // diatonic chord: the key itself
    }
    else
    {
        const PitchMask fit = bestFitChordScale (ctx.chordTones, keyMask, chord.root);
        if (fit != 0)
        {
            ctx.chordScale = static_cast<PitchMask> ((keyMask & fit) | ctx.chordTones);
        }
        else
        {
            // Nothing fits: keep key notes that are not a half step from a chord tone.
            PitchMask clash = 0;
            for (int pc = 0; pc < 12; ++pc)
                if (maskHas (ctx.chordTones, pc))
                    clash |= static_cast<PitchMask> (pcBit (pc + 1) | pcBit (pc - 1));
            ctx.chordScale = static_cast<PitchMask> ((keyMask & ~clash) | ctx.chordTones);
        }
    }

    // Avoid notes: a half step above a core tone and not a chord tone.
    for (int pc = 0; pc < 12; ++pc)
        if (maskHas (ctx.core, pc) && maskHas (ctx.chordScale, pc + 1) && ! maskHas (ctx.chordTones, pc + 1))
            ctx.avoid |= pcBit (pc + 1);

    // Colour tones: chord extensions, plus 9/11/13 from the chord scale that are not avoid notes.
    ctx.colour = chord.extensionMask();
    for (int iv : { 2, 5, 9 })
    {
        const int pc = chord.root + iv;
        if (maskHas (ctx.chordScale, pc) && ! maskHas (ctx.avoid, pc) && ! maskHas (ctx.core, pc))
            ctx.colour |= pcBit (pc);
    }

    return ctx;
}

std::vector<ChordContext> analyseProgression (const Progression& progression, const Scale& key, bool keepKeyNotes)
{
    std::vector<ChordContext> out;
    out.reserve (progression.chords.size());
    for (const auto& e : progression.chords)
        out.push_back (analyseChord (e.chord, key, keepKeyNotes));
    return out;
}

Progression repeatProgression (const Progression& progression, int times)
{
    if (times <= 1)
        return progression;

    Progression out;
    out.beatsPerBar = progression.beatsPerBar;

    for (int r = 0; r < times; ++r)
    {
        for (auto e : progression.chords)
        {
            e.start += r * progression.totalBeats;
            out.chords.push_back (e);
        }
    }

    out.totalBeats = progression.totalBeats * times;
    return out;
}

int stepsBetween (PitchMask mask, int a, int b) noexcept
{
    if (mask == 0)
        return b - a;

    if (a == b)
        return 0;

    const int dir = b > a ? 1 : -1;
    int steps = 0;
    for (int n = a; n != b;)
    {
        n += dir;
        if (maskHas (mask, n))
            ++steps;
    }

    // If b itself is not in the mask, count it as a step (so neighbours stay distinct).
    if (! maskHas (mask, b))
        ++steps;

    return dir * steps;
}

} // namespace melodee
