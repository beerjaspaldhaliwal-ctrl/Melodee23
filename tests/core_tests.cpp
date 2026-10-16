// Unit tests for the Melodee engine. No framework needed: run the executable, exit code 0 = pass.

#include "melodee/Melodee.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace melodee;

namespace
{
    int failures = 0;
    int checks = 0;
    std::string currentTest;

    void check (bool ok, const std::string& what)
    {
        ++checks;
        if (! ok)
        {
            ++failures;
            std::cerr << "  FAIL [" << currentTest << "] " << what << "\n";
        }
    }

    void test (const std::string& name, const std::function<void()>& body)
    {
        currentTest = name;
        const int before = failures;
        body();
        std::cout << (failures == before ? "  ok    " : "  FAIL  ") << name << "\n";
    }

    std::set<int> pcs (const Chord& c)
    {
        std::set<int> s;
        for (int i : c.intervals)
            s.insert (mod12 (c.root + i));
        return s;
    }

    std::set<int> pcsOf (std::initializer_list<const char*> names)
    {
        std::set<int> s;
        for (auto n : names)
        {
            size_t used = 0;
            s.insert (parseNoteName (n, used));
        }
        return s;
    }

    const KeyContext C_MAJOR { 0, false, false };
    const KeyContext A_MINOR { 9, true, false };
} // namespace

int main()
{
    std::cout << "Melodee core tests\n";

    test ("scale library is complete and well formed", [] {
        const auto& scales = allScales();
        check (scales.size() >= 100, "at least 100 scales, got " + std::to_string (scales.size()));

        for (const auto& s : scales)
        {
            check (! s.intervals.empty() && s.intervals.front() == 0, s.name + " starts on the root");
            for (size_t i = 1; i < s.intervals.size(); ++i)
                check (s.intervals[i] > s.intervals[i - 1] && s.intervals[i] < 12, s.name + " intervals ascend inside an octave");
        }

        for (const char* name : { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Dorian", "Phrygian", "Lydian",
                                  "Mixolydian", "Locrian", "Phrygian Dominant", "Lydian Dominant", "Altered", "Hungarian Minor",
                                  "Double Harmonic Major", "Major Pentatonic", "Minor Pentatonic", "Blues", "Whole Tone",
                                  "Diminished (Whole-Half)", "Bebop Dominant", "Hirajoshi", "In", "Persian", "Enigmatic",
                                  "Neapolitan Minor", "Chromatic" })
            check (findScale (name) >= 0, std::string ("has scale ") + name);

        check (findScale ("aeolian") == findScale ("Aeolian"), "case-insensitive lookup");
        check (findScale ("Spanish Gypsy") == findScale ("Phrygian Dominant"), "alias lookup");
        check (findScale ("Super Locrian") == findScale ("Altered"), "alias lookup 2");
    });

    test ("modes are rotations of their parent scale", [] {
        const Scale cMajor (0, findScale ("Major"));
        check (Scale (2, findScale ("Dorian")).mask() == cMajor.mask(), "D Dorian = C major notes");
        check (Scale (4, findScale ("Phrygian")).mask() == cMajor.mask(), "E Phrygian = C major notes");
        check (Scale (11, findScale ("Locrian")).mask() == cMajor.mask(), "B Locrian = C major notes");
        check (Scale (4, findScale ("Phrygian Dominant")).mask() == Scale (9, findScale ("Harmonic Minor")).mask(), "E Phrygian dominant = A harmonic minor");
        check (Scale (7, findScale ("Lydian Dominant")).mask() == Scale (2, findScale ("Melodic Minor")).mask(), "G Lydian dominant = D melodic minor");
        check (Scale (9, findScale ("Natural Minor")).isMinor(), "natural minor reads as minor");
        check (! Scale (0, findScale ("Mixolydian")).isMinor(), "mixolydian reads as major");
        check (Scale (3, findScale ("Major")).noteNames (true) == "Eb F G Ab Bb C D", "Eb major spelled with flats");
    });

    test ("scale stepping and snapping", [] {
        const Scale c (0, findScale ("Major"));
        check (c.stepFrom (60, 1) == 62, "C up one step is D");
        check (c.stepFrom (60, -1) == 59, "C down one step is B");
        check (c.stepFrom (60, 7) == 72, "seven steps is an octave");
        check (c.snap (61) == 60, "C# snaps down to C");
        check (stepsBetween (c.mask(), 60, 67) == 4, "C to G is four steps");
    });

    test ("chord symbols parse to the right notes", [] {
        struct Case { const char* symbol; std::initializer_list<const char*> notes; const char* name; };
        const Case cases[] = {
            { "C", { "C", "E", "G" }, "C" },
            { "Am", { "A", "C", "E" }, "Am" },
            { "Bdim", { "B", "D", "F" }, "Bdim" },
            { "Caug", { "C", "E", "G#" }, "Caug" },
            { "Dsus2", { "D", "E", "A" }, "Dsus2" },
            { "Gsus", { "G", "C", "D" }, "Gsus4" },
            { "Cmaj7", { "C", "E", "G", "B" }, "Cmaj7" },
            { "CM7", { "C", "E", "G", "B" }, "Cmaj7" },
            { "C\xCE\x94", { "C", "E", "G", "B" }, "Cmaj7" },
            { "G7", { "G", "B", "D", "F" }, "G7" },
            { "Dm7", { "D", "F", "A", "C" }, "Dm7" },
            { "C-7", { "C", "Eb", "G", "Bb" }, "Cm7" },
            { "Bm7b5", { "B", "D", "F", "A" }, "Bm7b5" },
            { "F#\xC3\xB8" "7", { "F#", "A", "C", "E" }, "F#m7b5" },
            { "Bdim7", { "B", "D", "F", "Ab" }, "Bdim7" },
            { "C\xC2\xB0" "7", { "C", "Eb", "Gb", "A" }, "Cdim7" },
            { "Cm(maj7)", { "C", "Eb", "G", "B" }, "CmMaj7" },
            { "C6", { "C", "E", "G", "A" }, "C6" },
            { "C6/9", { "C", "E", "G", "A", "D" }, "C6/9" },
            { "Cadd9", { "C", "E", "G", "D" }, "Cadd9" },
            { "Cmadd9", { "C", "Eb", "G", "D" }, "Cmadd9" },
            { "C9", { "C", "E", "G", "Bb", "D" }, "C9" },
            { "Cmaj9", { "C", "E", "G", "B", "D" }, "Cmaj9" },
            { "Cm9", { "C", "Eb", "G", "Bb", "D" }, "Cm9" },
            { "Cm11", { "C", "Eb", "G", "Bb", "D", "F" }, "Cm11" },
            { "C13", { "C", "E", "G", "Bb", "D", "A" }, "C13" },
            { "C7b9", { "C", "E", "G", "Bb", "Db" }, "C7b9" },
            { "C7#9", { "C", "E", "G", "Bb", "D#" }, "C7#9" },
            { "Cmaj7#11", { "C", "E", "G", "B", "F#" }, "Cmaj7#11" },
            { "C7(b9, #11)", { "C", "E", "G", "Bb", "Db", "F#" }, "C7b9#11" },
            { "C7alt", { "C", "E", "Bb", "Db", "D#", "G#" }, "C7alt" },
            { "C5", { "C", "G" }, "C5" },
            { "C7sus4", { "C", "F", "G", "Bb" }, "C7sus4" },
            { "Ebmaj7", { "Eb", "G", "Bb", "D" }, "Ebmaj7" },
            { "Bbm7", { "Bb", "Db", "F", "Ab" }, "Bbm7" },
        };

        for (const auto& c : cases)
        {
            const auto chord = parseChordSymbol (c.symbol);
            check (chord.has_value(), std::string ("parses ") + c.symbol);
            if (! chord)
                continue;
            check (pcs (*chord) == pcsOf (c.notes), std::string ("notes of ") + c.symbol);
            check (chord->name() == c.name, std::string ("name of ") + c.symbol + " is " + c.name + ", got " + chord->name());
        }

        const auto slash = parseChordSymbol ("D/F#");
        check (slash && slash->bassPc() == 6 && slash->root == 2, "slash chord bass");

        check (! parseChordSymbol ("Hm7").has_value(), "rejects H");
        check (! parseChordSymbol ("Cxyz").has_value(), "rejects nonsense suffix");
    });

    test ("roman numerals in major and minor keys", [] {
        auto name = [] (const char* numeral, const KeyContext& k) {
            const auto c = parseRomanNumeral (numeral, k);
            return c ? c->name() : std::string ("?");
        };

        check (name ("I", C_MAJOR) == "C", "I");
        check (name ("ii7", C_MAJOR) == "Dm7", "ii7");
        check (name ("V7", C_MAJOR) == "G7", "V7");
        check (name ("viiø7", C_MAJOR) == "Bm7b5", "vii half-diminished");
        check (name ("bVII", C_MAJOR) == "Bb", "bVII");
        check (name ("bVImaj7", C_MAJOR) == "Abmaj7", "bVImaj7");
        check (name ("iv", C_MAJOR) == "Fm", "borrowed iv");
        check (name ("#iv°7", C_MAJOR) == "F#dim7", "#iv dim7");
        check (name ("V/V", C_MAJOR) == "D", "secondary dominant");
        check (name ("V7/vi", C_MAJOR) == "E7", "V7/vi");
        check (name ("I/3", C_MAJOR) == "C/E", "inversion with bass degree");
        check (name ("i", A_MINOR) == "Am", "i in minor");
        check (name ("VI", A_MINOR) == "F", "VI in minor");
        check (name ("bVI", A_MINOR) == "F", "bVI in minor means the same");
        check (name ("III", A_MINOR) == "C", "III in minor");
        check (name ("V7", A_MINOR) == "E7", "V7 in minor");
        check (name ("vii°7", A_MINOR) == "G#dim7", "leading-tone dim7 in minor");
        check (name ("VII", A_MINOR) == "G", "subtonic VII in minor");
        check (name ("bII", A_MINOR) == "Bb", "Neapolitan");
        check (name ("imaj7", A_MINOR) == "AmMaj7", "minor-major 7");

        const auto c = parseRomanNumeral ("V7", C_MAJOR);
        check (c && romanNumeralFor (*c, C_MAJOR) == "V7", "numeral round trip");
    });

    test ("progression text formats", [] {
        auto names = [] (const char* text, const KeyContext& k = C_MAJOR) {
            const auto r = parseProgression (text, k);
            return r.ok() ? r.progression.toChordNames() : std::string ("ERROR");
        };

        check (names ("Am F C G") == "Am F C G", "chord names");
        check (names ("vi IV I V") == "Am F C G", "roman numerals");
        check (names ("ii7-V7-Imaj7") == "Dm7 G7 Cmaj7", "dash separated");
        check (names ("C-7 F7") == "Cm7 F7", "dash as minor");
        check (names ("| Dm7 G7 | Cmaj7 |") == "Dm7:0.5 G7:0.5 Cmaj7", "bars split evenly");
        check (names ("Am:2 F C") == "Am:2 F C", "durations in bars");
        check (names ("Am F x2") == "Am F Am F", "repeat");
        check (names ("C % F") == "C C F", "% repeats a chord");
        check (names ("C7(b9, #11) F") == "C7b9#11 F", "brackets with commas");
        check (names ("i VI III VII", A_MINOR) == "Am F C G", "minor numerals");

        const auto r = parseProgression ("Am F C G", C_MAJOR);
        check (std::abs (r.progression.totalBeats - 16.0) < 1e-9, "four bars of 4/4");
        check (r.progression.indexAt (5.0) == 1, "chord at beat 5 is the 2nd");

        const auto bad = parseProgression ("Am Zz9 C", C_MAJOR);
        check (! bad.ok() && bad.errors.size() == 1 && bad.progression.chords.size() == 2, "bad tokens are reported and skipped");
    });

    test ("every library progression parses in every key", [] {
        const auto& lib = progressionLibrary();
        check (lib.size() >= 250, "at least 250 progressions, got " + std::to_string (lib.size()));
        check (progressionGenres().size() >= 40, "at least 40 genres, got " + std::to_string (progressionGenres().size()));

        for (const auto& p : lib)
        {
            const int si = findScale (p.scale);
            check (si >= 0, p.name + ": scale " + p.scale + " exists");
            if (si < 0)
                continue;

            for (int root = 0; root < 12; ++root)
            {
                const auto r = parseProgression (p.numerals, Scale (root, si).keyContext());
                if (! r.ok())
                {
                    check (false, p.genre + " / " + p.name + " parses in key " + std::to_string (root));
                    break;
                }
            }
        }

        check (! searchProgressions ("jazz").empty(), "search finds jazz");
        check (! progressionsInGenre ("Trap").empty(), "trap genre exists");
    });

    test ("diatonic chords", [] {
        std::string s;
        for (const auto& c : diatonicChords (Scale (0, findScale ("Major")), true))
            s += c.name() + " ";
        check (s == "Cmaj7 Dm7 Em7 Fmaj7 G7 Am7 Bm7b5 ", "C major 7ths, got " + s);

        s.clear();
        for (const auto& c : diatonicChords (Scale (9, findScale ("Harmonic Minor")), false))
            s += c.name() + " ";
        check (s == "Am Bdim Caug Dm E F G#dim ", "A harmonic minor triads, got " + s);

        check (! diatonicChords (Scale (9, findScale ("Minor Pentatonic")), false).empty(), "pentatonic chords exist");
    });

    test ("chord scales fit borrowed chords", [] {
        const Scale c (0, findScale ("Major"));
        const auto bb = analyseChord (*parseChordSymbol ("Bb"), c);
        check (maskHas (bb.chordScale, 10) && ! maskHas (bb.chordScale, 11), "Bb in C uses Bb, not B");
        const auto e7 = analyseChord (*parseChordSymbol ("E7"), c);
        check (maskHas (e7.chordScale, 8) && ! maskHas (e7.chordScale, 7), "E7 in C uses G#, not G");
        const auto cMaj = analyseChord (*parseChordSymbol ("C"), c);
        check (maskHas (cMaj.avoid, 5), "F is an avoid note over C");
        check (cMaj.chordScale == c.mask(), "diatonic chord uses the key");
    });

    const char* progressions[] = { "Am F C G", "ii7 V7 Imaj7 vi7", "| C G | Am F | Dm7 G7 | C |", "I7 IV7 I7 V7" };
    const char* scaleNames[] = { "Natural Minor", "Major", "Major", "Blues" };
    const int tonics[] = { 9, 0, 0, 7 };

    test ("every style writes valid melodies", [&] {
        for (int pi = 0; pi < 4; ++pi)
        {
            const Scale key (tonics[pi], findScale (scaleNames[pi]));
            const auto prog = parseProgression (progressions[pi], key.keyContext()).progression;

            for (int st = 0; st < numMelodyStyles(); ++st)
            {
                for (std::uint32_t seed = 1; seed <= 3; ++seed)
                {
                    MelodySettings ms;
                    const auto m = generateMelody (prog, key, ms, static_cast<MelodyStyle> (st), seed);
                    const std::string what = melodyStyleNames()[static_cast<size_t> (st)] + " on \"" + progressions[pi] + "\" seed " + std::to_string (seed);

                    check (m.notes.size() >= 2, what + ": has notes");
                    for (size_t i = 0; i < m.notes.size(); ++i)
                    {
                        const auto& n = m.notes[i];
                        check (n.pitch >= ms.lowNote && n.pitch <= ms.highNote, what + ": in range");
                        check (n.start >= 0.0 && n.start + n.length <= prog.totalBeats + 1e-6, what + ": inside the progression");
                        check (n.length > 0.0, what + ": positive length");
                        check (n.velocity >= 1 && n.velocity <= 127, what + ": velocity");
                        if (i > 0)
                            check (m.notes[i - 1].start + m.notes[i - 1].length <= n.start + 1e-6, what + ": monophonic, no overlaps");
                    }
                }
            }
        }
    });

    test ("strong beats land on chord tones", [] {
        const Scale key (0, findScale ("Major"));
        const auto prog = parseProgression ("I V vi IV", key.keyContext()).progression;

        for (auto style : { MelodyStyle::Lyrical, MelodyStyle::ChordTones, MelodyStyle::Classical, MelodyStyle::PopHook })
        {
            int strong = 0, onChord = 0;
            for (std::uint32_t seed = 1; seed <= 20; ++seed)
            {
                const auto m = generateMelody (prog, key, {}, style, seed);
                for (const auto& n : m.notes)
                {
                    const double pos = std::fmod (n.start, 4.0);
                    if (std::abs (pos - std::round (pos)) > 1e-6 || (static_cast<int> (std::round (pos)) % 2) != 0)
                        continue; // beats 1 and 3 only
                    ++strong;
                    const auto* e = prog.at (n.start);
                    if (e != nullptr && (e->chord.mask() & pcBit (n.pitch)) != 0)
                        ++onChord;
                }
            }
            const double ratio = strong > 0 ? static_cast<double> (onChord) / strong : 0.0;
            check (ratio >= 0.7, melodyStyleNames()[static_cast<size_t> (style)] + ": chord tones on beats 1 and 3 = " + std::to_string (ratio));
        }
    });

    test ("melodies are reproducible and variations differ", [] {
        const Scale key (9, findScale ("Natural Minor"));
        const auto prog = parseProgression ("i VI III VII", key.keyContext()).progression;

        const auto a = generateMelody (prog, key, {}, MelodyStyle::PopHook, 42);
        const auto b = generateMelody (prog, key, {}, MelodyStyle::PopHook, 42);
        bool same = a.notes.size() == b.notes.size();
        for (size_t i = 0; same && i < a.notes.size(); ++i)
            same = a.notes[i].pitch == b.notes[i].pitch && a.notes[i].start == b.notes[i].start;
        check (same, "same seed gives the same melody");

        const auto vars = generateVariations (prog, key, {}, MelodyStyle::Mixed, 12, 7);
        check (vars.size() == 12, "twelve variations");

        std::set<std::string> fingerprints, styles;
        for (const auto& v : vars)
        {
            std::ostringstream fp;
            for (const auto& n : v.notes)
                fp << n.pitch << '@' << n.start << ' ';
            fingerprints.insert (fp.str());
            styles.insert (v.style);
        }
        check (fingerprints.size() == vars.size(), "all variations are different");
        check (styles.size() >= 8, "mixed mode uses many styles");

        const auto same3 = generateVariations (prog, key, {}, MelodyStyle::Lyrical, 6, 99);
        std::set<std::string> fps;
        for (const auto& v : same3)
        {
            std::ostringstream fp;
            for (const auto& n : v.notes)
                fp << n.pitch << '@' << n.start << ' ';
            fps.insert (fp.str());
        }
        check (fps.size() == same3.size(), "single-style variations differ");
    });

    test ("settings change the result", [] {
        const Scale key (0, findScale ("Major"));
        const auto prog = parseProgression ("I vi IV V", key.keyContext()).progression;

        size_t sparse = 0, dense = 0;
        for (std::uint32_t seed = 1; seed <= 10; ++seed)
        {
            MelodySettings lo, hi;
            lo.density = 0.0;
            hi.density = 1.0;
            sparse += generateMelody (prog, key, lo, MelodyStyle::Lyrical, seed).notes.size();
            dense += generateMelody (prog, key, hi, MelodyStyle::Lyrical, seed).notes.size();
        }
        check (dense > sparse * 2, "density 1 writes far more notes than density 0");

        MelodySettings narrow;
        narrow.lowNote = 67;
        narrow.highNote = 79;
        for (const auto& n : generateMelody (prog, key, narrow, MelodyStyle::Cinematic, 3).notes)
            check (n.pitch >= 67 && n.pitch <= 79, "custom range respected");

        MelodySettings tonicEnd;
        const auto m = generateMelody (prog, key, tonicEnd, MelodyStyle::Lyrical, 5);
        check (! m.notes.empty() && mod12 (m.notes.back().pitch) == 0, "ends on the tonic");
    });

    test ("every transform produces a usable variation", [] {
        const Scale key (0, findScale ("Major"));
        const auto prog = parseProgression ("I V vi IV", key.keyContext()).progression;
        MelodySettings ms;
        const auto base = generateMelody (prog, key, ms, MelodyStyle::Lyrical, 11);

        for (int t = 0; t < numTransforms(); ++t)
        {
            const auto tr = static_cast<MelodyTransform> (t);
            const auto m = transformMelody (base, tr, prog, key, ms, 5);
            const auto& name = transformNames()[static_cast<size_t> (t)];
            check (! m.notes.empty(), name + ": has notes");
            for (const auto& n : m.notes)
            {
                check (n.start >= -1e-6 && n.start < prog.totalBeats + 0.05, name + ": inside the progression");
                check (n.pitch >= 0 && n.pitch <= 127, name + ": valid pitch");
            }
        }

        const auto up = transformMelody (base, MelodyTransform::OctaveUp, prog, key, ms, 1);
        check (up.notes.size() == base.notes.size() && up.notes.front().pitch == base.notes.front().pitch + 12, "octave up");

        const auto harm = transformMelody (base, MelodyTransform::HarmonizeThirds, prog, key, ms, 1);
        check (harm.notes.size() == base.notes.size() * 2, "harmony doubles the notes");

        const auto retro = transformMelody (base, MelodyTransform::Retrograde, prog, key, ms, 1);
        check (retro.notes.size() == base.notes.size(), "retrograde keeps the note count");

        // Fit to new chords: strong beats become chord tones of the new progression.
        const auto newProg = parseProgression ("ii V iii vi", key.keyContext()).progression;
        const auto fitted = transformMelody (base, MelodyTransform::FitToChords, newProg, key, ms, 1);
        for (const auto& n : fitted.notes)
        {
            if (std::abs (std::fmod (n.start, 4.0)) > 1e-6)
                continue;
            const auto* e = newProg.at (n.start);
            if (e == nullptr)
                continue;
            const auto ctx = analyseChord (e->chord, key);
            check (((ctx.chordTones | ctx.colour) & pcBit (n.pitch)) != 0, "fitted downbeat is a chord or colour tone");
        }
    });

    test ("chord voicings and rhythms", [] {
        const Scale key (0, findScale ("Major"));
        const auto prog = parseProgression ("Cmaj7 Am7 Dm7 G7", key.keyContext()).progression;

        for (size_t v = 0; v < voicingNames().size(); ++v)
        {
            const auto voicings = voiceProgression (prog, static_cast<VoicingStyle> (v), 60);
            check (voicings.size() == prog.chords.size(), voicingNames()[v] + ": one voicing per chord");
            for (size_t i = 0; i < voicings.size(); ++i)
            {
                check (voicings[i].size() >= 2, voicingNames()[v] + ": at least two notes");
                const auto& chord = prog.chords[i].chord;
                const int third = mod12 (chord.root + (chord.isMinor() ? 3 : 4));
                bool hasThird = false;
                for (int n : voicings[i])
                {
                    check (n >= 20 && n <= 100, voicingNames()[v] + ": sensible register");
                    hasThird = hasThird || mod12 (n) == third;
                }
                if (static_cast<VoicingStyle> (v) != VoicingStyle::Power)
                    check (hasThird, voicingNames()[v] + ": contains the 3rd of " + chord.name());
            }
        }

        // Voice-led voicings move less than root-position close voicings.
        auto travel = [] (const std::vector<std::vector<int>>& vs) {
            double t = 0;
            for (size_t i = 1; i < vs.size(); ++i)
                t += std::abs (vs[i].front() - vs[i - 1].front()) + std::abs (vs[i].back() - vs[i - 1].back());
            return t;
        };
        const auto longProg = parseProgression ("C F G Am Em F G C", key.keyContext()).progression;
        check (travel (voiceProgression (longProg, VoicingStyle::VoiceLed, 60)) <= travel (voiceProgression (longProg, VoicingStyle::Close, 60)),
               "voice leading moves less than block chords");

        for (size_t r = 0; r < chordRhythmNames().size(); ++r)
        {
            AccompanimentSettings as;
            as.rhythm = static_cast<ChordRhythm> (r);
            const auto notes = renderChords (prog, as, 1);
            check (! notes.empty(), chordRhythmNames()[r] + ": renders");
            for (const auto& n : notes)
                check (n.start >= -0.05 && n.start < prog.totalBeats, chordRhythmNames()[r] + ": inside the progression");
        }

        for (size_t b = 1; b < bassStyleNames().size(); ++b)
        {
            AccompanimentSettings as;
            as.bass = static_cast<BassStyle> (b);
            const auto notes = renderBass (prog, as, 1);
            check (! notes.empty(), bassStyleNames()[b] + ": renders");
            for (const auto& n : notes)
                check (n.pitch >= 24 && n.pitch <= 60, bassStyleNames()[b] + ": bass register");
            if (! notes.empty())
                check (mod12 (notes.front().pitch) == prog.chords.front().chord.root, bassStyleNames()[b] + ": starts on the root");
        }

        AccompanimentSettings slash;
        const auto slashProg = parseProgression ("C/E F G/B C", key.keyContext()).progression;
        check (mod12 (renderBass (slashProg, slash, 1).front().pitch) == 4, "slash chord bass note is used");
    });

    test ("MIDI files round trip", [] {
        const Scale key (0, findScale ("Major"));
        const auto prog = parseProgression ("I V vi IV", key.keyContext()).progression;
        const auto m = generateMelody (prog, key, {}, MelodyStyle::PopHook, 3);
        const auto chords = renderChords (prog, {}, 3);

        const auto data = writeMidiFile ({ { "Melody", 0, -1, m.notes }, { "Chords", 1, 0, chords } }, 120.0, 4);
        check (data.size() > 50 && data[0] == 'M' && data[1] == 'T', "header");

        const auto tracks = readMidiFile (data);
        check (tracks.size() == 3, "conductor + 2 tracks");
        if (tracks.size() == 3)
        {
            check (tracks[1].name == "Melody" && tracks[2].name == "Chords", "track names");
            check (tracks[1].notes.size() == m.notes.size(), "melody note count");
            check (tracks[2].notes.size() == chords.size(), "chord note count");
            check (tracks[2].channel == 1, "chord channel");
            for (size_t i = 0; i < std::min (tracks[1].notes.size(), m.notes.size()); ++i)
            {
                check (tracks[1].notes[i].pitch == m.notes[i].pitch, "pitch preserved");
                check (std::abs (tracks[1].notes[i].start - m.notes[i].start) < 0.002, "timing preserved");
            }
        }
    });

    std::cout << "\n" << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
