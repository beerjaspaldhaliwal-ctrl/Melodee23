#include "MelodyInternal.h"

#include <algorithm>
#include <cmath>

namespace melodee
{

using namespace detail;

namespace
{
    constexpr double pi = 3.14159265358979323846;

    struct Onset
    {
        double pos;          // beats from the start of the bar
        double lengthFactor; // fraction of the gap to the next note
    };

    using Rhythm = std::vector<Onset>;

    struct BarRole
    {
        bool phraseStart = false;
        bool phraseEnd = false;
        bool finalBar = false;
        bool call = false;
        bool response = false;
        char label = 'A';
        bool variant = false;
    };

    struct Motif
    {
        Rhythm rhythm;
        std::vector<int> pitches;
        int bar = -1;
    };

    enum class MotifOp { Repeat, Sequence, Inversion, Retrograde };

    /** A note being built: the length is a factor until durations are resolved. */
    struct Draft
    {
        int pitch;
        double start;
        double lengthFactor;
        double strength;
        bool phraseLast;
    };

    int sign (int x) noexcept { return (x > 0) - (x < 0); }

    double contourShape (Contour c, double x) noexcept
    {
        switch (c)
        {
            case Contour::Arch:    return -0.7 + 1.5 * std::sin (pi * x);
            case Contour::Valley:  return 0.7 - 1.5 * std::sin (pi * x);
            case Contour::Rising:  return -0.8 + 1.6 * x;
            case Contour::Falling: return 0.8 - 1.6 * x;
            case Contour::Wave:    return 0.75 * std::sin (2.0 * pi * x);
            case Contour::Flat:    return -0.1;
            case Contour::Auto:    break;
        }
        return 0.0;
    }

    class Generator
    {
    public:
        Generator (const Progression& progression, const Scale& keyScale, const MelodySettings& settings,
                   MelodyStyle melodyStyle, std::uint32_t seedValue)
            : prog (progression), key (keyScale), s (settings), style (melodyStyle),
              p (styleProfile (melodyStyle)), e (effectiveSettings (p, settings)), rng (seedValue), seed (seedValue)
        {
        }

        Melody run()
        {
            Melody m;
            m.seed = seed;
            m.style = p.name;

            if (prog.empty() || prog.totalBeats <= 0.0)
                return m;

            setup();

            if ((p.flags & FlagArpeggio) != 0)
                buildArpeggio();
            else
                buildPhrased();

            if ((p.flags & FlagBebop) != 0)
                addBebopApproaches();

            auto notes = resolveDurations();

            const EmbellishContext ec { prog, ctxs, bpb, total, lo, hi, pentaMask };
            if ((p.flags & FlagGraceNotes) != 0)
                addGraceNotes (notes, ec, rng, (p.flags & FlagBlueNotes) != 0);
            if ((p.flags & FlagOrnaments) != 0)
                addOrnaments (notes, ec, rng, 1.0);
            if ((p.flags & FlagRuns) != 0)
                addRuns (notes, ec, rng);
            if ((p.flags & FlagRolls) != 0)
                addRolls (notes, ec, rng);

            resolveEnding (notes);
            applySwing (notes, e.swing, (p.flags & FlagSwing16) != 0);
            applyHumanize (notes, s.humanize, rng);
            finaliseMonophonic (notes, total, s.lowNote, std::max (s.highNote, s.lowNote + 12));

            m.notes = std::move (notes);
            m.description = std::string (p.name) + " \xC2\xB7 " + contourNames()[static_cast<size_t> (contour)] + " contour \xC2\xB7 "
                          + phraseFormNames()[static_cast<size_t> (form)] + " \xC2\xB7 seed " + std::to_string (seed);
            return m;
        }

    private:
        const Progression& prog;
        const Scale& key;
        MelodySettings s;
        MelodyStyle style;
        const StyleProfile& p;
        Effective e;
        Rng rng;
        std::uint32_t seed;

        std::vector<ChordContext> ctxs;
        int bpb = 4;
        double total = 0.0;
        int numBars = 0;
        int lo = 60, hi = 84;
        Contour contour = Contour::Arch;
        PhraseForm form = PhraseForm::ABAC;
        int phraseLen = 4;
        PitchMask keyMask = 0, pentaMask = 0, blueMask = 0;
        std::vector<int> tops;
        double topMean = 72.0;

        std::vector<Draft> drafts;
        Motif motifs[26];

        //======================================================================
        void setup()
        {
            bpb = std::max (1, s.beatsPerBar);
            total = prog.totalBeats;
            numBars = std::max (1, static_cast<int> (std::ceil (total / bpb - 1e-9)));
            ctxs = analyseProgression (prog, key, (p.flags & FlagBlueNotes) != 0);
            keyMask = key.mask();
            pentaMask = pentatonicOf (key);
            blueMask = blueNotesOf (key);

            // Melodic window inside the user's range.
            const int userLo = std::clamp (s.lowNote, 0, 120);
            const int userHi = std::clamp (std::max (s.highNote, userLo + 12), userLo + 12, 127);
            const int userSpan = userHi - userLo;
            int span = p.rangeSemis + static_cast<int> (std::lround ((e.leaps - 0.5) * 8.0));
            span = std::clamp (span, 9, userSpan);
            const int centre = (userLo + userHi) / 2 + rng.range (-2, 2);
            lo = std::clamp (centre - span / 2, userLo, userHi - span);
            hi = lo + span;

            // Contour and form: "Auto" picks the style's own most of the time, something else for variety.
            contour = s.contour;
            if (contour == Contour::Auto)
            {
                contour = p.contour;
                if (contour == Contour::Auto || rng.chance (0.4))
                {
                    static const Contour options[] = { Contour::Arch, Contour::Wave, Contour::Rising, Contour::Falling, Contour::Valley };
                    contour = options[rng.range (0, 4)];
                }
            }

            form = s.form;
            if (form == PhraseForm::Auto)
            {
                form = p.form;
                if (rng.chance (0.3) && (p.flags & FlagCallResponse) == 0)
                {
                    static const PhraseForm options[] = { PhraseForm::AABA, PhraseForm::ABAB, PhraseForm::ABAC, PhraseForm::Repeat, PhraseForm::AABB };
                    form = options[rng.range (0, 4)];
                }
            }

            phraseLen = numBars >= 4 ? 4 : numBars;

            if ((p.flags & FlagCounter) != 0)
            {
                tops = topVoices (prog, 64);
                double sum = 0.0;
                for (int t : tops)
                    sum += t;
                topMean = tops.empty() ? 64.0 : sum / static_cast<double> (tops.size());
            }
        }

        const ChordContext& ctxAt (double t) const { return ctxs[static_cast<size_t> (contextIndexAt (prog, t))]; }

        double contourTarget (double t) const
        {
            const double phraseBeats = static_cast<double> (phraseLen * bpb);
            const double x = phraseBeats > 0.0 ? std::fmod (t, phraseBeats) / phraseBeats : 0.0;
            const double centre = (lo + hi) / 2.0;
            const double amp = (hi - lo) / 2.0;

            double target = centre + amp * 0.8 * contourShape (contour, x);

            // Rising/Falling also drift across the whole melody (later phrases climb higher).
            if (contour == Contour::Rising)
                target += amp * 0.3 * (t / total - 0.5);
            else if (contour == Contour::Falling)
                target -= amp * 0.3 * (t / total - 0.5);

            if ((p.flags & FlagCounter) != 0 && ! tops.empty())
            {
                const int ci = contextIndexAt (prog, t);
                target = centre - (tops[static_cast<size_t> (ci)] - topMean) * 0.8;
            }

            return std::clamp (target, static_cast<double> (lo), static_cast<double> (hi));
        }

        //======================================================================
        std::vector<BarRole> barRoles() const
        {
            const char* pattern = "ABAC";
            switch (form)
            {
                case PhraseForm::Repeat:       pattern = "AAAa"; break;
                case PhraseForm::AABA:         pattern = "AABA"; break;
                case PhraseForm::ABAB:         pattern = "ABAb"; break;
                case PhraseForm::AABB:         pattern = "AABB"; break;
                case PhraseForm::ABAC:         pattern = "ABAC"; break;
                case PhraseForm::CallResponse: pattern = "ABAb"; break;
                case PhraseForm::Through:      pattern = "ABCD"; break;
                case PhraseForm::Auto:         break;
            }

            const bool callResponse = form == PhraseForm::CallResponse || (p.flags & FlagCallResponse) != 0;
            const int numPhrases = (numBars + phraseLen - 1) / phraseLen;

            std::vector<BarRole> roles (static_cast<size_t> (numBars));
            for (int b = 0; b < numBars; ++b)
            {
                auto& r = roles[static_cast<size_t> (b)];
                const int phrase = b / phraseLen;
                const int inPhrase = b % phraseLen;

                r.phraseStart = inPhrase == 0;
                r.phraseEnd = inPhrase == phraseLen - 1 || b == numBars - 1;
                r.finalBar = b == numBars - 1;
                r.call = callResponse && inPhrase % 2 == 0;
                r.response = callResponse && inPhrase % 2 == 1;

                char ch = pattern[inPhrase % 4];
                if (form == PhraseForm::Through)
                    ch = static_cast<char> ('A' + (b % 26));

                r.variant = ch >= 'a' && ch <= 'z';
                r.label = static_cast<char> (std::toupper (static_cast<unsigned char> (ch)));

                // The last bar of the last phrase always gets a fresh cadence.
                if (phrase == numPhrases - 1 && r.finalBar && phrase > 0)
                    r.variant = true;
            }
            return roles;
        }

        //======================================================================
        Rhythm makeRhythm (double barLen, const BarRole& role)
        {
            Grid grid = p.grid;
            if (grid == Grid::Mixed)
                grid = rng.chance (0.25) ? Grid::Triplet : Grid::Sixteenth;

            const int spb = grid == Grid::Triplet ? 3 : (grid == Grid::Eighth ? 2 : 4);
            const int nSlots = std::max (1, static_cast<int> (std::lround (barLen * spb)));

            double density = e.density;
            if (role.response)
                density *= 0.7;

            double target = (1.0 + 9.0 * std::pow (density, 1.25)) * barLen / 4.0;
            if (role.finalBar)
                target *= 0.5;
            else if (role.phraseEnd)
                target *= 0.65;

            const int count = std::clamp (static_cast<int> (std::lround (target * rng.uniform (0.85, 1.15))), 1, nSlots);

            auto slotWeight = [&] (int slot) {
                const double pos = static_cast<double> (slot) / spb;
                double w = metricStrength (pos, bpb);
                const double frac = pos - std::floor (pos + 1e-6);
                const double sync = e.syncopation;

                if (slot == 0)
                    w *= 1.0 - 0.3 * sync;
                else if (std::abs (frac) < 1e-6)
                    w *= 1.0 - 0.45 * sync;
                else if (std::abs (frac - 0.5) < 1e-6)
                    w *= 1.0 + 1.6 * sync;
                else
                    w *= 1.0 + 0.3 * sync + 0.9 * sync * sync;

                const double gamma = 2.6 - 2.0 * e.density;
                return std::pow (w, gamma);
            };

            std::vector<int> slots;

            const bool canUsePattern = ! p.patterns.empty() && grid == Grid::Sixteenth && std::abs (barLen - 4.0) < 1e-6;
            if (canUsePattern && rng.chance (0.6))
            {
                const char* pattern = rng.pick (p.patterns);
                for (int i = 0; i < 16 && pattern[i] != '\0'; ++i)
                    if (pattern[i] == 'x')
                        slots.push_back (i);

                // Nudge the pattern toward the requested density.
                while (static_cast<int> (slots.size()) > count + 2 && slots.size() > 1)
                {
                    int weakest = 1;
                    for (size_t i = 1; i < slots.size(); ++i)
                        if (slotWeight (slots[i]) < slotWeight (slots[static_cast<size_t> (weakest)]))
                            weakest = static_cast<int> (i);
                    slots.erase (slots.begin() + weakest);
                }
                while (static_cast<int> (slots.size()) < count - 2)
                {
                    std::vector<double> w (static_cast<size_t> (nSlots));
                    for (int i = 0; i < nSlots; ++i)
                        w[static_cast<size_t> (i)] = std::find (slots.begin(), slots.end(), i) == slots.end() ? slotWeight (i) : 0.0;
                    const int pick = rng.weighted (w);
                    if (pick < 0)
                        break;
                    slots.push_back (pick);
                }
            }
            else
            {
                std::vector<double> w (static_cast<size_t> (nSlots));
                for (int i = 0; i < nSlots; ++i)
                    w[static_cast<size_t> (i)] = slotWeight (i);

                for (int n = 0; n < count; ++n)
                {
                    const int pick = rng.weighted (w);
                    if (pick < 0)
                        break;
                    slots.push_back (pick);
                    w[static_cast<size_t> (pick)] = 0.0;
                }
            }

            std::sort (slots.begin(), slots.end());
            slots.erase (std::unique (slots.begin(), slots.end()), slots.end());

            if (role.phraseStart && ! role.response && (slots.empty() || slots.front() != 0) && rng.chance (0.85 - 0.5 * e.syncopation))
                slots.insert (slots.begin(), 0);

            // A response waits a moment before answering.
            if (role.response && slots.size() > 1 && rng.chance (0.7))
            {
                std::vector<int> kept;
                for (int sl : slots)
                    if (sl >= spb)
                        kept.push_back (sl);
                if (! kept.empty())
                    slots = kept;
            }

            // Phrase ends breathe: drop late notes so the last one can ring.
            if (role.phraseEnd && slots.size() > 1)
            {
                std::vector<int> kept;
                for (int sl : slots)
                    if (static_cast<double> (sl) / spb < barLen * 0.75 || ! rng.chance (0.75))
                        kept.push_back (sl);
                if (! kept.empty())
                    slots = kept;
            }

            Rhythm rhythm;
            for (int sl : slots)
            {
                double lf = e.articulation * rng.uniform (0.9, 1.05);
                if (rng.chance (e.rests * 0.35))
                    lf *= 0.45;
                rhythm.push_back ({ static_cast<double> (sl) / spb, std::clamp (lf, 0.15, 1.0) });
            }

            if (rhythm.empty())
                rhythm.push_back ({ 0.0, e.articulation });

            return rhythm;
        }

        //======================================================================
        struct PitchContext
        {
            double t = 0.0;
            double strength = 0.5;
            int prev = -1;
            int prevInterval = 0;
            bool phraseLast = false;
            bool finalNote = false;
            bool callEnd = false;
            bool responseEnd = false;
            double nextT = -1.0;      // start of the next note, if known
            double nextStrength = 0.0;
        };

        int choosePitch (const PitchContext& pc)
        {
            const auto& h = ctxAt (pc.t);
            const double target = contourTarget (pc.t);

            PitchMask allowed = h.chordScale;
            if ((p.flags & FlagPentatonic) != 0)
            {
                const auto penta = static_cast<PitchMask> ((pentaMask & h.chordScale) | h.core);
                if (maskCount (penta) >= 4)
                    allowed = penta;
            }

            const bool blues = (p.flags & FlagBlueNotes) != 0;
            if (blues)
                allowed |= blueMask;

            const ChordContext* next = nullptr;
            if (pc.nextT >= 0.0 && pc.nextStrength >= 0.6)
            {
                const int ci = contextIndexAt (prog, pc.t);
                const int ni = contextIndexAt (prog, pc.nextT);
                if (ni != ci)
                    next = &ctxs[static_cast<size_t> (ni)];
            }

            const int topPc = ((p.flags & FlagCounter) != 0 && ! tops.empty())
                                ? mod12 (tops[static_cast<size_t> (contextIndexAt (prog, pc.t))])
                                : -1;

            std::vector<int> cands;
            std::vector<double> weights;

            for (int n = lo; n <= hi; ++n)
            {
                if (! maskHas (allowed, n))
                    continue;

                const bool isCore = maskHas (h.core, n);
                const bool isStable = maskHas (h.stable, n);
                const bool isColour = maskHas (h.colour, n);
                const bool isAvoid = maskHas (h.avoid, n);
                const bool isBlue = blues && maskHas (blueMask, n) && ! maskHas (h.chordScale, n);
                const bool isRoot = mod12 (n) == h.root;

                double harm;
                if (pc.strength >= 0.6)
                {
                    if (isCore)
                        harm = 1.0 + 6.0 * e.chordTones;
                    else if (isColour)
                        harm = 0.3 + 4.0 * p.extensions;
                    else
                        harm = 0.05 + 0.6 * (1.0 - e.chordTones);

                    if (isAvoid)
                        harm *= 0.08;
                    if (isBlue)
                        harm *= 0.25;
                }
                else if (pc.strength >= 0.4)
                {
                    harm = isCore ? 1.0 + 2.0 * e.chordTones : (isColour ? 0.8 + p.extensions : 0.8);
                    if (isAvoid)
                        harm *= 0.5;
                    if (isBlue)
                        harm *= 0.6;
                }
                else
                {
                    harm = isCore ? 1.3 : 1.0;
                    if (isBlue)
                        harm *= 0.7;
                }

                double mel = 1.0;
                if (pc.prev >= 0)
                {
                    const int iv = n - pc.prev;
                    const int a = std::abs (iv);

                    if (a == 0)       mel = 0.15 + 1.1 * p.repeatNote;
                    else if (a <= 2)  mel = 1.0;
                    else if (a <= 4)  mel = 0.35 + 0.55 * e.leaps;
                    else if (a <= 7)  mel = 0.08 + 0.5 * e.leaps;
                    else if (a < 12)  mel = 0.02 + 0.22 * e.leaps;
                    else if (a == 12) mel = (p.flags & FlagOctaveJumps) != 0 ? 0.25 + 0.3 * e.leaps : 0.02 + 0.1 * e.leaps;
                    else              mel = 0.003;

                    if (a == 6)
                        mel *= 0.3;
                    if (a == 10 || a == 11)
                        mel *= 0.5;

                    // After a leap, step back the other way.
                    if (std::abs (pc.prevInterval) >= 5)
                    {
                        if (sign (iv) == -sign (pc.prevInterval) && a >= 1 && a <= 4)
                            mel *= 2.5;
                        else if (sign (iv) == sign (pc.prevInterval))
                            mel *= 0.3;
                    }

                    if (iv == 0 && pc.prevInterval == 0)
                        mel *= 0.1 + 0.8 * p.repeatNote;

                    if ((p.flags & FlagScalarRuns) != 0 && a >= 1 && a <= 2)
                    {
                        mel *= 2.0;
                        if (pc.prevInterval != 0 && sign (iv) == sign (pc.prevInterval))
                            mel *= 1.8;
                    }
                }

                const double d = n - target;
                const double sigma = 3.0 + 4.0 * e.leaps;
                const double con = std::exp (-d * d / (2.0 * sigma * sigma));

                double end = 1.0;
                if (pc.finalNote)
                    end = mod12 (n) == key.root() ? 8.0 : (isStable ? 1.5 : 0.2);
                else if (pc.phraseLast)
                    end = isStable ? (isRoot ? 3.0 : 2.0) : 0.4;
                else if (pc.callEnd)
                    end = isRoot ? 0.4 : 1.3;

                if (pc.responseEnd && ! pc.finalNote)
                    end *= isStable ? 2.5 : 0.6;

                if (next != nullptr && pc.strength < 0.6)
                {
                    // Lead smoothly into the next chord.
                    for (int dn = -2; dn <= 2; ++dn)
                        if (dn != 0 && maskHas (next->core, n + dn))
                        {
                            mel *= 1.5;
                            break;
                        }
                }

                double w = harm * mel * (0.08 + con) * end;

                if (topPc >= 0 && mod12 (n) == topPc)
                    w *= 0.4;

                cands.push_back (n);
                weights.push_back (std::pow (w, 1.15));
            }

            const int pick = rng.weighted (weights);
            if (pick < 0)
                return std::clamp (snapToMask (h.chordScale, static_cast<int> (std::lround (target))), lo, hi);

            return cands[static_cast<size_t> (pick)];
        }

        //======================================================================
        std::vector<int> pitchesFor (const Rhythm& rhythm, double barStart, const BarRole& role, size_t firstToGenerate,
                                     std::vector<int> pitches)
        {
            pitches.resize (rhythm.size(), 0);

            for (size_t i = firstToGenerate; i < rhythm.size(); ++i)
            {
                PitchContext pc;
                pc.t = barStart + rhythm[i].pos;
                pc.strength = metricStrength (rhythm[i].pos, bpb);

                const int prev = i > 0 ? pitches[i - 1] : (drafts.empty() ? -1 : drafts.back().pitch);
                const int prevPrev = i > 1 ? pitches[i - 2] : (i == 1 ? (drafts.empty() ? -1 : drafts.back().pitch)
                                                                      : (drafts.size() > 1 ? drafts[drafts.size() - 2].pitch : -1));
                pc.prev = prev;
                pc.prevInterval = (prev >= 0 && prevPrev >= 0) ? prev - prevPrev : 0;

                const bool lastInBar = i + 1 == rhythm.size();
                pc.phraseLast = lastInBar && role.phraseEnd;
                pc.finalNote = lastInBar && role.finalBar && s.endOnTonic;
                pc.callEnd = lastInBar && role.call;
                pc.responseEnd = lastInBar && role.response;

                if (i + 1 < rhythm.size())
                {
                    pc.nextT = barStart + rhythm[i + 1].pos;
                    pc.nextStrength = metricStrength (rhythm[i + 1].pos, bpb);
                }
                else
                {
                    pc.nextT = barStart + bpb;
                    pc.nextStrength = 1.0;
                }

                pitches[i] = choosePitch (pc);
            }

            return pitches;
        }

        std::vector<int> adaptMotif (const Motif& motif, double barStart, MotifOp op)
        {
            const double oldStart = motif.bar * static_cast<double> (bpb);
            const double firstPos = motif.rhythm.empty() ? 0.0 : motif.rhythm.front().pos;
            const auto& oldCtx = ctxAt (oldStart + firstPos);
            const auto& newCtx = ctxAt (barStart + firstPos);

            std::vector<int> src = motif.pitches;
            if (src.empty())
                return src;

            if (op == MotifOp::Retrograde)
                std::reverse (src.begin(), src.end());

            if (op == MotifOp::Inversion)
            {
                const int pivot = src.front();
                for (auto& n : src)
                    n = stepAlongMask (keyMask, pivot, -stepsBetween (keyMask, pivot, n));
            }

            int shiftSemis = mod12 (newCtx.root - oldCtx.root);
            if (shiftSemis > 6)
                shiftSemis -= 12;

            if (op != MotifOp::Repeat && shiftSemis != 0)
            {
                const int base = 60 + mod12 (oldCtx.root);
                const int steps = stepsBetween (keyMask, base, base + shiftSemis);
                for (auto& n : src)
                    n = stepAlongMask (keyMask, n, steps);
            }

            // Keep the motif near the contour and inside the window.
            double mean = 0.0;
            for (int n : src)
                mean += n;
            mean /= static_cast<double> (src.size());

            const double target = contourTarget (barStart);
            int octave = 0;
            if (mean - target > 7.0)
                octave = -12;
            else if (target - mean > 7.0)
                octave = 12;

            const auto [mn, mx] = std::minmax_element (src.begin(), src.end());
            if (*mx + octave > hi)
                octave -= 12;
            if (*mn + octave < lo)
                octave += 12;

            for (size_t i = 0; i < src.size(); ++i)
            {
                int n = src[i] + octave;
                const double pos = i < motif.rhythm.size() ? motif.rhythm[i].pos : 0.0;
                const int dir = i > 0 ? sign (src[i] - src[i - 1]) : 1;
                n = fitPitchToChord (n, ctxAt (barStart + pos), metricStrength (pos, bpb), dir == 0 ? 1 : -dir);
                while (n < lo) n += 12;
                while (n > hi) n -= 12;
                src[i] = n;
            }

            return src;
        }

        MotifOp chooseMotifOp (const Motif& motif, double barStart)
        {
            const double firstPos = motif.rhythm.empty() ? 0.0 : motif.rhythm.front().pos;
            const bool sameChord = ctxAt (motif.bar * static_cast<double> (bpb) + firstPos).root == ctxAt (barStart + firstPos).root;

            if ((p.flags & FlagMotifDevelop) != 0)
            {
                const double r = rng.uniform();
                if (r < 0.4)  return MotifOp::Sequence;
                if (r < 0.65) return MotifOp::Inversion;
                if (r < 0.85) return MotifOp::Retrograde;
                return MotifOp::Repeat;
            }

            if (sameChord)
                return MotifOp::Repeat;

            if ((p.flags & FlagSequence) != 0)
                return rng.chance (0.7) ? MotifOp::Sequence : MotifOp::Repeat;

            return rng.chance (0.5) ? MotifOp::Sequence : MotifOp::Repeat;
        }

        void buildPhrased()
        {
            const auto roles = barRoles();

            for (int b = 0; b < numBars; ++b)
            {
                const auto& role = roles[static_cast<size_t> (b)];
                const double barStart = b * static_cast<double> (bpb);
                const double barLen = std::min (static_cast<double> (bpb), total - barStart);
                if (barLen <= 0.01)
                    break;

                auto& motif = motifs[role.label - 'A'];
                Rhythm rhythm;
                std::vector<int> pitches;

                const bool reuse = motif.bar >= 0 && ! motif.rhythm.empty() && rng.chance (0.35 + 0.65 * e.repetition);

                if (reuse)
                {
                    for (const auto& o : motif.rhythm)
                        if (o.pos < barLen - 1e-6)
                            rhythm.push_back (o);

                    Motif trimmed = motif;
                    trimmed.rhythm = rhythm;
                    trimmed.pitches.resize (rhythm.size());

                    pitches = adaptMotif (trimmed, barStart, chooseMotifOp (motif, barStart));

                    // Variants and cadences rewrite the tail; low repetition rewrites more often.
                    size_t regenerateFrom = rhythm.size();
                    if (role.variant || role.finalBar)
                        regenerateFrom = rhythm.size() > 2 ? rhythm.size() - 2 : 0;
                    else if (rng.chance ((1.0 - e.repetition) * 0.5) || role.phraseEnd)
                        regenerateFrom = rhythm.size() > 1 ? rhythm.size() - 1 : 0;

                    if (regenerateFrom < rhythm.size())
                        pitches = pitchesFor (rhythm, barStart, role, regenerateFrom, pitches);
                }
                else
                {
                    rhythm = makeRhythm (barLen, role);
                    pitches = pitchesFor (rhythm, barStart, role, 0, {});

                    if (motif.bar < 0)
                    {
                        motif.rhythm = rhythm;
                        motif.pitches = pitches;
                        motif.bar = b;
                    }
                }

                for (size_t i = 0; i < rhythm.size(); ++i)
                {
                    Draft d;
                    d.pitch = pitches[i];
                    d.start = barStart + rhythm[i].pos;
                    d.lengthFactor = rhythm[i].lengthFactor;
                    d.strength = metricStrength (rhythm[i].pos, bpb);
                    d.phraseLast = i + 1 == rhythm.size() && role.phraseEnd;
                    drafts.push_back (d);
                }
            }
        }

        //======================================================================
        void buildArpeggio()
        {
            enum class Pattern { Up, Down, UpDown, Alberti, Broken, Pedal, Random };
            static const Pattern patterns[] = { Pattern::Up, Pattern::Down, Pattern::UpDown, Pattern::Alberti, Pattern::Broken, Pattern::Pedal, Pattern::Random };
            const Pattern pattern = patterns[rng.range (0, 6)];

            int spb = e.density > 0.55 ? 4 : 2;
            if (rng.chance (0.25))
                spb = 3;

            const double step = 1.0 / spb;
            const int cellSize = rng.chance (0.5) ? 4 : 5;
            const bool addNinth = p.extensions > 0.3 && rng.chance (0.5);

            int prevChord = -1;
            int counter = 0;
            int lastPick = -1;
            std::vector<int> cell;

            const int totalSteps = static_cast<int> (std::floor (total * spb + 1e-6));
            for (int k = 0; k < totalSteps; ++k)
            {
                const double t = k * step;
                const int ci = contextIndexAt (prog, t);
                const auto& h = ctxs[static_cast<size_t> (ci)];

                if (ci != prevChord)
                {
                    PitchMask tones = h.chordTones;
                    if (addNinth && maskHas (h.chordScale, h.root + 2) && ! maskHas (h.avoid, h.root + 2))
                        tones |= pcBit (h.root + 2);

                    const int anchor = static_cast<int> (std::lround (contourTarget (t))) - 4;
                    int startNote = snapToMask (tones, std::clamp (anchor, lo, hi), 1);
                    cell.clear();
                    for (int n = startNote; n <= hi + 12 && static_cast<int> (cell.size()) < cellSize; ++n)
                        if (maskHas (tones, n))
                            cell.push_back (n);
                    while (! cell.empty() && cell.back() > hi && cell.front() - 12 >= lo)
                    {
                        for (auto& c : cell)
                            c -= 12;
                    }

                    prevChord = ci;
                    counter = 0;
                }

                if (cell.empty())
                    continue;

                const int n = static_cast<int> (cell.size());
                int idx = 0;
                switch (pattern)
                {
                    case Pattern::Up:     idx = counter % n; break;
                    case Pattern::Down:   idx = n - 1 - (counter % n); break;
                    case Pattern::UpDown: { const int period = 2 * n - 2; const int ph = counter % std::max (1, period); idx = ph < n ? ph : period - ph; break; }
                    case Pattern::Alberti: { static const int a[4] = { 0, 2, 1, 2 }; idx = std::min (n - 1, a[counter % 4]); break; }
                    case Pattern::Broken:  { static const int b[4] = { 0, 2, 1, 3 }; idx = std::min (n - 1, b[counter % 4]); break; }
                    case Pattern::Pedal:   idx = counter % 2 == 0 ? 0 : 1 + ((counter / 2) % (n - 1)); break;
                    case Pattern::Random:
                    {
                        idx = rng.range (0, n - 1);
                        if (idx == lastPick)
                            idx = (idx + 1) % n;
                        break;
                    }
                }
                lastPick = idx;
                ++counter;

                if (k > 0 && rng.chance (e.rests * 0.25))
                    continue;

                Draft d;
                d.pitch = cell[static_cast<size_t> (idx)];
                d.start = t;
                d.lengthFactor = e.articulation;
                d.strength = metricStrength (std::fmod (t, static_cast<double> (bpb)), bpb);
                d.phraseLast = false;
                drafts.push_back (d);
            }
        }

        //======================================================================
        void addBebopApproaches()
        {
            for (size_t i = 2; i < drafts.size(); ++i)
            {
                auto& target = drafts[i];
                auto& before = drafts[i - 1];
                auto& twoBefore = drafts[i - 2];

                if (target.strength < 0.6 || before.strength >= 0.6)
                    continue;
                if (std::abs ((target.start - before.start) - 0.5) > 1e-6 || std::abs ((before.start - twoBefore.start) - 0.5) > 1e-6)
                    continue;

                const auto& h = ctxAt (target.start);
                const double r = rng.uniform();

                if (r < 0.3)
                {
                    // Enclosure: scale note above, chromatic note below, then the target.
                    const int above = stepAlongMask (h.chordScale, target.pitch, 1);
                    const int below = target.pitch - 1;
                    if (above <= hi + 2 && below >= lo - 2)
                    {
                        twoBefore.pitch = above;
                        before.pitch = below;
                    }
                }
                else if (r < 0.6)
                {
                    const int approach = target.pitch + (twoBefore.pitch > target.pitch ? 1 : -1);
                    if (approach != twoBefore.pitch)
                        before.pitch = approach;
                }
            }
        }

        std::vector<NoteEvent> resolveDurations()
        {
            std::stable_sort (drafts.begin(), drafts.end(), [] (const Draft& a, const Draft& b) { return a.start < b.start; });

            std::vector<NoteEvent> notes;
            notes.reserve (drafts.size());

            const bool longTones = (p.flags & FlagLongTones) != 0;
            const double maxLen = longTones ? 16.0 : (e.density < 0.3 ? 3.0 : 2.0);

            for (size_t i = 0; i < drafts.size(); ++i)
            {
                const auto& d = drafts[i];
                const double gap = (i + 1 < drafts.size() ? drafts[i + 1].start : total) - d.start;
                double len = std::min (gap, maxLen) * d.lengthFactor;

                if (d.phraseLast && i + 1 < drafts.size())
                    len = std::max (len, std::min (gap * 0.92, std::max (gap - 0.5, gap * 0.6)));

                NoteEvent n;
                n.pitch = d.pitch;
                n.start = d.start;
                n.length = std::max (0.05, len);
                n.velocity = std::clamp (p.velocity + static_cast<int> (std::lround ((d.strength - 0.5) * 20.0)) + rng.range (-3, 3), 35, 127);
                notes.push_back (n);
            }

            return notes;
        }

        void resolveEnding (std::vector<NoteEvent>& notes)
        {
            if (notes.empty() || ! s.endOnTonic)
                return;

            auto& last = notes.back();
            const auto& h = ctxAt (std::max (0.0, total - 0.01));

            const int tonic = key.root();
            const int goalPc = maskHas (h.chordScale, tonic) ? tonic : h.root;

            int best = last.pitch;
            int bestDist = 100;
            for (int n = lo; n <= hi; ++n)
            {
                if (mod12 (n) != goalPc)
                    continue;
                const int d = std::abs (n - last.pitch);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = n;
                }
            }

            last.pitch = best;
            last.length = std::max (last.length, (total - last.start) * 0.98);
            last.velocity = std::max (40, last.velocity - 4);
        }
    };

    double similarity (const Melody& a, const Melody& b)
    {
        if (a.notes.empty() || b.notes.empty())
            return 0.0;

        int same = 0;
        for (const auto& n : a.notes)
            for (const auto& m : b.notes)
                if (n.pitch == m.pitch && std::abs (n.start - m.start) < 0.02)
                {
                    ++same;
                    break;
                }

        return static_cast<double> (same) / static_cast<double> (std::max (a.notes.size(), b.notes.size()));
    }
} // namespace

Melody generateMelody (const Progression& progression, const Scale& key, const MelodySettings& settings,
                       MelodyStyle style, std::uint32_t seed)
{
    if (style == MelodyStyle::Mixed)
    {
        const auto& cycle = mixedStyleCycle();
        style = cycle[seed % cycle.size()];
    }

    Generator g (progression, key, settings, style, seed);
    auto m = g.run();
    m.name = m.style;
    return m;
}

std::vector<Melody> generateVariations (const Progression& progression, const Scale& key, const MelodySettings& settings,
                                        MelodyStyle style, int count, std::uint32_t baseSeed)
{
    std::vector<Melody> out;
    if (progression.empty())
        return out;

    // For Mixed, walk a shuffled list of styles (plus genre-appropriate extras) so neighbours differ.
    std::vector<MelodyStyle> styles;
    if (style == MelodyStyle::Mixed)
    {
        styles = mixedStyleCycle();

        int sevenths = 0;
        for (const auto& c : progression.chords)
            sevenths += c.chord.hasSeventh() ? 1 : 0;
        if (sevenths * 2 >= static_cast<int> (progression.chords.size()))
            styles.push_back (MelodyStyle::Bebop);
        if (key.isMinor())
            styles.push_back (MelodyStyle::Trap);

        Rng shuffler (baseSeed ^ 0x5bd1e995u);
        shuffler.shuffle (styles);
    }

    for (int i = 0; i < count; ++i)
    {
        const MelodyStyle s = styles.empty() ? style : styles[static_cast<size_t> (i) % styles.size()];

        Melody best;
        for (std::uint32_t attempt = 0; attempt < 4; ++attempt)
        {
            const auto seed = mixSeed (baseSeed, static_cast<std::uint32_t> (i) + attempt * 977u);
            Generator g (progression, key, settings, s, seed);
            auto m = g.run();

            bool distinct = true;
            for (const auto& prev : out)
                if (similarity (prev, m) > 0.8)
                    distinct = false;

            best = std::move (m);
            if (distinct)
                break;
        }

        best.name = std::to_string (i + 1) + ". " + best.style;
        out.push_back (std::move (best));
    }

    return out;
}

} // namespace melodee
