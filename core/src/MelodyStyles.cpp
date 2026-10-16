#include "MelodyInternal.h"

#include "melodee/Accompaniment.h"

#include <algorithm>
#include <cmath>

namespace melodee
{

namespace detail
{
    namespace
    {
        // name, description,
        // density, syncopation, leaps, chordTones, repetition, rests, swing, noteLength,
        // grid, repeatNote, extensions, range, contour, form, flags, velocity, patterns
        const std::vector<StyleProfile>& profiles()
        {
            static const std::vector<StyleProfile> list {
                { "Lyrical", "Singable top line: mostly steps, chord tones on strong beats, breathing phrases.",
                  0.40, 0.25, 0.25, 0.70, 0.55, 0.30, 0.0, 0.85,
                  Grid::Sixteenth, 0.25, 0.20, 14, Contour::Arch, PhraseForm::ABAC, FlagSequence, 90,
                  { "x...x...x.x.x...", "x.....x.x...x...", "x...x.x.x.......", "x.x.x...x...x...", "..x.x.x.x...x...", "x...x...x...x.x." } },

                { "Pop Hook", "Catchy, syncopated, repetitive hook built from one short idea.",
                  0.50, 0.60, 0.30, 0.65, 0.80, 0.30, 0.0, 0.75,
                  Grid::Sixteenth, 0.55, 0.15, 12, Contour::Wave, PhraseForm::AABA, FlagSequence, 96,
                  { "x..x..x.........", "x..x..x...x.x...", "..x.x.x...x.x...", "x.x...x.x.x.....", "...x..x.x..x....", "x..x..x.x..x..x.", "x.x.x..x..x....." } },

                { "Chord Tones", "Guide-tone melody: 3rds and 7ths on the beat, outlining every chord change.",
                  0.30, 0.15, 0.35, 0.95, 0.40, 0.15, 0.0, 0.95,
                  Grid::Sixteenth, 0.20, 0.10, 15, Contour::Arch, PhraseForm::Through, 0, 88,
                  { "x.......x.......", "x...x...x...x...", "x.....x.x.......", "x.......x...x..." } },

                { "Arpeggio", "Broken-chord patterns (up, down, Alberti, broken) that voice-lead between chords.",
                  0.70, 0.10, 0.60, 1.00, 0.60, 0.05, 0.0, 0.70,
                  Grid::Sixteenth, 0.0, 0.40, 19, Contour::Wave, PhraseForm::Repeat, FlagArpeggio, 92,
                  {} },

                { "Scalar Runs", "Flowing scale runs that connect chord tones.",
                  0.75, 0.20, 0.10, 0.60, 0.30, 0.15, 0.0, 0.85,
                  Grid::Sixteenth, 0.05, 0.20, 17, Contour::Wave, PhraseForm::Through, FlagScalarRuns, 90,
                  { "x.x.x.x.x.x.x.x.", "x.xxx.x.x.xxx.x.", "x...x.x.xxxxx...", "x.x.xxxxx...x..." } },

                { "Motif Development", "One short motif, then sequenced, inverted and reversed across the chords.",
                  0.45, 0.40, 0.35, 0.70, 0.85, 0.25, 0.0, 0.80,
                  Grid::Sixteenth, 0.30, 0.20, 14, Contour::Rising, PhraseForm::ABAC, FlagMotifDevelop | FlagSequence, 92,
                  { "x.x.x...x.......", "x..x..x.x.......", "x.xx..x.x.......", "..x.x.x.x......." } },

                { "Call & Response", "A busy question phrase answered by a calmer, resolving phrase.",
                  0.50, 0.40, 0.30, 0.70, 0.70, 0.45, 0.0, 0.80,
                  Grid::Sixteenth, 0.30, 0.15, 14, Contour::Wave, PhraseForm::CallResponse, FlagCallResponse, 92,
                  { "x.x.x.x.x.......", "x..x..x.x.x.....", "..x.x.x.x.x....." } },

                { "Jazz Bebop", "Swung 8th-note lines with enclosures and chromatic approach notes.",
                  0.80, 0.30, 0.35, 0.70, 0.20, 0.15, 0.65, 0.85,
                  Grid::Eighth, 0.05, 0.50, 19, Contour::Wave, PhraseForm::Through, FlagBebop, 88,
                  {} },

                { "Blues", "Blue notes, slides and call-and-response licks over a shuffle.",
                  0.45, 0.45, 0.35, 0.60, 0.65, 0.40, 0.60, 0.80,
                  Grid::Eighth, 0.40, 0.10, 13, Contour::Falling, PhraseForm::CallResponse, FlagBlueNotes | FlagGraceNotes | FlagPentatonic, 95,
                  {} },

                { "Pentatonic Riff", "Repeating riff from the pentatonic scale. Works in almost any genre.",
                  0.55, 0.45, 0.40, 0.60, 0.75, 0.30, 0.0, 0.70,
                  Grid::Sixteenth, 0.35, 0.0, 12, Contour::Wave, PhraseForm::Repeat, FlagPentatonic, 96,
                  { "x..x..x.x.x.....", "x.x..x.x..x.x...", "x..x.x..x..x.x..", "..x.x..x.x..x..." } },

                { "Trap / Drill", "Dark, repetitive melody with triplet rolls, pockets of space and octave jumps.",
                  0.45, 0.55, 0.30, 0.60, 0.80, 0.45, 0.0, 0.60,
                  Grid::Mixed, 0.70, 0.05, 12, Contour::Wave, PhraseForm::Repeat, FlagRolls | FlagOctaveJumps, 100,
                  { "x.....x...x.....", "x..x..x.....x...", "x.......x.x.x...", "x...x..x...x.x..", "x..x....x..x.x..", "x.x...x...x....." } },

                { "R&B / Neo-Soul", "Rich 9ths, 11ths and 13ths, laid-back phrasing and vocal-style runs.",
                  0.45, 0.55, 0.40, 0.55, 0.50, 0.35, 0.35, 0.85,
                  Grid::Sixteenth, 0.20, 0.90, 15, Contour::Arch, PhraseForm::ABAC, FlagRuns | FlagSwing16, 85,
                  { "x..x..x...x.....", "..x.x..x.x......", "x...x..x..x.x...", "...x..x.x...x..." } },

                { "EDM Lead", "Syncopated pluck / lead patterns with octave jumps. Built to loop.",
                  0.65, 0.70, 0.45, 0.75, 0.85, 0.25, 0.0, 0.55,
                  Grid::Sixteenth, 0.35, 0.10, 14, Contour::Wave, PhraseForm::Repeat, FlagOctaveJumps, 105,
                  { "x.x..x.x..x.x...", "..x...x...x...x.", "x..x..x..x..x.x.", "x.xx.x.xx.x.x.x.", "x..x..x.x..x..x.", "..x..x..x..x.x.." } },

                { "Lo-Fi", "Lazy swing, lots of space, 7ths and 9ths. Relaxed and nostalgic.",
                  0.35, 0.50, 0.35, 0.60, 0.60, 0.50, 0.55, 0.80,
                  Grid::Sixteenth, 0.30, 0.80, 12, Contour::Falling, PhraseForm::ABAB, FlagSwing16, 78,
                  { "x.....x...x.....", "..x...x.x.......", "x...x.....x.x...", "...x..x.....x..." } },

                { "Ballad", "Long, expressive notes with wide, resolving leaps.",
                  0.25, 0.20, 0.45, 0.75, 0.50, 0.25, 0.0, 1.00,
                  Grid::Sixteenth, 0.20, 0.30, 17, Contour::Arch, PhraseForm::ABAC, FlagLongTones, 82,
                  { "x.......x.......", "x.....x.x.......", "x...x...x.......", "x.......x...x..." } },

                { "Classical", "Periodic phrases, sequences and clear cadences with light ornaments.",
                  0.50, 0.10, 0.35, 0.75, 0.70, 0.15, 0.0, 0.85,
                  Grid::Sixteenth, 0.15, 0.0, 17, Contour::Arch, PhraseForm::ABAC, FlagSequence | FlagGraceNotes, 88,
                  { "x...x...x...x...", "x.x.x.x.x...x...", "x..xx...x.x.x...", "x.xxx.x.x...x...", "x...x.x.x.x.x..." } },

                { "Latin / Reggaeton", "Dembow and tresillo rhythms (3+3+2) with punchy repeated notes.",
                  0.50, 0.60, 0.30, 0.70, 0.80, 0.30, 0.0, 0.70,
                  Grid::Sixteenth, 0.45, 0.10, 12, Contour::Wave, PhraseForm::Repeat, 0, 98,
                  { "x..x..x.x..x..x.", "x..x..x...x.x...", "...x..x....x..x.", "x..x..x.x..x.x..", "x..x..x........." } },

                { "Afrobeats", "Bouncy syncopation, pentatonic flavour and call-and-response.",
                  0.50, 0.65, 0.30, 0.65, 0.75, 0.35, 0.20, 0.70,
                  Grid::Sixteenth, 0.45, 0.20, 12, Contour::Wave, PhraseForm::CallResponse, FlagPentatonic | FlagSwing16, 95,
                  { "x..x..x...x..x..", "..x..x..x.x..x..", "x..x...x..x.x...", "x.x..x..x..x.x.." } },

                { "Rock Riff", "Driving riff around the root with pentatonic and blues notes.",
                  0.55, 0.45, 0.45, 0.55, 0.85, 0.30, 0.0, 0.60,
                  Grid::Eighth, 0.50, 0.0, 12, Contour::Flat, PhraseForm::Repeat, FlagPentatonic | FlagBlueNotes, 105,
                  {} },

                { "Cinematic", "Long, wide, rising lines with octave leaps at the climax.",
                  0.25, 0.15, 0.55, 0.75, 0.60, 0.15, 0.0, 1.00,
                  Grid::Sixteenth, 0.10, 0.50, 20, Contour::Rising, PhraseForm::ABAC, FlagLongTones | FlagOctaveJumps, 90,
                  { "x.......x.......", "x.....x.x.......", "x...........x...", "x.......x...x.x." } },

                { "Ambient", "Sparse, floating tones that favour colour notes (9ths, 11ths, 13ths).",
                  0.12, 0.10, 0.50, 0.50, 0.40, 0.45, 0.0, 1.00,
                  Grid::Sixteenth, 0.10, 1.00, 20, Contour::Wave, PhraseForm::Through, FlagLongTones, 70,
                  { "x...............", "x.......x.......", "....x..........." } },

                { "Counter Melody", "A second line that moves against the chords' top voice. Layer it under a lead.",
                  0.30, 0.25, 0.30, 0.80, 0.50, 0.30, 0.0, 0.95,
                  Grid::Sixteenth, 0.20, 0.40, 12, Contour::Auto, PhraseForm::Through, FlagCounter, 80,
                  { "x.......x.......", "x...x...x.......", "....x...x...x..." } },

                { "Ornamental", "Grace notes, mordents, trills and turns: classical, Celtic or Middle Eastern flavour.",
                  0.40, 0.25, 0.30, 0.70, 0.60, 0.20, 0.0, 0.85,
                  Grid::Sixteenth, 0.20, 0.10, 14, Contour::Arch, PhraseForm::ABAC, FlagOrnaments | FlagGraceNotes, 90,
                  { "x...x...x.x.x...", "x.....x.x...x...", "x...x.x.x......." } },
            };
            return list;
        }
    } // namespace

    const StyleProfile& styleProfile (MelodyStyle style)
    {
        const auto& list = profiles();
        auto idx = static_cast<size_t> (style);
        if (idx >= list.size())
            idx = 0;
        return list[idx];
    }

    const std::vector<MelodyStyle>& mixedStyleCycle()
    {
        static const std::vector<MelodyStyle> cycle {
            MelodyStyle::Lyrical, MelodyStyle::PopHook, MelodyStyle::MotifDevelopment, MelodyStyle::CallResponse,
            MelodyStyle::ChordTones, MelodyStyle::Arpeggio, MelodyStyle::RnBNeoSoul, MelodyStyle::Pentatonic,
            MelodyStyle::EDMLead, MelodyStyle::Classical, MelodyStyle::LoFi, MelodyStyle::Ballad,
            MelodyStyle::ScalarRuns, MelodyStyle::Cinematic, MelodyStyle::Latin, MelodyStyle::Afrobeats,
            MelodyStyle::Ornamental,
        };
        return cycle;
    }

    Effective effectiveSettings (const StyleProfile& p, const MelodySettings& s)
    {
        auto mix = [] (double base, double amount) { return std::clamp (base + (amount - 0.5), 0.0, 1.0); };

        Effective e;
        e.density = mix (p.density, s.density);
        e.syncopation = mix (p.syncopation, s.syncopation);
        e.leaps = mix (p.leaps, s.leaps);
        e.chordTones = mix (p.chordTones, s.chordTones);
        e.repetition = mix (p.repetition, s.repetition);
        e.rests = mix (p.rests, s.rests);
        e.swing = mix (p.swing, s.swing);
        e.articulation = 0.3 + 0.7 * mix (p.noteLength, s.noteLength);
        return e;
    }

    double metricStrength (double pos, int beatsPerBar) noexcept
    {
        constexpr double eps = 1e-4;
        if (pos < eps)
            return 1.0;

        const double frac = pos - std::floor (pos + eps);
        if (std::abs (frac) < eps)
        {
            if (beatsPerBar % 2 == 0 && std::abs (pos - beatsPerBar / 2.0) < eps)
                return 0.8;
            return 0.65;
        }
        if (std::abs (frac - 0.5) < eps)
            return 0.42;
        if (std::abs (frac - 1.0 / 3.0) < eps || std::abs (frac - 2.0 / 3.0) < eps)
            return 0.32;
        return 0.2;
    }

    int contextIndexAt (const Progression& prog, double t) noexcept
    {
        if (prog.chords.empty())
            return -1;

        const int i = prog.indexAt (t);
        if (i >= 0)
            return i;

        return t <= 0.0 ? 0 : static_cast<int> (prog.chords.size()) - 1;
    }

    int fitPitchToChord (int pitch, const ChordContext& ctx, double strength, int preferDirection) noexcept
    {
        if (strength >= 0.6)
        {
            if (! maskHas (ctx.chordTones, pitch) && ! maskHas (ctx.colour, pitch))
                return snapToMask (ctx.core, pitch, preferDirection);
            return pitch;
        }

        if (! maskHas (ctx.chordScale, pitch))
            return snapToMask (ctx.chordScale, pitch, preferDirection);

        return pitch;
    }

    void applySwing (std::vector<NoteEvent>& notes, double amount, bool sixteenths)
    {
        if (amount <= 0.001)
            return;

        constexpr double eps = 1e-4;
        const double unit = sixteenths ? 0.5 : 1.0;          // swing pair length in beats
        const double shift = amount * unit / 6.0;            // up to a triplet feel

        for (auto& n : notes)
        {
            const double inPair = std::fmod (n.start, unit);
            if (std::abs (inPair - unit / 2.0) < eps)
            {
                n.start += shift;
                n.length = std::max (0.05, n.length - shift * 0.5);
            }
        }
    }

    void applyHumanize (std::vector<NoteEvent>& notes, double amount, Rng& rng)
    {
        if (amount <= 0.0)
            return;

        for (auto& n : notes)
        {
            if (n.start > 0.01)
                n.start = std::max (0.0, n.start + rng.uniform (-1.0, 1.0) * 0.02 * amount);
            n.velocity = std::clamp (n.velocity + static_cast<int> (std::lround (rng.uniform (-1.0, 1.0) * 14.0 * amount)), 20, 127);
        }
    }

    void finaliseMonophonic (std::vector<NoteEvent>& notes, double totalBeats, int lo, int hi)
    {
        std::stable_sort (notes.begin(), notes.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });

        std::vector<NoteEvent> out;
        out.reserve (notes.size());

        for (auto n : notes)
        {
            if (n.start >= totalBeats - 0.01)
                continue;

            while (n.pitch < lo) n.pitch += 12;
            while (n.pitch > hi) n.pitch -= 12;

            if (! out.empty() && n.start - out.back().start < 0.02)
            {
                out.back() = n; // two notes at the same time: keep the later decision
                continue;
            }
            out.push_back (n);
        }

        for (size_t i = 0; i < out.size(); ++i)
        {
            const double limit = (i + 1 < out.size() ? out[i + 1].start : totalBeats) - out[i].start;
            out[i].length = std::clamp (out[i].length, 0.04, std::max (0.04, limit - 0.005));
        }

        notes = std::move (out);
    }

    PitchMask pentatonicOf (const Scale& key) noexcept
    {
        const int r = key.root();
        if (key.isMinor())
            return static_cast<PitchMask> (pcBit (r) | pcBit (r + 3) | pcBit (r + 5) | pcBit (r + 7) | pcBit (r + 10));
        return static_cast<PitchMask> (pcBit (r) | pcBit (r + 2) | pcBit (r + 4) | pcBit (r + 7) | pcBit (r + 9));
    }

    PitchMask blueNotesOf (const Scale& key) noexcept
    {
        const int r = key.root();
        return static_cast<PitchMask> (pcBit (r + 3) | pcBit (r + 6) | pcBit (r + 10));
    }

    std::vector<int> topVoices (const Progression& prog, int center)
    {
        std::vector<int> tops;
        for (const auto& v : voiceProgression (prog, VoicingStyle::VoiceLed, center))
            tops.push_back (v.empty() ? center : v.back());
        return tops;
    }
} // namespace detail

//==============================================================================
const std::vector<std::string>& melodyStyleNames()
{
    static const std::vector<std::string> names = [] {
        std::vector<std::string> n;
        for (int i = 0; i < static_cast<int> (MelodyStyle::Mixed); ++i)
            n.push_back (detail::styleProfile (static_cast<MelodyStyle> (i)).name);
        n.push_back ("Mixed (every style)");
        return n;
    }();
    return names;
}

std::string melodyStyleDescription (MelodyStyle style)
{
    if (style == MelodyStyle::Mixed)
        return "Every variation uses a different style so you can compare approaches.";
    return detail::styleProfile (style).description;
}

MelodyStyle melodyStyleFromName (std::string_view name)
{
    const auto& names = melodyStyleNames();
    const auto target = toLower (trim (name));

    for (size_t i = 0; i < names.size(); ++i)
    {
        const auto n = toLower (names[i]);
        if (n == target || (target.size() >= 3 && n.find (target) == 0))
            return static_cast<MelodyStyle> (i);
    }

    // Loose matching on single words ("trap", "jazz", "rnb", "edm", "latin"...).
    for (size_t i = 0; i < names.size(); ++i)
    {
        auto n = toLower (names[i]);
        n.erase (std::remove_if (n.begin(), n.end(), [] (char c) { return c == '&' || c == '-' || c == '/'; }), n.end());
        auto t = target;
        t.erase (std::remove_if (t.begin(), t.end(), [] (char c) { return c == '&' || c == '-' || c == '/'; }), t.end());
        if (! t.empty() && n.find (t) != std::string::npos)
            return static_cast<MelodyStyle> (i);
    }

    return MelodyStyle::Mixed;
}

int numMelodyStyles() noexcept { return static_cast<int> (MelodyStyle::Mixed) + 1; }

const std::vector<std::string>& contourNames()
{
    static const std::vector<std::string> names { "Auto", "Arch", "Valley", "Rising", "Falling", "Wave", "Flat" };
    return names;
}

const std::vector<std::string>& phraseFormNames()
{
    static const std::vector<std::string> names { "Auto", "Repeat (AAAA')", "AABA", "ABAB'", "AABB", "ABAC", "Call & Response", "Through-composed" };
    return names;
}

} // namespace melodee
