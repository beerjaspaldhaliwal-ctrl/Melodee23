// melodee: command-line front end for the Melodee engine.
// Generates melody variations, chords and bass for a progression and writes MIDI files.

#include "melodee/Melodee.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace melodee;

namespace
{
    void printUsage()
    {
        std::cout <<
R"(melodee - melodies, chords and variations for any progression

USAGE
  melodee --key A --scale "Natural Minor" --prog "Am F C G" [options]
  melodee --key C --genre "Neo-Soul" --pick 2 --style "R&B" --variations 6
  melodee --list-scales | --list-genres | --list-progressions [GENRE] | --list-styles
          --list-transforms | --list-voicings | --list-rhythms | --list-bass

PROGRESSION
  --prog TEXT          Chord names or roman numerals: "Am F C G", "vi IV I V", "ii7-V7-Imaj7",
                       "| Dm7 G7 | Cmaj7 |", "Am:2 F:2", "V7/vi", "I/3", "x2" (repeat)
  --genre NAME         Use a progression from the built-in library (with --pick N, default 1)
  --key NOTE           Key / tonic: C, F#, Bb ... (default C)
  --scale NAME         Any scale (see --list-scales). Default: the library's or Major
  --loops N            Repeat the progression N times before writing the melody (default 1)
  --time N             Beats per bar (default 4)
  --bpm N              Tempo written to the MIDI file (default 120)

MELODY
  --style NAME         Melody style (see --list-styles) or "mixed" (default)
  --variations N       Number of variations (default 8)
  --seed N             Base seed; the same seed gives the same results (default 1)
  --low NOTE --high NOTE   Range, e.g. C4 and C6 (default C4..C6)
  --density X --syncopation X --leaps X --chord-tones X --repetition X
  --rests X --swing X --length X      0..1, 0.5 = the style's default
  --humanize X         0..1 (default 0.15)
  --contour NAME       Auto, Arch, Valley, Rising, Falling, Wave, Flat
  --form NAME          Auto, Repeat, AABA, ABAB, AABB, ABAC, CallResponse, Through
  --transform NAME     Also write variations of variation 1 using this transform
                       (repeatable; "all" for every transform)

ACCOMPANIMENT
  --voicing NAME       Chord voicing (see --list-voicings), default Voice-Led
  --rhythm NAME        Chord rhythm (see --list-rhythms), default Sustain
  --bass NAME          Bass style (see --list-bass), default Roots

OUTPUT
  --out DIR            Folder for .mid files (default ./melodee_out)
  --print              Print every melody as note names
)";
    }

    std::string lower (std::string s)
    {
        for (auto& c : s)
            c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
        return s;
    }

    int findByName (const std::vector<std::string>& names, const std::string& wanted)
    {
        const auto w = lower (wanted);
        for (size_t i = 0; i < names.size(); ++i)
            if (lower (names[i]) == w)
                return static_cast<int> (i);
        for (size_t i = 0; i < names.size(); ++i)
            if (lower (names[i]).find (w) != std::string::npos)
                return static_cast<int> (i);
        return -1;
    }

    int parseNote (const std::string& text, int fallback)
    {
        size_t used = 0;
        const int pc = parseNoteName (text, used);
        if (pc < 0)
            return fallback;
        const auto rest = text.substr (used);
        if (rest.empty())
            return 60 + pc;
        const int octave = std::atoi (rest.c_str());
        return (octave + 1) * 12 + pc;
    }

    std::string safeFileName (std::string s)
    {
        for (auto& c : s)
            if (! std::isalnum (static_cast<unsigned char> (c)))
                c = '_';
        return s;
    }

    void printMelody (const Melody& m, bool flats)
    {
        std::cout << "  " << m.name << "  (" << m.description << ")\n    ";
        int count = 0;
        for (const auto& n : m.notes)
        {
            std::cout << midiNoteName (n.pitch, flats) << "@" << std::fixed << std::setprecision (2) << n.start << " ";
            if (++count % 10 == 0)
                std::cout << "\n    ";
        }
        std::cout << "\n";
    }
} // namespace

int main (int argc, char** argv)
{
    std::map<std::string, std::string> opt;
    std::vector<std::string> transforms;
    std::vector<std::string> flags;

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a.rfind ("--", 0) != 0)
            continue;
        a = a.substr (2);

        const bool hasValue = i + 1 < argc && std::string (argv[i + 1]).rfind ("--", 0) != 0;
        if (a == "transform" && hasValue)
            transforms.push_back (argv[++i]);
        else if (hasValue)
            opt[a] = argv[++i];
        else
            flags.push_back (a);
    }

    auto has = [&] (const std::string& f) { return std::find (flags.begin(), flags.end(), f) != flags.end() || opt.count (f) > 0; };
    auto num = [&] (const std::string& k, double def) { return opt.count (k) ? std::atof (opt[k].c_str()) : def; };

    if (argc < 2 || has ("help") || has ("h"))
    {
        printUsage();
        return 0;
    }

    // ---- Listings --------------------------------------------------------------
    if (has ("list-scales"))
    {
        std::string category;
        for (const auto& s : allScales())
        {
            if (s.category != category)
            {
                category = s.category;
                std::cout << "\n[" << category << "]\n";
            }
            std::cout << "  " << std::left << std::setw (28) << s.name << Scale (0, s).noteNames (false);
            if (! s.aliases.empty())
                std::cout << "   (" << s.aliases << ")";
            std::cout << "\n";
        }
        std::cout << "\n" << allScales().size() << " scales\n";
        return 0;
    }

    if (has ("list-genres"))
    {
        for (const auto& g : progressionGenres())
            std::cout << "  " << g << " (" << progressionsInGenre (g).size() << ")\n";
        return 0;
    }

    if (has ("list-progressions"))
    {
        const auto& lib = progressionLibrary();
        const auto filter = opt.count ("list-progressions") ? opt["list-progressions"] : "";
        const auto indices = filter.empty() ? searchProgressions ("") : searchProgressions (filter);
        std::string genre;
        for (int i : indices)
        {
            const auto& p = lib[static_cast<size_t> (i)];
            if (p.genre != genre)
            {
                genre = p.genre;
                std::cout << "\n[" << genre << "]\n";
            }
            std::cout << "  " << std::left << std::setw (30) << p.name << std::setw (44) << p.numerals << p.scale << "\n";
        }
        std::cout << "\n" << indices.size() << " progressions\n";
        return 0;
    }

    if (has ("list-styles"))
    {
        for (int i = 0; i < numMelodyStyles(); ++i)
            std::cout << "  " << std::left << std::setw (22) << melodyStyleNames()[static_cast<size_t> (i)]
                      << melodyStyleDescription (static_cast<MelodyStyle> (i)) << "\n";
        return 0;
    }

    if (has ("list-transforms"))
    {
        for (int i = 0; i < numTransforms(); ++i)
            std::cout << "  " << std::left << std::setw (22) << transformNames()[static_cast<size_t> (i)]
                      << transformDescription (static_cast<MelodyTransform> (i)) << "\n";
        return 0;
    }

    if (has ("list-voicings"))  { for (const auto& n : voicingNames()) std::cout << "  " << n << "\n"; return 0; }
    if (has ("list-rhythms"))   { for (const auto& n : chordRhythmNames()) std::cout << "  " << n << "\n"; return 0; }
    if (has ("list-bass"))      { for (const auto& n : bassStyleNames()) std::cout << "  " << n << "\n"; return 0; }

    // ---- Key, scale, progression ---------------------------------------------------
    size_t used = 0;
    const int tonic = opt.count ("key") ? parseNoteName (opt["key"], used) : 0;
    if (tonic < 0)
    {
        std::cerr << "Unknown key: " << opt["key"] << "\n";
        return 1;
    }

    std::string progText = opt.count ("prog") ? opt["prog"] : "";
    std::string scaleName = opt.count ("scale") ? opt["scale"] : "";

    if (opt.count ("genre"))
    {
        const auto idx = progressionsInGenre (opt["genre"]);
        std::vector<int> found = idx.empty() ? searchProgressions (opt["genre"]) : idx;
        if (found.empty())
        {
            std::cerr << "No progressions for genre \"" << opt["genre"] << "\". Try --list-genres.\n";
            return 1;
        }
        const int pick = std::clamp (static_cast<int> (num ("pick", 1)), 1, static_cast<int> (found.size())) - 1;
        const auto& def = progressionLibrary()[static_cast<size_t> (found[static_cast<size_t> (pick)])];
        progText = def.numerals;
        if (scaleName.empty())
            scaleName = def.scale;
        std::cout << "Library: " << def.genre << " / " << def.name << "  (" << def.description << ")\n";
    }

    if (scaleName.empty())
        scaleName = "Major";

    const int scaleIndex = findScale (scaleName);
    if (scaleIndex < 0)
    {
        std::cerr << "Unknown scale: " << scaleName << ". Try --list-scales.\n";
        return 1;
    }

    if (progText.empty())
    {
        std::cerr << "Give a progression with --prog or --genre.\n";
        return 1;
    }

    const Scale key (tonic, scaleIndex);
    const bool flats = key.keyContext().preferFlats;
    const int beatsPerBar = std::clamp (static_cast<int> (num ("time", 4)), 1, 12);

    auto parsed = parseProgression (progText, key.keyContext(), beatsPerBar);
    for (const auto& e : parsed.errors)
        std::cerr << "Warning: " << e << "\n";
    if (parsed.progression.empty())
    {
        std::cerr << "The progression is empty.\n";
        return 1;
    }

    const Progression prog = repeatProgression (parsed.progression, std::clamp (static_cast<int> (num ("loops", 1)), 1, 8));

    std::cout << "Key:    " << key.name (flats) << "  (" << key.noteNames (flats) << ")\n";
    std::cout << "Chords: " << parsed.progression.toChordNames() << "\n";
    std::cout << "Roman:  " << parsed.progression.toRomanNumerals (key.keyContext()) << "\n";

    // ---- Settings ----------------------------------------------------------------
    MelodySettings ms;
    ms.beatsPerBar = beatsPerBar;
    ms.lowNote = opt.count ("low") ? parseNote (opt["low"], 60) : 60;
    ms.highNote = opt.count ("high") ? parseNote (opt["high"], 84) : 84;
    ms.density = num ("density", 0.5);
    ms.syncopation = num ("syncopation", 0.5);
    ms.leaps = num ("leaps", 0.5);
    ms.chordTones = num ("chord-tones", 0.5);
    ms.repetition = num ("repetition", 0.5);
    ms.rests = num ("rests", 0.5);
    ms.swing = num ("swing", 0.5);
    ms.noteLength = num ("length", 0.5);
    ms.humanize = num ("humanize", 0.15);

    if (opt.count ("contour"))
        if (const int c = findByName (contourNames(), opt["contour"]); c >= 0)
            ms.contour = static_cast<Contour> (c);

    if (opt.count ("form"))
    {
        auto f = lower (opt["form"]);
        if (f == "callresponse" || f == "call")
            f = "call & response";
        if (f == "through")
            f = "through-composed";
        if (const int c = findByName (phraseFormNames(), f); c >= 0)
            ms.form = static_cast<PhraseForm> (c);
    }

    const MelodyStyle style = opt.count ("style") ? melodyStyleFromName (opt["style"]) : MelodyStyle::Mixed;
    const int count = std::clamp (static_cast<int> (num ("variations", 8)), 1, 64);
    const auto seed = static_cast<std::uint32_t> (num ("seed", 1));

    AccompanimentSettings as;
    as.beatsPerBar = beatsPerBar;
    if (opt.count ("voicing"))
        if (const int v = findByName (voicingNames(), opt["voicing"]); v >= 0)
            as.voicing = static_cast<VoicingStyle> (v);
    if (opt.count ("rhythm"))
        if (const int v = findByName (chordRhythmNames(), opt["rhythm"]); v >= 0)
            as.rhythm = static_cast<ChordRhythm> (v);
    if (opt.count ("bass"))
        if (const int v = findByName (bassStyleNames(), opt["bass"]); v >= 0)
            as.bass = static_cast<BassStyle> (v);

    // ---- Generate --------------------------------------------------------------------
    auto melodies = generateVariations (prog, key, ms, style, count, seed);

    if (! melodies.empty())
    {
        std::vector<MelodyTransform> todo;
        for (const auto& t : transforms)
        {
            if (lower (t) == "all")
            {
                for (int i = 0; i < numTransforms(); ++i)
                    todo.push_back (static_cast<MelodyTransform> (i));
            }
            else if (const int idx = findByName (transformNames(), t); idx >= 0)
            {
                todo.push_back (static_cast<MelodyTransform> (idx));
            }
            else
            {
                std::cerr << "Unknown transform: " << t << " (see --list-transforms)\n";
            }
        }

        const Melody base = melodies.front();
        for (size_t i = 0; i < todo.size(); ++i)
            melodies.push_back (transformMelody (base, todo[i], prog, key, ms, mixSeed (seed, 1000 + static_cast<std::uint32_t> (i))));
    }

    const auto chords = renderChords (prog, as, seed);
    const auto bass = renderBass (prog, as, seed);

    const std::filesystem::path outDir = opt.count ("out") ? std::filesystem::path (opt["out"]) : std::filesystem::path ("melodee_out");
    std::error_code ec;
    std::filesystem::create_directories (outDir, ec);
    const auto shownDir = std::filesystem::absolute (outDir, ec).string();

    if (ec || ! std::filesystem::is_directory (outDir, ec))
    {
        std::cerr << "Can't create the output folder \"" << shownDir << "\".\n"
                  << "Choose a folder you can write to with --out, for example --out \"%USERPROFILE%\\Documents\\Melodee\"\n";
        return 1;
    }

    const double bpm = num ("bpm", 120);
    const MidiTrack chordTrack { "Chords", 1, 0, chords };
    const MidiTrack bassTrack { "Bass", 2, 33, bass };

    int failures = 0;
    auto save = [&] (const std::filesystem::path& path, const std::vector<MidiTrack>& tracks) {
        if (! saveMidiFile (path.string(), tracks, bpm, beatsPerBar))
        {
            std::cerr << "Could not write " << path.string() << "\n";
            ++failures;
        }
    };

    save (outDir / "chords.mid", { chordTrack });
    if (! bass.empty())
        save (outDir / "bass.mid", { bassTrack });

    std::vector<std::string> lines;
    for (size_t i = 0; i < melodies.size(); ++i)
    {
        const auto& m = melodies[i];
        std::ostringstream file;
        file << std::setw (2) << std::setfill ('0') << (i + 1) << "_" << safeFileName (m.name.substr (m.name.find (' ') + 1)) << ".mid";

        std::vector<MidiTrack> tracks { { "Melody", 0, 0, m.notes }, chordTrack };
        if (! bass.empty())
            tracks.push_back (bassTrack);

        save (outDir / file.str(), tracks);
        lines.push_back (file.str() + "   " + std::to_string (m.notes.size()) + " notes   " + m.description);
    }

    if (failures > 0)
    {
        std::cerr << failures << " file(s) could not be written to " << shownDir << "\n";
        return 1;
    }

    std::cout << "\nWrote " << melodies.size() << " melodies to " << shownDir << "\n";
    for (size_t i = 0; i < melodies.size(); ++i)
    {
        if (has ("print"))
            printMelody (melodies[i], flats);
        else
            std::cout << "  " << lines[i] << "\n";
    }

    return 0;
}
