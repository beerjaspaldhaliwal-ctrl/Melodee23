#include "melodee/Accompaniment.h"

#include "MelodyInternal.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace melodee
{

namespace
{
    struct Hit
    {
        double pos;     // beats into the bar
        double length;  // beats
        int velocityOffset;
    };

    /** Pitch class nearest to `center` (ties go down). */
    int nearestWithPc (int pc, int center)
    {
        int best = center;
        int bestDist = 100;
        for (int n = center - 6; n <= center + 6; ++n)
        {
            if (mod12 (n) == mod12 (pc) && std::abs (n - center) < bestDist)
            {
                bestDist = std::abs (n - center);
                best = n;
            }
        }
        return best;
    }

    int intervalPriority (int iv)
    {
        switch (iv)
        {
            case 0:  return 0;    // root
            case 3:
            case 4:  return 1;    // 3rd
            case 2:
            case 5:  return 2;    // sus tone
            case 10:
            case 11: return 3;    // 7th
            case 9:  return 4;    // 6th / dim7
            case 14:
            case 13:
            case 15: return 5;    // 9ths
            case 18:
            case 20:
            case 21: return 6;    // #11, b13, 13
            case 17: return 7;    // 11
            case 6:
            case 8:  return 5;    // altered 5ths matter
            case 7:  return 8;    // perfect 5th is the first to go
            default: return 9;
        }
    }

    /** The chord's tones as pitch-class offsets from the root, most important first, at most `maxNotes`. */
    std::vector<int> chooseTones (const Chord& chord, int maxNotes)
    {
        std::vector<int> iv = chord.intervals;
        std::stable_sort (iv.begin(), iv.end(), [] (int a, int b) { return intervalPriority (a) < intervalPriority (b); });
        if (static_cast<int> (iv.size()) > maxNotes)
            iv.resize (static_cast<size_t> (maxNotes));

        std::vector<int> pcs;
        for (int i : iv)
        {
            const int pc = mod12 (i);
            if (std::find (pcs.begin(), pcs.end(), pc) == pcs.end())
                pcs.push_back (pc);
        }
        std::sort (pcs.begin(), pcs.end());
        return pcs;
    }

    /** Root-position close voicing whose lowest note is the root nearest `rootNear`. */
    std::vector<int> closeVoicing (const Chord& chord, int rootNear, int maxNotes = 5)
    {
        const auto pcs = chooseTones (chord, maxNotes);
        const int root = nearestWithPc (chord.root, rootNear);

        std::vector<int> notes;
        for (int pc : pcs)
            notes.push_back (root + pc);
        return notes;
    }

    std::vector<std::vector<int>> inversionsOf (const std::vector<int>& close)
    {
        std::vector<std::vector<int>> out;
        auto v = close;
        for (size_t i = 0; i < close.size(); ++i)
        {
            out.push_back (v);
            std::rotate (v.begin(), v.begin() + 1, v.end());
            v.back() += 12;
        }
        return out;
    }

    double mean (const std::vector<int>& v)
    {
        return v.empty() ? 0.0 : std::accumulate (v.begin(), v.end(), 0.0) / static_cast<double> (v.size());
    }

    double movement (const std::vector<int>& a, const std::vector<int>& b)
    {
        double d = 0.0;
        for (int n : a)
        {
            int best = 100;
            for (int m : b)
                best = std::min (best, std::abs (n - m));
            d += best;
        }
        return d / static_cast<double> (std::max<size_t> (1, a.size()));
    }

    std::vector<int> voiceLed (const Chord& chord, const std::vector<int>* previous, int center, int maxNotes)
    {
        const auto base = closeVoicing (chord, center - 5, maxNotes);
        std::vector<int> best = base;
        double bestScore = 1e9;

        for (int octave = -12; octave <= 12; octave += 12)
        {
            for (auto inv : inversionsOf (base))
            {
                for (auto& n : inv)
                    n += octave;

                const double centred = std::abs (mean (inv) - center);
                double score = previous != nullptr && ! previous->empty() ? movement (inv, *previous) * 2.0 + centred * 0.35 : centred;

                // Keep it out of the mud.
                if (inv.front() < center - 14)
                    score += 6.0;

                if (score < bestScore)
                {
                    bestScore = score;
                    best = inv;
                }
            }
        }
        return best;
    }

    int toneOrDefault (const Chord& chord, std::initializer_list<int> options, int fallback)
    {
        for (int o : options)
            for (int i : chord.intervals)
                if (i == o)
                    return o;
        return fallback;
    }

    std::vector<int> voiceChord (const Chord& chord, VoicingStyle style, const std::vector<int>* previous, int center)
    {
        if (chord.noChord)
            return {};

        switch (style)
        {
            case VoicingStyle::Close:
                return closeVoicing (chord, center - 5, 4);

            case VoicingStyle::VoiceLed:
                return voiceLed (chord, previous, center, 4);

            case VoicingStyle::Open:
            {
                auto v = closeVoicing (chord, center - 10, 4);
                for (size_t i = 1; i < v.size(); i += 2)
                    v[i] += 12;
                std::sort (v.begin(), v.end());
                return v;
            }

            case VoicingStyle::Drop2:
            case VoicingStyle::Drop3:
            {
                auto v = voiceLed (chord, previous, center + 4, 4);
                if (v.size() == 3)
                    v.push_back (v.front() + 12);
                std::sort (v.begin(), v.end());
                const size_t drop = style == VoicingStyle::Drop2 ? 2 : 3;
                if (v.size() >= drop)
                    v[v.size() - drop] -= 12;
                std::sort (v.begin(), v.end());
                return v;
            }

            case VoicingStyle::Shell:
            {
                const int root = nearestWithPc (chord.root, center - 10);
                const int third = toneOrDefault (chord, { 4, 3, 5, 2 }, 4);
                const int seventh = toneOrDefault (chord, { 10, 11, 9 }, toneOrDefault (chord, { 7, 6, 8 }, 7));
                return { root, root + third + (third < seventh ? 0 : 12), root + seventh };
            }

            case VoicingStyle::Rootless:
            {
                const int third = toneOrDefault (chord, { 4, 3, 5, 2 }, 4);
                const int fifth = toneOrDefault (chord, { 21, 7, 6, 8 }, 7);
                const bool minor = third == 3;
                const int seventh = toneOrDefault (chord, { 10, 11, 9 }, minor ? 10 : 9);
                const int ninth = toneOrDefault (chord, { 14, 13, 15 }, chord.hasInterval (6) ? 17 : 14);

                // A form: 3-5-7-9.  B form: 7-9-3-5.  Pick the one closer to the centre / previous.
                const int root = nearestWithPc (chord.root, center - 7);
                std::vector<int> a { root + third, root + mod12 (fifth) + (mod12 (fifth) < third ? 12 : 0), root + seventh, root + ninth };
                std::vector<int> b { root + seventh - 12, root + ninth - 12, root + third, root + mod12 (fifth) + (mod12 (fifth) < third ? 12 : 0) };
                std::sort (a.begin(), a.end());
                std::sort (b.begin(), b.end());

                const double sa = previous != nullptr && ! previous->empty() ? movement (a, *previous) : std::abs (mean (a) - center);
                const double sb = previous != nullptr && ! previous->empty() ? movement (b, *previous) : std::abs (mean (b) - center);
                return sa <= sb ? a : b;
            }

            case VoicingStyle::Power:
            {
                const int root = nearestWithPc (chord.root, center - 12);
                const int fifth = toneOrDefault (chord, { 7, 6, 8 }, 7);
                return { root, root + fifth, root + 12 };
            }

            case VoicingStyle::Spread:
            {
                const int root = nearestWithPc (chord.root, center - 20);
                const int third = toneOrDefault (chord, { 4, 3, 5, 2 }, 4);
                const int fifth = toneOrDefault (chord, { 7, 6, 8 }, 7);
                std::vector<int> v { root, root + fifth, root + 12 + third, root + 12 + fifth, root + 24 };
                const int top = toneOrDefault (chord, { 14, 11, 10, 9 }, -1);
                if (top >= 0)
                    v.push_back (root + 24 + mod12 (top));
                std::sort (v.begin(), v.end());
                return v;
            }

            case VoicingStyle::Lush:
            {
                auto v = voiceLed (chord, previous, center, 4);
                const bool hasNinth = chord.hasInterval (2) || chord.hasInterval (1) || chord.hasInterval (3 + 12);
                const bool diminished = chord.hasInterval (3) && chord.hasInterval (6);
                const int add = diminished ? 5 : 2; // 11th over diminished, 9th otherwise
                if (! hasNinth || diminished)
                {
                    int n = v.back();
                    while (mod12 (n) != mod12 (chord.root + add))
                        ++n;
                    if (n - v.back() < 2)
                        n += 12;
                    v.push_back (n);
                }
                return v;
            }

            case VoicingStyle::Quartal:
            {
                const int third = toneOrDefault (chord, { 4, 3, 5, 2 }, 4);
                PitchMask allowed = chord.mask() | pcBit (chord.root + 2) | pcBit (chord.root + (third == 3 ? 5 : 9));
                if (chord.hasInterval (6))
                    allowed |= pcBit (chord.root + 5);

                std::vector<int> v;
                int n = nearestWithPc (chord.root + third, center - 7);
                for (int k = 0; k < 4; ++k)
                {
                    const int snapped = snapToMask (allowed, n, 1);
                    if (v.empty() || snapped > v.back())
                        v.push_back (snapped);
                    n = (v.empty() ? n : v.back()) + 5;
                }
                return v;
            }
        }

        return closeVoicing (chord, center - 5);
    }

    std::vector<Hit> chordPattern (ChordRhythm r)
    {
        switch (r)
        {
            case ChordRhythm::Sustain:        return {};
            case ChordRhythm::BarHits:        return { { 0, 3.9, 0 } };
            case ChordRhythm::HalfNotes:      return { { 0, 1.9, 0 }, { 2, 1.9, -6 } };
            case ChordRhythm::Quarters:       return { { 0, 0.9, 0 }, { 1, 0.9, -8 }, { 2, 0.9, -4 }, { 3, 0.9, -8 } };
            case ChordRhythm::Eighths:        return { { 0, 0.45, 0 }, { 0.5, 0.45, -12 }, { 1, 0.45, -6 }, { 1.5, 0.45, -12 },
                                                       { 2, 0.45, -3 }, { 2.5, 0.45, -12 }, { 3, 0.45, -6 }, { 3.5, 0.45, -12 } };
            case ChordRhythm::Offbeats:       return { { 0.5, 0.35, 0 }, { 1.5, 0.35, -4 }, { 2.5, 0.35, 0 }, { 3.5, 0.35, -4 } };
            case ChordRhythm::PopSyncopation: return { { 0, 1.4, 0 }, { 1.5, 1.4, -6 }, { 3, 0.9, -4 } };
            case ChordRhythm::Charleston:     return { { 0, 1.4, 0 }, { 1.5, 0.5, -6 } };
            case ChordRhythm::Tresillo:       return { { 0, 0.6, 0 }, { 0.75, 0.6, -8 }, { 1.5, 0.45, -4 }, { 2, 0.6, -2 }, { 2.75, 0.6, -8 }, { 3.5, 0.45, -4 } };
            case ChordRhythm::Gospel:         return { { 0, 1.4, 0 }, { 1.5, 0.4, -10 }, { 2.5, 0.4, -6 }, { 3.5, 0.45, -2 } };
            case ChordRhythm::HouseStabs:     return { { 0.5, 0.2, 0 }, { 1.5, 0.2, -4 }, { 2.5, 0.2, 0 }, { 3.5, 0.2, -4 } };
            case ChordRhythm::Reggae:         return { { 1, 0.3, 0 }, { 3, 0.3, 0 } };
            case ChordRhythm::TranceGate:
            {
                std::vector<Hit> h;
                for (int i = 0; i < 16; ++i)
                    h.push_back ({ i * 0.25, 0.18, (i % 4 == 0) ? 0 : (i % 2 == 0 ? -10 : -18) });
                return h;
            }
            default: return {};
        }
    }

    /** Repeats a 4-beat pattern across a bar of any length. */
    std::vector<Hit> fitPatternToBar (const std::vector<Hit>& pattern, int beatsPerBar)
    {
        std::vector<Hit> out;
        for (int offset = 0; offset < beatsPerBar; offset += 4)
            for (auto h : pattern)
                if (h.pos + offset < beatsPerBar - 1e-6)
                {
                    h.pos += offset;
                    out.push_back (h);
                }
        return out;
    }

    /** Hit times inside one chord: the pattern per bar, plus the chord start if the pattern misses it. */
    std::vector<Hit> hitsForChord (const ChordEvent& e, const std::vector<Hit>& pattern, int bpb)
    {
        std::vector<Hit> hits;
        const double end = e.start + e.length;

        const int firstBar = static_cast<int> (std::floor (e.start / bpb + 1e-9));
        const int lastBar = static_cast<int> (std::ceil (end / bpb - 1e-9));

        for (int b = firstBar; b < lastBar; ++b)
        {
            for (const auto& h : pattern)
            {
                const double t = b * static_cast<double> (bpb) + h.pos;
                if (t >= e.start - 1e-6 && t < end - 1e-6)
                    hits.push_back ({ t, std::min (h.length, end - t), h.velocityOffset });
            }
        }

        const bool coversStart = std::any_of (hits.begin(), hits.end(), [&] (const Hit& h) { return h.pos < e.start + 0.26; });
        if (! coversStart)
        {
            const double nextHit = hits.empty() ? end : hits.front().pos;
            hits.insert (hits.begin(), { e.start, std::max (0.1, std::min (nextHit - e.start, 4.0) * 0.95), 0 });
        }

        return hits;
    }
} // namespace

const std::vector<std::string>& voicingNames()
{
    static const std::vector<std::string> names { "Close", "Voice-Led", "Open", "Drop 2", "Drop 3", "Shell (1-3-7)",
                                                  "Rootless (Jazz)", "Power", "Spread (Cinematic)", "Lush (add 9)", "Quartal (4ths)" };
    return names;
}

const std::vector<std::string>& chordRhythmNames()
{
    static const std::vector<std::string> names { "Sustain", "Bar Hits", "Half Notes", "Quarters", "8ths", "Off-beats",
                                                  "Pop 3+3+2", "Charleston", "Tresillo / Dembow", "Arp Up", "Arp Up-Down",
                                                  "Alberti", "Guitar Strum", "Piano Ballad", "Gospel Pushes", "House Stabs",
                                                  "Trance Gate", "Reggae Skank" };
    return names;
}

const std::vector<std::string>& bassStyleNames()
{
    static const std::vector<std::string> names { "Off", "Roots", "Root-Fifth", "Disco Octaves", "Walking (Jazz)",
                                                  "Pumping 8ths", "Trap 808", "Reggae One-Drop", "Funk 16ths",
                                                  "Arpeggio", "Tresillo" };
    return names;
}

std::vector<std::vector<int>> voiceProgression (const Progression& progression, VoicingStyle style, int center)
{
    std::vector<std::vector<int>> out;
    const std::vector<int>* previous = nullptr;

    for (const auto& e : progression.chords)
    {
        out.push_back (voiceChord (e.chord, style, previous, center));
        if (! out.back().empty())
            previous = &out.back();
    }

    return out;
}

std::vector<NoteEvent> renderChords (const Progression& progression, const AccompanimentSettings& s, std::uint32_t seed)
{
    std::vector<NoteEvent> notes;
    if (progression.empty())
        return notes;

    Rng rng (seed);
    const int bpb = std::max (1, s.beatsPerBar);
    const auto voicings = voiceProgression (progression, s.voicing, s.chordCenter);
    const auto pattern = fitPatternToBar (chordPattern (s.rhythm), bpb);

    for (size_t ci = 0; ci < progression.chords.size(); ++ci)
    {
        const auto& e = progression.chords[ci];
        const auto& v = voicings[ci];
        if (v.empty())
            continue;

        auto addNote = [&] (int pitch, double start, double length, int velOffset) {
            NoteEvent n;
            n.pitch = std::clamp (pitch, 0, 127);
            n.start = start;
            n.length = std::max (0.05, length);
            n.velocity = std::clamp (s.velocity + velOffset, 20, 127);
            notes.push_back (n);
        };

        const double end = e.start + e.length;

        switch (s.rhythm)
        {
            case ChordRhythm::Sustain:
                for (int n : v)
                    addNote (n, e.start, e.length * 0.98, 0);
                break;

            case ChordRhythm::ArpUp:
            case ChordRhythm::ArpUpDown:
            case ChordRhythm::Alberti:
            {
                std::vector<int> seq;
                if (s.rhythm == ChordRhythm::ArpUp)
                {
                    seq = v;
                }
                else if (s.rhythm == ChordRhythm::ArpUpDown)
                {
                    seq = v;
                    for (size_t i = v.size() - 1; i-- > 1;)
                        seq.push_back (v[i]);
                }
                else
                {
                    const int low = v.front();
                    const int high = v.back();
                    const int mid = v[v.size() / 2];
                    seq = { low, high, mid, high };
                }

                const double step = s.rhythm == ChordRhythm::Alberti ? 0.5 : 0.25;
                int k = 0;
                for (double t = e.start; t < end - 1e-6; t += step, ++k)
                    addNote (seq[static_cast<size_t> (k) % seq.size()], t, step * 0.95, (k % 4 == 0) ? 0 : -10);
                break;
            }

            case ChordRhythm::Strum:
            {
                // D . D U . U D U  (8ths)
                static const Hit strums[] = { { 0, 0.95, 0 }, { 1, 0.45, -6 }, { 1.5, 0.45, -14 }, { 2.5, 0.45, -14 }, { 3, 0.45, -6 }, { 3.5, 0.45, -14 } };
                const std::vector<Hit> pat (std::begin (strums), std::end (strums));
                const auto hits = hitsForChord (e, fitPatternToBar (pat, bpb), bpb);
                int idx = 0;
                for (const auto& h : hits)
                {
                    const bool down = idx++ % 2 == 0 || h.velocityOffset > -10;
                    for (size_t i = 0; i < v.size(); ++i)
                    {
                        const size_t string = down ? i : v.size() - 1 - i;
                        const double offset = static_cast<double> (i) * 0.018;
                        addNote (v[string], h.pos + offset, std::max (0.05, h.length - offset), h.velocityOffset - static_cast<int> (i) * 2);
                    }
                }
                break;
            }

            case ChordRhythm::PianoBallad:
            {
                const int bass = v.front() - 12;
                std::vector<Hit> lh { { 0, 1.95, 0 }, { 2, 1.95, -6 } };
                for (const auto& h : hitsForChord (e, fitPatternToBar (lh, bpb), bpb))
                    addNote (bass, h.pos, h.length, h.velocityOffset - 4);

                std::vector<Hit> rh { { 0, 0.95, 0 } };
                for (const auto& h : hitsForChord (e, fitPatternToBar (rh, bpb), bpb))
                    for (int n : v)
                        addNote (n, h.pos, h.length, h.velocityOffset - 6);

                std::vector<Hit> fill { { 1, 0.45, -14 }, { 1.5, 0.45, -16 }, { 2.5, 0.45, -14 }, { 3, 0.45, -16 }, { 3.5, 0.45, -14 } };
                int k = 0;
                for (const auto& h : hitsForChord (e, fitPatternToBar (fill, bpb), bpb))
                {
                    if (h.pos < e.start + 0.5)
                        continue;
                    addNote (v[static_cast<size_t> (k++ % static_cast<int> (v.size()))], h.pos, h.length, h.velocityOffset);
                }
                break;
            }

            default:
            {
                for (const auto& h : hitsForChord (e, pattern, bpb))
                    for (int n : v)
                        addNote (n, h.pos, h.length, h.velocityOffset);
                break;
            }
        }
    }

    detail::applySwing (notes, s.swing, false);
    detail::applyHumanize (notes, s.humanize, rng);
    std::stable_sort (notes.begin(), notes.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
    return notes;
}

std::vector<NoteEvent> renderBass (const Progression& progression, const AccompanimentSettings& s, std::uint32_t seed)
{
    std::vector<NoteEvent> notes;
    if (progression.empty() || s.bass == BassStyle::Off)
        return notes;

    Rng rng (seed ^ 0xb5297a4du);
    const int bpb = std::max (1, s.beatsPerBar);
    const int lowLimit = 24;
    const int highLimit = 60;

    auto clampBass = [&] (int n) {
        while (n < lowLimit) n += 12;
        while (n > highLimit) n -= 12;
        return n;
    };

    auto add = [&] (int pitch, double start, double length, int velOffset) {
        NoteEvent n;
        n.pitch = clampBass (pitch);
        n.start = start;
        n.length = std::max (0.05, length);
        n.velocity = std::clamp (s.velocity + 6 + velOffset, 20, 127);
        notes.push_back (n);
    };

    for (size_t ci = 0; ci < progression.chords.size(); ++ci)
    {
        const auto& e = progression.chords[ci];
        if (e.chord.noChord)
            continue;

        const int root = nearestWithPc (e.chord.bassPc(), s.bassCenter);
        const int fifthIv = toneOrDefault (e.chord, { 7, 6, 8 }, 7);
        const int thirdIv = toneOrDefault (e.chord, { 4, 3, 5, 2 }, 4);
        const int seventhIv = toneOrDefault (e.chord, { 10, 11, 9 }, 12);
        const int fifth = root + fifthIv - (root + fifthIv > s.bassCenter + 7 ? 12 : 0);
        const bool hasNext = ci + 1 < progression.chords.size();
        const int nextRoot = hasNext ? nearestWithPc (progression.chords[ci + 1].chord.bassPc(), root) : root;

        auto pattern = [&] (std::vector<Hit> pat, auto pitchFor) {
            int k = 0;
            for (const auto& h : hitsForChord (e, fitPatternToBar (pat, bpb), bpb))
                add (pitchFor (k++, h), h.pos, h.length, h.velocityOffset);
        };

        switch (s.bass)
        {
            case BassStyle::Off:
                break;

            case BassStyle::Roots:
                pattern ({ { 0, 3.9, 0 } }, [&] (int, const Hit&) { return root; });
                break;

            case BassStyle::RootFifth:
                pattern ({ { 0, 1.9, 0 }, { 2, 1.9, -6 } }, [&] (int k, const Hit&) { return k % 2 == 0 ? root : fifth; });
                break;

            case BassStyle::Octaves:
            {
                std::vector<Hit> pat;
                for (int i = 0; i < 8; ++i)
                    pat.push_back ({ i * 0.5, 0.4, i % 2 == 0 ? 0 : -8 });
                pattern (pat, [&] (int k, const Hit&) { return k % 2 == 0 ? root : root + 12; });
                break;
            }

            case BassStyle::Pumping:
            {
                std::vector<Hit> pat;
                for (int i = 0; i < 8; ++i)
                    pat.push_back ({ i * 0.5, 0.42, i % 2 == 0 ? 0 : -10 });
                pattern (pat, [&] (int, const Hit&) { return root; });
                break;
            }

            case BassStyle::Walking:
            {
                int prev = root;
                const int beats = static_cast<int> (std::floor (e.length + 1e-6));
                for (int b = 0; b < beats; ++b)
                {
                    int pitch;
                    if (b == 0)
                        pitch = root;
                    else if (b == beats - 1 && hasNext)
                    {
                        // Approach the next root: chromatic, scale step or its fifth.
                        const double r = rng.uniform();
                        pitch = r < 0.5 ? nextRoot + (prev > nextRoot ? 1 : -1) : (r < 0.8 ? nextRoot + (prev > nextRoot ? 2 : -2) : nextRoot + 7 - 12);
                    }
                    else
                    {
                        static const int choices[] = { 0, 1, 2, 3 };
                        const int c = choices[rng.range (0, 3)];
                        const int iv = c == 0 ? thirdIv : (c == 1 ? fifthIv : (c == 2 ? (seventhIv == 12 ? 9 : seventhIv) : 12));
                        pitch = root + iv;
                        if (std::abs (pitch - prev) > 7)
                            pitch -= 12;
                    }
                    add (pitch, e.start + b, 0.9, b == 0 ? 0 : -6);
                    prev = clampBass (pitch);
                }
                break;
            }

            case BassStyle::Trap808:
            {
                const bool octaveHit = rng.chance (0.5);
                pattern ({ { 0, 1.45, 0 }, { 1.5, 0.95, -4 }, { 2.5, 0.95, -2 }, { 3.5, 0.45, -8 } },
                         [&] (int k, const Hit& h) {
                             const double pos = std::fmod (h.pos, static_cast<double> (bpb));
                             return (octaveHit && k > 0 && pos > 3.0) ? root + 12 : root;
                         });
                break;
            }

            case BassStyle::Reggae:
                pattern ({ { 0, 0.9, 0 }, { 1.5, 0.4, -8 }, { 2.5, 0.4, -6 }, { 3, 0.9, -4 } },
                         [&] (int k, const Hit&) { return k % 4 == 2 ? fifth : root; });
                break;

            case BassStyle::Funk:
                pattern ({ { 0, 0.22, 0 }, { 0.75, 0.2, -10 }, { 1.5, 0.2, -4 }, { 2, 0.22, -2 }, { 2.5, 0.2, -8 }, { 3.25, 0.2, -10 }, { 3.75, 0.2, -6 } },
                         [&] (int k, const Hit&) {
                             switch (k % 7)
                             {
                                 case 2: return root + 12;
                                 case 4: return root + (seventhIv == 12 ? fifthIv : seventhIv);
                                 case 6: return hasNext ? nextRoot - 1 : root;
                                 default: return root;
                             }
                         });
                break;

            case BassStyle::Arpeggio:
                pattern ({ { 0, 0.9, 0 }, { 1, 0.9, -6 }, { 2, 0.9, -4 }, { 3, 0.9, -6 } },
                         [&] (int k, const Hit&) {
                             static const int order[4] = { 0, 1, 2, 1 };
                             const int which = order[k % 4];
                             return which == 0 ? root : (which == 1 ? root + thirdIv : root + fifthIv);
                         });
                break;

            case BassStyle::Tresillo:
                pattern ({ { 0, 0.6, 0 }, { 0.75, 0.6, -8 }, { 1.5, 0.4, -4 }, { 2, 0.6, -2 }, { 2.75, 0.6, -8 }, { 3.5, 0.4, -4 } },
                         [&] (int, const Hit&) { return root; });
                break;
        }

    }

    detail::applySwing (notes, s.swing, false);
    detail::applyHumanize (notes, s.humanize * 0.5, rng);
    std::stable_sort (notes.begin(), notes.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });
    return notes;
}

} // namespace melodee
