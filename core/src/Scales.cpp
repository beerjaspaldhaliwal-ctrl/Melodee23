#include "melodee/Scales.h"

#include <algorithm>
#include <cstdlib>

namespace melodee
{

namespace
{
    std::vector<int> rotate (const std::vector<int>& parent, int mode)
    {
        std::vector<int> out;
        const int n = static_cast<int> (parent.size());
        const int base = parent[static_cast<size_t> (mode % n)];
        for (int i = 0; i < n; ++i)
            out.push_back (mod12 (parent[static_cast<size_t> ((mode + i) % n)] - base));
        return out;
    }

    bool hasMinorThird (const std::vector<int>& iv)
    {
        const bool m3 = std::find (iv.begin(), iv.end(), 3) != iv.end();
        const bool M3 = std::find (iv.begin(), iv.end(), 4) != iv.end();
        return m3 && ! M3;
    }

    struct Builder
    {
        std::vector<ScaleDef> scales;
        std::vector<std::string> categories;
        std::string currentCategory;

        void category (const std::string& name)
        {
            currentCategory = name;
            categories.push_back (name);
        }

        void add (const std::string& name, std::vector<int> intervals, const std::string& aliases, const std::string& description)
        {
            ScaleDef d;
            d.name = name;
            d.category = currentCategory;
            d.intervals = std::move (intervals);
            d.aliases = aliases;
            d.description = description;
            d.minorTonality = hasMinorThird (d.intervals);
            scales.push_back (std::move (d));
        }

        void addMinor (const std::string& name, std::vector<int> intervals, const std::string& aliases, const std::string& description)
        {
            add (name, std::move (intervals), aliases, description);
            scales.back().minorTonality = true;
        }

        struct Mode { const char* name; const char* aliases; const char* description; };

        void addModes (const std::vector<int>& parent, std::initializer_list<Mode> modes)
        {
            int index = 0;
            for (const auto& m : modes)
                add (m.name, rotate (parent, index++), m.aliases, m.description);
        }
    };

    std::vector<ScaleDef> buildScales (std::vector<std::string>& categoriesOut)
    {
        Builder b;

        b.category ("Major & Minor");
        b.add ("Major", { 0, 2, 4, 5, 7, 9, 11 }, "Ionian, Bilawal, Ajam, Gong heptatonic", "The major scale. Bright, stable, the basis of most pop.");
        b.add ("Natural Minor", { 0, 2, 3, 5, 7, 8, 10 }, "Aeolian, Minor, Asavari, Ethiopian Geez", "The natural minor scale. Dark, emotional.");
        b.add ("Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 }, "Nahawand, Mohammedan", "Minor with a raised 7th: dramatic, classical, trap and metal.");
        b.add ("Melodic Minor", { 0, 2, 3, 5, 7, 9, 11 }, "Jazz Minor, Hawaiian", "Minor with raised 6th and 7th. Smooth jazz minor sound.");
        b.add ("Harmonic Major", { 0, 2, 4, 5, 7, 8, 11 }, "Ionian b6", "Major with a flat 6th: bittersweet, cinematic.");

        b.category ("Church Modes");
        b.addModes ({ 0, 2, 4, 5, 7, 9, 11 }, {
            { "Ionian", "Major mode", "Mode 1 of major. Same notes as the major scale." },
            { "Dorian", "Kafi, Russian minor", "Minor with a bright 6th. Funk, soul, house, Celtic." },
            { "Phrygian", "Bhairavi, Kurd", "Minor with a flat 2nd. Dark, Spanish, metal, drill." },
            { "Lydian", "Kalyan, Yaman", "Major with a sharp 4th. Dreamy, floating, film scores." },
            { "Mixolydian", "Khamaj, Dominant scale", "Major with a flat 7th. Rock, blues, funk, Celtic." },
            { "Aeolian", "Natural minor mode", "Mode 6 of major. Same notes as natural minor." },
            { "Locrian", "Half-diminished mode", "Diminished tonic. Unstable, metal and tension." },
        });

        b.category ("Harmonic Minor Modes");
        b.addModes ({ 0, 2, 3, 5, 7, 8, 11 }, {
            { "Harmonic Minor (Mode 1)", "Aeolian #7", "Mode 1 of harmonic minor." },
            { "Locrian Natural 6", "Locrian #6", "Mode 2 of harmonic minor." },
            { "Ionian #5", "Augmented Major, Ionian Augmented", "Mode 3 of harmonic minor." },
            { "Dorian #4", "Ukrainian Dorian, Romanian Minor, Altered Dorian, Nikriz, Dorian #11", "Mode 4 of harmonic minor. Eastern European and klezmer." },
            { "Phrygian Dominant", "Spanish Gypsy, Freygish, Hijaz, Ahava Rabbah, Mixolydian b9 b13", "Mode 5 of harmonic minor. Flamenco, Middle Eastern, metal." },
            { "Lydian #2", "Lydian #9", "Mode 6 of harmonic minor." },
            { "Super Locrian bb7", "Ultralocrian, Altered Diminished", "Mode 7 of harmonic minor." },
        });

        b.category ("Melodic Minor Modes");
        b.addModes ({ 0, 2, 3, 5, 7, 9, 11 }, {
            { "Melodic Minor (Mode 1)", "Jazz Minor mode", "Mode 1 of melodic minor." },
            { "Dorian b2", "Phrygian Natural 6, Javanese", "Mode 2 of melodic minor." },
            { "Lydian Augmented", "Lydian #5", "Mode 3 of melodic minor. Ethereal." },
            { "Lydian Dominant", "Overtone, Acoustic, Mixolydian #11, Lydian b7", "Mode 4 of melodic minor. Jazz, video game, Simpsons-like." },
            { "Mixolydian b6", "Aeolian Dominant, Hindu, Melodic Major", "Mode 5 of melodic minor. Hopeful-sad." },
            { "Locrian Natural 2", "Half-Diminished, Aeolian b5, Locrian #2", "Mode 6 of melodic minor. For m7b5 chords." },
            { "Altered", "Super Locrian, Altered Dominant, Diminished Whole-Tone", "Mode 7 of melodic minor. Jazz altered dominants." },
        });

        b.category ("Harmonic Major Modes");
        b.addModes ({ 0, 2, 4, 5, 7, 8, 11 }, {
            { "Harmonic Major (Mode 1)", "Ionian b6 mode", "Mode 1 of harmonic major." },
            { "Dorian b5", "Locrian #2 #6", "Mode 2 of harmonic major." },
            { "Phrygian b4", "Altered Natural 5", "Mode 3 of harmonic major." },
            { "Lydian b3", "Lydian Minor, Melodic Minor #4", "Mode 4 of harmonic major." },
            { "Mixolydian b2", "Mixolydian b9", "Mode 5 of harmonic major." },
            { "Lydian Augmented #2", "Lydian #2 #5", "Mode 6 of harmonic major." },
            { "Locrian bb7", "Locrian Diminished 7", "Mode 7 of harmonic major." },
        });

        b.category ("Double Harmonic Modes");
        b.addModes ({ 0, 1, 4, 5, 7, 8, 11 }, {
            { "Double Harmonic Major", "Byzantine, Arabic, Bhairav, Hijaz Kar, Gypsy Major", "Two augmented 2nds. Middle Eastern, Indian, exotic." },
            { "Lydian #2 #6", "Double Harmonic mode 2", "Mode 2 of double harmonic major." },
            { "Ultraphrygian", "Phrygian b4 bb7", "Mode 3 of double harmonic major." },
            { "Hungarian Minor", "Double Harmonic Minor, Gypsy Minor, Algerian heptatonic", "Minor with #4 and raised 7th. Dark and exotic." },
            { "Oriental", "Double Harmonic mode 5", "Mode 5 of double harmonic major." },
            { "Ionian #2 #5", "Double Harmonic mode 6", "Mode 6 of double harmonic major." },
            { "Locrian bb3 bb7", "Double Harmonic mode 7", "Mode 7 of double harmonic major." },
        });

        b.category ("Pentatonic");
        b.add ("Major Pentatonic", { 0, 2, 4, 7, 9 }, "Gong, Mongolian, Tezeta Major", "Five-note major. Folk, country, pop hooks, Afrobeats.");
        b.add ("Minor Pentatonic", { 0, 3, 5, 7, 10 }, "Yu", "Five-note minor. Rock, blues, hip-hop, R&B riffs.");
        b.add ("Egyptian", { 0, 2, 5, 7, 10 }, "Suspended Pentatonic, Shang, Slendro (approx.)", "Suspended pentatonic. Open, ancient sound.");
        b.add ("Blues Minor Pentatonic", { 0, 3, 5, 8, 10 }, "Man Gong, Jue", "Pentatonic mode with a flat 6th.");
        b.add ("Blues Major Pentatonic", { 0, 2, 5, 7, 9 }, "Ritusen, Yo, Zhi, Scottish Pentatonic", "Pentatonic mode without a 3rd.");
        b.add ("Dominant Pentatonic", { 0, 2, 4, 7, 10 }, "Mixolydian Pentatonic", "Pentatonic for dominant 7th chords.");
        b.add ("Minor 6 Pentatonic", { 0, 3, 5, 7, 9 }, "Dorian Pentatonic", "Minor pentatonic with a major 6th. Neo-soul and jazz.");
        b.add ("Major b6 Pentatonic", { 0, 2, 4, 7, 8 }, "", "Major pentatonic with a flat 6th.");
        b.add ("Minor Major 7 Pentatonic", { 0, 3, 5, 7, 11 }, "", "Minor pentatonic with a major 7th.");

        b.category ("Blues");
        b.addMinor ("Blues", { 0, 3, 5, 6, 7, 10 }, "Minor Blues, Hexatonic Blues", "Minor pentatonic plus the flat 5 blue note.");
        b.add ("Major Blues", { 0, 2, 3, 4, 7, 9 }, "Major Blues Hexatonic", "Major pentatonic plus the flat 3 blue note.");
        b.add ("Nine-Note Blues", { 0, 2, 3, 4, 5, 6, 7, 9, 10 }, "Composite Blues", "Major and minor blues combined.");
        b.addMinor ("Blues Dorian", { 0, 2, 3, 5, 6, 7, 9, 10 }, "Dorian Blues", "Dorian with the flat 5 blue note.");

        b.category ("Bebop");
        b.add ("Bebop Dominant", { 0, 2, 4, 5, 7, 9, 10, 11 }, "Bebop Mixolydian", "Mixolydian with a passing major 7th.");
        b.add ("Bebop Major", { 0, 2, 4, 5, 7, 8, 9, 11 }, "Bebop Ionian", "Major with a passing #5.");
        b.addMinor ("Bebop Dorian", { 0, 2, 3, 4, 5, 7, 9, 10 }, "Bebop Minor", "Dorian with a passing major 3rd.");
        b.addMinor ("Bebop Melodic Minor", { 0, 2, 3, 5, 7, 8, 9, 11 }, "", "Melodic minor with a passing #5.");
        b.addMinor ("Bebop Harmonic Minor", { 0, 2, 3, 5, 7, 8, 10, 11 }, "Bebop Natural Minor", "Natural minor with a passing major 7th.");

        b.category ("Symmetric");
        b.add ("Whole Tone", { 0, 2, 4, 6, 8, 10 }, "Messiaen Mode 1", "All whole steps. Dreamy, unresolved.");
        b.addMinor ("Diminished (Whole-Half)", { 0, 2, 3, 5, 6, 8, 9, 11 }, "Octatonic, Diminished, Messiaen Mode 2 rotation", "For diminished 7th chords.");
        b.add ("Diminished (Half-Whole)", { 0, 1, 3, 4, 6, 7, 9, 10 }, "Dominant Diminished, Messiaen Mode 2", "For 7b9 dominant chords.");
        b.add ("Augmented", { 0, 3, 4, 7, 8, 11 }, "Hexatonic Augmented", "Alternating minor 3rds and half steps.");
        b.add ("Augmented Inverse", { 0, 1, 4, 5, 8, 9 }, "Hexatonic Augmented mode 2", "Alternating half steps and minor 3rds.");
        b.add ("Tritone", { 0, 1, 4, 6, 7, 10 }, "Petrushka", "Two major triads a tritone apart.");
        b.add ("Two-Semitone Tritone", { 0, 1, 2, 6, 7, 8 }, "", "Chromatic clusters a tritone apart.");
        b.add ("Messiaen Mode 3", { 0, 2, 3, 4, 6, 7, 8, 10, 11 }, "", "Nine-note symmetric scale.");
        b.add ("Messiaen Mode 4", { 0, 1, 2, 5, 6, 7, 8, 11 }, "", "Eight-note symmetric scale.");
        b.add ("Messiaen Mode 5", { 0, 1, 5, 6, 7, 11 }, "", "Six-note symmetric scale.");
        b.add ("Messiaen Mode 6", { 0, 2, 4, 5, 6, 8, 10, 11 }, "", "Eight-note symmetric scale.");
        b.add ("Messiaen Mode 7", { 0, 1, 2, 3, 5, 6, 7, 8, 9, 11 }, "", "Ten-note symmetric scale.");
        b.add ("Chromatic", { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, "Twelve-tone", "All twelve notes.");

        b.category ("Hexatonic");
        b.add ("Major Hexatonic", { 0, 2, 4, 5, 7, 9 }, "", "Major without the 7th.");
        b.add ("Minor Hexatonic", { 0, 2, 3, 5, 7, 10 }, "", "Dorian/Aeolian without the 6th.");
        b.add ("Ritsu", { 0, 1, 3, 5, 8, 10 }, "", "Japanese hexatonic mode.");
        b.add ("Prometheus", { 0, 2, 4, 6, 9, 10 }, "Mystic, Scriabin", "Scriabin's mystic-chord scale.");
        b.add ("Prometheus Neapolitan", { 0, 1, 4, 6, 9, 10 }, "", "Prometheus with a flat 2nd.");
        b.add ("Raga Kumud", { 0, 2, 4, 5, 7, 11 }, "Hexatonic Major no 6", "Major without the 6th.");

        b.category ("Japanese");
        b.add ("Hirajoshi", { 0, 2, 3, 7, 8 }, "", "Koto tuning. Classic Japanese minor sound.");
        b.add ("In", { 0, 1, 5, 7, 8 }, "Miyako-bushi, Sakura, Ambassel", "Japanese pentatonic with a flat 2nd.");
        b.add ("Insen", { 0, 1, 5, 7, 10 }, "", "Japanese pentatonic used in shakuhachi music.");
        b.add ("Iwato", { 0, 1, 5, 6, 10 }, "", "Very dark Japanese pentatonic.");
        b.add ("Yo", { 0, 2, 5, 7, 9 }, "", "Bright Japanese folk pentatonic.");
        b.add ("Kumoi", { 0, 2, 3, 7, 9 }, "Minor 6 Kumoi", "Japanese pentatonic, minor with a major 6th.");
        b.add ("Ryukyu", { 0, 4, 5, 7, 11 }, "Okinawan", "Okinawan pentatonic.");

        b.category ("World & Exotic");
        b.add ("Neapolitan Major", { 0, 1, 3, 5, 7, 9, 11 }, "", "Classical: flat 2nd with raised 6th and 7th.");
        b.add ("Neapolitan Minor", { 0, 1, 3, 5, 7, 8, 11 }, "", "Harmonic minor with a flat 2nd.");
        b.add ("Hungarian Major", { 0, 3, 4, 6, 7, 9, 10 }, "", "Major with #2 and #4.");
        b.add ("Hungarian Gypsy", { 0, 2, 3, 6, 7, 8, 10 }, "Gypsy, Romani minor", "Minor with #4 and flat 6th.");
        b.add ("Romanian Major", { 0, 1, 4, 6, 7, 9, 10 }, "Petrushka chord scale", "Lydian dominant with a flat 2nd.");
        b.add ("Persian", { 0, 1, 4, 5, 6, 8, 11 }, "", "Middle Eastern scale with a flat 5th.");
        b.add ("Enigmatic", { 0, 1, 4, 6, 8, 10, 11 }, "Verdi's Enigmatic", "Verdi's mysterious scale.");
        b.add ("Major Locrian", { 0, 2, 4, 5, 6, 8, 10 }, "Arabian", "Major with flat 5, 6 and 7.");
        b.add ("Leading Whole Tone", { 0, 2, 4, 6, 8, 10, 11 }, "", "Whole tone plus a leading tone.");
        b.add ("Spanish 8-Tone", { 0, 1, 3, 4, 5, 6, 8, 10 }, "Spanish Phrygian, Jewish 8-tone", "Phrygian with both 3rds and a flat 5.");
        b.add ("Flamenco", { 0, 1, 3, 4, 5, 7, 8, 10 }, "Phrygian Major-Minor", "Phrygian with an added major 3rd.");
        b.add ("Algerian", { 0, 2, 3, 5, 6, 7, 8, 11 }, "", "North African eight-note minor.");
        b.add ("Todi", { 0, 1, 3, 6, 7, 8, 11 }, "Thaat Todi", "Indian thaat Todi.");
        b.add ("Purvi", { 0, 1, 4, 6, 7, 8, 11 }, "Thaat Purvi", "Indian thaat Purvi.");
        b.add ("Marva", { 0, 1, 4, 6, 7, 9, 11 }, "Thaat Marwa", "Indian thaat Marva.");
        b.add ("Pelog", { 0, 1, 3, 7, 8 }, "Balinese, Pelog Selisir (approx.)", "Gamelan scale (12-TET approximation).");
        b.add ("Chinese", { 0, 4, 6, 7, 11 }, "", "Lydian-flavoured pentatonic.");
        b.add ("Tezeta Minor", { 0, 2, 3, 7, 8 }, "Ethiopian", "Ethiopian qenet (12-TET approximation).");
        b.add ("Bhairavi Thaat", { 0, 1, 3, 5, 7, 8, 10 }, "", "Same notes as Phrygian; Indian name.");
        b.add ("Kafi Thaat", { 0, 2, 3, 5, 7, 9, 10 }, "", "Same notes as Dorian; Indian name.");

        categoriesOut = b.categories;
        return b.scales;
    }

    struct ScaleTable
    {
        std::vector<std::string> categories;
        std::vector<ScaleDef> scales;

        ScaleTable() { scales = buildScales (categories); }
    };

    const ScaleTable& table()
    {
        static const ScaleTable t;
        return t;
    }
} // namespace

PitchMask ScaleDef::mask() const noexcept
{
    PitchMask m = 0;
    for (int i : intervals)
        m |= pcBit (i);
    return m;
}

const std::vector<ScaleDef>& allScales() { return table().scales; }

const std::vector<std::string>& scaleCategories() { return table().categories; }

int findScale (std::string_view nameOrAlias)
{
    const auto target = trim (nameOrAlias);
    const auto& scales = allScales();

    for (size_t i = 0; i < scales.size(); ++i)
        if (equalsIgnoreCase (scales[i].name, target))
            return static_cast<int> (i);

    for (size_t i = 0; i < scales.size(); ++i)
    {
        std::string_view aliases (scales[i].aliases);
        size_t start = 0;
        while (start <= aliases.size())
        {
            const size_t comma = aliases.find (',', start);
            const auto alias = trim (aliases.substr (start, comma == std::string_view::npos ? std::string_view::npos : comma - start));
            if (! alias.empty() && equalsIgnoreCase (alias, target))
                return static_cast<int> (i);
            if (comma == std::string_view::npos)
                break;
            start = comma + 1;
        }
    }

    return -1;
}

Scale::Scale (int root, int scaleIndex)
{
    const auto& scales = allScales();
    if (scaleIndex < 0 || scaleIndex >= static_cast<int> (scales.size()))
        scaleIndex = 0;

    *this = Scale (root, scales[static_cast<size_t> (scaleIndex)]);
    defIndex = scaleIndex;
}

Scale::Scale (int root, const ScaleDef& d)
    : rootPc (mod12 (root)), steps (d.intervals), pcMask (maskTranspose (d.mask(), root)), minor (d.minorTonality)
{
    const auto& scales = allScales();
    for (size_t i = 0; i < scales.size(); ++i)
        if (&scales[i] == &d)
            defIndex = static_cast<int> (i);
}

const ScaleDef& Scale::def() const { return allScales()[static_cast<size_t> (defIndex)]; }

KeyContext Scale::keyContext() const noexcept
{
    KeyContext k;
    k.tonic = rootPc;
    k.minor = minor;
    k.preferFlats = keyUsesFlats (rootPc, minor);
    return k;
}

std::string Scale::name (bool preferFlats) const { return pitchClassName (rootPc, preferFlats) + " " + def().name; }

std::string Scale::noteNames (bool preferFlats) const
{
    std::string s;
    for (int step : steps)
    {
        if (! s.empty())
            s += ' ';
        s += pitchClassName (rootPc + step, preferFlats);
    }
    return s;
}

int Scale::stepFrom (int midiNote, int numSteps) const noexcept { return stepAlongMask (pcMask, midiNote, numSteps); }

int Scale::snap (int midiNote) const noexcept { return snapToMask (pcMask, midiNote); }

int Scale::degreeOf (int pitch) const noexcept
{
    const int rel = mod12 (pitch - rootPc);
    for (size_t i = 0; i < steps.size(); ++i)
        if (steps[i] == rel)
            return static_cast<int> (i);
    return -1;
}

int snapToMask (PitchMask mask, int midiNote, int preferDirection) noexcept
{
    if (mask == 0 || maskHas (mask, midiNote))
        return midiNote;

    for (int d = 1; d <= 12; ++d)
    {
        const int first = midiNote + (preferDirection < 0 ? -d : d);
        const int second = midiNote + (preferDirection < 0 ? d : -d);
        if (maskHas (mask, first))
            return first;
        if (maskHas (mask, second))
            return second;
    }
    return midiNote;
}

int stepAlongMask (PitchMask mask, int midiNote, int numSteps) noexcept
{
    if (mask == 0)
        return midiNote + numSteps;

    int note = snapToMask (mask, midiNote, numSteps >= 0 ? -1 : 1);
    const int dir = numSteps >= 0 ? 1 : -1;

    for (int i = 0; i < std::abs (numSteps); ++i)
    {
        do
        {
            note += dir;
        } while (! maskHas (mask, note));
    }
    return note;
}

} // namespace melodee
