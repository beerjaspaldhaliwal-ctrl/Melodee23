#include "melodee/Chords.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace melodee
{

namespace
{
    enum class Third { Major, Minor, Sus2, Sus4, None };
    enum class Fifth { Perfect, Flat, Sharp };

    struct Spec
    {
        Third third = Third::Major;
        Fifth fifth = Fifth::Perfect;
        int seventh = -1;            // 9 (diminished), 10 (minor), 11 (major)
        bool seventhRequested = false;
        bool majorSeventh = false;
        bool dim = false;
        bool six = false;
        bool sixNine = false;
        bool power = false;
        bool alt = false;
        bool sus24 = false;
        bool noThird = false;
        bool noFifth = false;
        int extension = 0;           // highest natural extension: 9, 11 or 13
        std::vector<int> alterations;
        std::vector<int> adds;
    };

    class Cursor
    {
    public:
        explicit Cursor (std::string_view t) : text (t) {}

        bool done() const noexcept { return pos >= text.size(); }
        char peek (size_t offset = 0) const noexcept { return pos + offset < text.size() ? text[pos + offset] : '\0'; }

        /** Case-insensitive match; consumes on success. */
        bool matchCI (std::string_view word)
        {
            if (pos + word.size() > text.size())
                return false;
            if (! equalsIgnoreCase (text.substr (pos, word.size()), word))
                return false;
            pos += word.size();
            return true;
        }

        bool lookCI (std::string_view word) const noexcept
        {
            return pos + word.size() <= text.size() && equalsIgnoreCase (text.substr (pos, word.size()), word);
        }

        bool matchCS (std::string_view word)
        {
            if (text.substr (pos, word.size()) != word)
                return false;
            pos += word.size();
            return true;
        }

        bool lookDigit (size_t offset = 0) const noexcept { return std::isdigit (static_cast<unsigned char> (peek (offset))) != 0; }

        int readNumber()
        {
            int n = 0;
            int digits = 0;
            while (lookDigit() && digits < 2)
            {
                n = n * 10 + (peek() - '0');
                ++pos;
                ++digits;
            }
            return digits > 0 ? n : -1;
        }

        void skip (size_t n = 1) noexcept { pos += n; }

    private:
        std::string_view text;
        size_t pos = 0;
    };

    void setExtension (Spec& s, int n)
    {
        s.seventhRequested = true;
        s.extension = std::max (s.extension, n);
    }

    /** Applies a bare chord number (7, 9, 11, 13, 6, 69...) that follows the quality. */
    bool applyNumber (Spec& s, int n)
    {
        switch (n)
        {
            case 5:  s.power = true; return true;
            case 6:  s.six = true; return true;
            case 7:  s.seventhRequested = true; return true;
            case 9:
            case 11:
            case 13: setExtension (s, n); return true;
            case 2:  s.adds.push_back (14); return true;  // "C2" is commonly an add9
            case 4:  s.third = Third::Sus4; return true;  // "C4" means sus4
            case 69: s.six = true; s.sixNine = true; s.adds.push_back (14); return true;
            default: return false;
        }
    }

    int addInterval (int accidental, int n)
    {
        switch (n)
        {
            case 2:  return 2 + accidental;
            case 4:  return 5 + accidental;
            case 6:  return 9 + accidental;
            case 9:  return 14 + accidental;
            case 11: return 17 + accidental;
            case 13: return 21 + accidental;
            default: return -1;
        }
    }

    bool parseSpec (std::string_view text, Spec& s)
    {
        Cursor c (text);

        while (! c.done() && c.peek() == ' ')
            c.skip();

        // --- Quality --------------------------------------------------------
        auto readMajorSeventhNumber = [&]() {
            // after "maj": optional 7/9/11/13 (or nothing = plain major triad)
            if (c.lookDigit())
            {
                const int n = c.readNumber();
                if (n == 7 || n == 9 || n == 11 || n == 13)
                {
                    s.majorSeventh = true;
                    s.seventhRequested = true;
                    if (n > 7)
                        s.extension = std::max (s.extension, n);
                    return true;
                }
                if (n == 6)
                {
                    s.six = true;
                    return true;
                }
                return false;
            }
            return true;
        };

        if (c.lookCI ("maj"))
        {
            c.matchCI ("maj");
            c.matchCI ("or");
            if (! readMajorSeventhNumber())
                return false;
        }
        else if (c.matchCI ("min") || c.matchCS ("mi") || c.matchCS ("-") || c.matchCS ("m"))
        {
            s.third = Third::Minor;

            // minor-major: mMaj7, m(maj7), mM7, -maj7
            const bool bracket = c.peek() == '(';
            if (bracket && (c.lookCI ("(maj") || c.lookCI ("(M")))
                c.skip();

            if (c.matchCI ("maj") || c.matchCS ("M") || (c.lookCI ("ma") && c.lookDigit (2) && c.matchCI ("ma")))
            {
                if (! c.lookDigit())
                {
                    s.majorSeventh = true;
                    s.seventhRequested = true;
                }
                else if (! readMajorSeventhNumber())
                {
                    return false;
                }
            }

            if (bracket && c.peek() == ')')
                c.skip();
        }
        else if (c.matchCS ("M") || (c.lookCI ("ma") && c.lookDigit (2) && c.matchCI ("ma")))
        {
            if (! readMajorSeventhNumber())
                return false;
        }
        else if (c.matchCI ("dim") || (c.peek() == 'o' && ! c.lookCI ("omit") && (c.skip(), true)))
        {
            s.dim = true;
            s.third = Third::Minor;
            s.fifth = Fifth::Flat;
        }
        else if (c.matchCI ("aug") || c.matchCS ("+"))
        {
            s.fifth = Fifth::Sharp;
        }
        else if (c.lookCI ("alt"))
        {
            // handled in the modifier loop
        }

        // --- Main number -----------------------------------------------------
        if (c.lookDigit())
        {
            if (c.peek() == '6' && c.peek (1) == '/' && c.peek (2) == '9')
            {
                c.skip (3);
                applyNumber (s, 69);
            }
            else
            {
                const int n = c.readNumber();
                if (! applyNumber (s, n))
                    return false;
            }
        }

        // --- Modifiers -------------------------------------------------------
        while (! c.done())
        {
            const char ch = c.peek();

            if (ch == '(' || ch == ')' || ch == ',' || ch == ' ' || ch == '/' || ch == '[' || ch == ']' || ch == '.')
            {
                c.skip();
                continue;
            }

            if (c.matchCI ("sus"))
            {
                if (c.peek() == '2')
                {
                    c.skip();
                    s.third = Third::Sus2;
                    if (c.peek() == '4')
                    {
                        c.skip();
                        s.sus24 = true;
                    }
                }
                else
                {
                    if (c.peek() == '4')
                        c.skip();
                    s.third = Third::Sus4;
                    if (c.peek() == '2')
                    {
                        c.skip();
                        s.third = Third::Sus2;
                        s.sus24 = true;
                    }
                }
                continue;
            }

            if (c.matchCI ("add"))
            {
                int acc = 0;
                if (c.peek() == 'b' || c.peek() == '-') { acc = -1; c.skip(); }
                else if (c.peek() == '#' || c.peek() == '+') { acc = 1; c.skip(); }
                const int iv = addInterval (acc, c.readNumber());
                if (iv < 0)
                    return false;
                s.adds.push_back (iv);
                continue;
            }

            if (c.lookCI ("maj") || (c.peek() == 'M' && c.lookDigit (1)) || (c.lookCI ("ma") && c.lookDigit (2)))
            {
                if (! c.matchCI ("maj") && ! (c.lookCI ("ma") && c.lookDigit (2) && c.matchCI ("ma")))
                    c.matchCS ("M");
                if (! c.lookDigit())
                {
                    s.majorSeventh = true;
                    s.seventhRequested = true;
                }
                else if (! readMajorSeventhNumber())
                {
                    return false;
                }
                continue;
            }

            if (c.matchCI ("alt"))
            {
                s.alt = true;
                continue;
            }

            if (c.matchCI ("omit") || c.matchCI ("no"))
            {
                const int n = c.readNumber();
                if (n == 3) s.noThird = true;
                else if (n == 5) s.noFifth = true;
                else return false;
                continue;
            }

            if (c.matchCI ("dim"))
            {
                s.dim = true;
                s.third = Third::Minor;
                s.fifth = Fifth::Flat;
                if (c.lookDigit() && ! applyNumber (s, c.readNumber()))
                    return false;
                continue;
            }

            if (ch == 'b' || ch == '-' || ch == '#' || ch == '+')
            {
                const int acc = (ch == 'b' || ch == '-') ? -1 : 1;
                c.skip();

                if (! c.lookDigit())
                {
                    if (acc > 0)  // trailing "+" means #5
                    {
                        s.fifth = Fifth::Sharp;
                        continue;
                    }
                    return false;
                }

                const int n = c.readNumber();
                if (n == 5)                { s.fifth = acc < 0 ? Fifth::Flat : Fifth::Sharp; }
                else if (n == 9)           { s.alterations.push_back (acc < 0 ? 13 : 15); }
                else if (n == 11 && acc > 0) { s.alterations.push_back (18); }
                else if (n == 4 && acc > 0)  { s.alterations.push_back (18); }
                else if (n == 13 && acc < 0) { s.alterations.push_back (20); }
                else if (n == 6 && acc < 0)  { s.alterations.push_back (20); }
                else return false;
                continue;
            }

            if (c.lookDigit())
            {
                const int n = c.readNumber();
                if (n == 6)
                    s.six = true;
                else if (n == 7)
                    s.seventhRequested = true;
                else if (n == 9 || n == 11 || n == 13)
                {
                    if (s.seventhRequested || s.majorSeventh)
                        s.extension = std::max (s.extension, n);
                    else
                        s.adds.push_back (addInterval (0, n));
                }
                else
                    return false;
                continue;
            }

            return false;
        }

        return true;
    }

    std::vector<int> buildIntervals (Spec& s)
    {
        if (s.power)
            return { 0, 7 };

        std::vector<int> iv { 0 };

        if (s.alt)
        {
            s.third = Third::Major;
            s.seventhRequested = true;
            s.majorSeventh = false;
        }

        if (s.seventhRequested)
        {
            if (s.majorSeventh)
                s.seventh = 11;
            else if (s.dim && s.extension == 0 && s.alterations.empty())
                s.seventh = 9;
            else
                s.seventh = 10;
        }

        if (! s.noThird)
        {
            switch (s.third)
            {
                case Third::Major: iv.push_back (4); break;
                case Third::Minor: iv.push_back (3); break;
                case Third::Sus2:  iv.push_back (2); if (s.sus24) iv.push_back (5); break;
                case Third::Sus4:  iv.push_back (5); break;
                case Third::None:  break;
            }
        }

        if (! s.noFifth && ! s.alt)
        {
            switch (s.fifth)
            {
                case Fifth::Perfect: iv.push_back (7); break;
                case Fifth::Flat:    iv.push_back (6); break;
                case Fifth::Sharp:   iv.push_back (8); break;
            }
        }

        if (s.six)
            iv.push_back (s.seventh >= 0 ? 21 : 9);

        if (s.seventh >= 0)
            iv.push_back (s.seventh);

        if (s.alt)
        {
            iv.push_back (13);
            iv.push_back (15);
            iv.push_back (20);
        }

        auto hasAlteration = [&] (int a) { return std::find (s.alterations.begin(), s.alterations.end(), a) != s.alterations.end(); };

        if (s.extension >= 9 && ! hasAlteration (13) && ! hasAlteration (15))
            iv.push_back (14);
        if (s.extension >= 11 && ! hasAlteration (18))
        {
            // Natural 11 on a 13th chord is only kept for minor chords (it clashes with a major 3rd).
            if (s.extension == 11 || s.third == Third::Minor)
                iv.push_back (17);
        }
        if (s.extension >= 13 && ! hasAlteration (20))
            iv.push_back (21);

        for (int a : s.alterations)
            iv.push_back (a);
        for (int a : s.adds)
            iv.push_back (a);

        std::sort (iv.begin(), iv.end());
        iv.erase (std::unique (iv.begin(), iv.end()), iv.end());
        return iv;
    }

    std::string canonicalSuffix (const Spec& s)
    {
        if (s.power)
            return "5";

        const bool minor = s.third == Third::Minor && ! s.noThird;
        const bool dimTriad = minor && s.fifth == Fifth::Flat;
        const std::string number = s.extension > 0 ? std::to_string (s.extension) : (s.seventh >= 0 ? "7" : "");

        auto hasAlteration = [&] (int a) { return std::find (s.alterations.begin(), s.alterations.end(), a) != s.alterations.end(); };

        std::string out;

        if (s.alt)
        {
            out = "7alt";
        }
        else if (dimTriad && s.seventh == 9)
        {
            out = "dim7";
        }
        else if (dimTriad && s.seventh == 10)
        {
            out = "m" + number + "b5";
        }
        else if (dimTriad && s.seventh == 11)
        {
            out = "dim(maj7)";
        }
        else if (dimTriad)
        {
            out = "dim";
        }
        else
        {
            if (minor)
                out += "m";

            if (s.seventh == 11)
                out += (minor ? "Maj" : "maj") + number;
            else if (s.seventh >= 0)
                out += number;

            if (s.six && s.seventh < 0)
                out += s.sixNine ? "6/9" : "6";

            if (s.fifth == Fifth::Sharp)
                out += (s.seventh < 0 && ! minor && out.empty()) ? "aug" : "#5";
            else if (s.fifth == Fifth::Flat)
                out += "b5";
        }

        if (s.third == Third::Sus2)
            out += s.sus24 ? "sus2sus4" : "sus2";
        else if (s.third == Third::Sus4)
            out += "sus4";

        if (! s.alt)
        {
            if (hasAlteration (13)) out += "b9";
            if (hasAlteration (15)) out += "#9";
            if (hasAlteration (18)) out += "#11";
            if (hasAlteration (20)) out += "b13";
        }

        std::vector<int> adds = s.adds;
        std::sort (adds.begin(), adds.end());
        adds.erase (std::unique (adds.begin(), adds.end()), adds.end());

        for (int a : adds)
        {
            if (s.sixNine && a == 14)
                continue;

            switch (a)
            {
                case 2:  out += "add2"; break;
                case 5:  out += "add4"; break;
                case 9:  out += "add6"; break;
                case 13: out += "addb9"; break;
                case 14: out += "add9"; break;
                case 15: out += "add#9"; break;
                case 17: out += "add11"; break;
                case 18: out += "add#11"; break;
                case 20: out += "addb13"; break;
                case 21: out += "add13"; break;
                default: break;
            }
        }

        if (s.noThird && s.third != Third::Sus2 && s.third != Third::Sus4)
            out += "(no3)";
        if (s.noFifth)
            out += "(no5)";

        return out;
    }
} // namespace

//==============================================================================
std::string Chord::name() const
{
    if (noChord)
        return "N.C.";

    std::string n = pitchClassName (root, flats) + suffix;
    if (bass >= 0 && bass != root)
        n += "/" + pitchClassName (bass, flats);
    return n;
}

PitchMask Chord::mask() const noexcept
{
    if (noChord)
        return 0;

    PitchMask m = 0;
    for (int i : intervals)
        m |= pcBit (root + i);
    return m;
}

PitchMask Chord::coreMask() const noexcept
{
    if (noChord)
        return 0;

    PitchMask m = 0;
    for (int i : intervals)
        if (i < 12)
            m |= pcBit (root + i);
    return m;
}

PitchMask Chord::extensionMask() const noexcept
{
    if (noChord)
        return 0;

    PitchMask m = 0;
    for (int i : intervals)
        if (i >= 12)
            m |= pcBit (root + i);
    return static_cast<PitchMask> (m & ~coreMask());
}

bool Chord::hasInterval (int semitones) const noexcept
{
    for (int i : intervals)
        if (mod12 (i) == mod12 (semitones))
            return true;
    return false;
}

bool Chord::isMinor() const noexcept
{
    bool m3 = false, M3 = false;
    for (int i : intervals)
    {
        if (i == 3) m3 = true;
        if (i == 4) M3 = true;
    }
    return m3 && ! M3;
}

bool Chord::isDominant() const noexcept
{
    bool M3 = false, m7 = false;
    for (int i : intervals)
    {
        if (i == 4) M3 = true;
        if (i == 10) m7 = true;
    }
    return M3 && m7;
}

bool Chord::hasSeventh() const noexcept
{
    for (int i : intervals)
        if (i == 10 || i == 11 || (i == 9 && suffix.find ("dim7") != std::string::npos))
            return true;
    return false;
}

//==============================================================================
std::optional<std::vector<int>> parseChordSuffix (std::string_view suffix, std::string& canonical)
{
    const auto text = normaliseMusicText (suffix);
    Spec s;

    if (! parseSpec (text, s))
        return std::nullopt;

    auto iv = buildIntervals (s);
    canonical = canonicalSuffix (s);
    return iv;
}

std::optional<Chord> makeChord (int rootPc, std::string_view suffix, bool flats)
{
    std::string canonical;
    auto iv = parseChordSuffix (suffix, canonical);
    if (! iv)
        return std::nullopt;

    Chord c;
    c.root = mod12 (rootPc);
    c.intervals = std::move (*iv);
    c.suffix = canonical;
    c.flats = flats;
    return c;
}

std::optional<Chord> parseChordSymbol (std::string_view symbolIn)
{
    const auto symbol = trim (normaliseMusicText (symbolIn));
    if (symbol.empty())
        return std::nullopt;

    {
        const auto lower = toLower (symbol);
        if (lower == "n.c." || lower == "nc" || lower == "n.c" || lower == "x" || lower == "rest")
        {
            Chord c;
            c.noChord = true;
            c.intervals.clear();
            c.suffix.clear();
            return c;
        }
    }

    size_t used = 0;
    bool flat = false;
    const int root = parseNoteName (symbol, used, &flat);
    if (root < 0)
        return std::nullopt;

    std::string_view rest (symbol);
    rest.remove_prefix (used);

    // Slash chord: last '/' followed by a note name ("C/E", "Am7/G"), not "6/9".
    int bass = -1;
    bool bassFlat = false;
    const auto slash = rest.rfind ('/');
    if (slash != std::string_view::npos)
    {
        size_t bassUsed = 0;
        const auto bassText = rest.substr (slash + 1);
        const int b = parseNoteName (bassText, bassUsed, &bassFlat);
        if (b >= 0 && bassUsed == bassText.size())
        {
            bass = b;
            rest = rest.substr (0, slash);
        }
    }

    auto chord = makeChord (root, rest, flat || bassFlat);
    if (! chord)
        return std::nullopt;

    chord->bass = bass == root ? -1 : bass;
    if (! flat && symbol.size() > 1 && symbol[1] == '#')
        chord->flats = false;
    return chord;
}

//==============================================================================
std::optional<std::string> identifyChordSuffix (PitchMask rel)
{
    static const std::map<PitchMask, std::string> table = [] {
        std::map<PitchMask, std::string> t;
        auto add = [&t] (std::initializer_list<int> iv, const char* name) {
            PitchMask m = 0;
            for (int i : iv)
                m |= pcBit (i);
            t.emplace (m, name);
        };

        add ({ 0, 4, 7 }, "");
        add ({ 0, 3, 7 }, "m");
        add ({ 0, 3, 6 }, "dim");
        add ({ 0, 4, 8 }, "aug");
        add ({ 0, 2, 7 }, "sus2");
        add ({ 0, 5, 7 }, "sus4");
        add ({ 0, 4, 6 }, "b5");
        add ({ 0, 7 }, "5");
        add ({ 0, 4, 7, 11 }, "maj7");
        add ({ 0, 4, 7, 10 }, "7");
        add ({ 0, 3, 7, 10 }, "m7");
        add ({ 0, 3, 7, 11 }, "mMaj7");
        add ({ 0, 3, 6, 10 }, "m7b5");
        add ({ 0, 3, 6, 9 }, "dim7");
        add ({ 0, 3, 6, 11 }, "dim(maj7)");
        add ({ 0, 4, 8, 11 }, "maj7#5");
        add ({ 0, 4, 8, 10 }, "7#5");
        add ({ 0, 4, 6, 10 }, "7b5");
        add ({ 0, 4, 6, 11 }, "maj7b5");
        add ({ 0, 5, 7, 10 }, "7sus4");
        add ({ 0, 2, 7, 10 }, "7sus2");
        add ({ 0, 5, 7, 11 }, "maj7sus4");
        add ({ 0, 2, 7, 11 }, "maj7sus2");
        add ({ 0, 4, 7, 9 }, "6");
        add ({ 0, 3, 7, 9 }, "m6");
        add ({ 0, 3, 8 }, "m#5");
        add ({ 0, 3, 8, 10 }, "m7#5");
        add ({ 0, 3, 8, 11 }, "mMaj7#5");
        add ({ 0, 2, 6 }, "sus2b5");
        add ({ 0, 5, 6 }, "sus4b5");
        add ({ 0, 5, 8 }, "sus4#5");
        add ({ 0, 1, 7 }, "sus(b2)");
        add ({ 0, 1, 5, 7 }, "sus4(b9)");
        add ({ 0, 2, 4, 7 }, "add9");
        add ({ 0, 2, 3, 7 }, "madd9");
        add ({ 0, 4, 5, 7 }, "add11");
        add ({ 0, 3, 5, 7 }, "madd11");
        add ({ 0, 2, 6, 10 }, "7b5sus2");
        add ({ 0, 1, 4, 7 }, "addb9");
        add ({ 0, 3, 4, 7 }, "add#9");
        add ({ 0, 1, 4, 10 }, "7b9(no5)");
        add ({ 0, 4, 9, 10 }, "13(no5)");
        return t;
    }();

    const auto it = table.find (rel);
    if (it == table.end())
        return std::nullopt;
    return it->second;
}

std::string romanNumeralFor (const Chord& chord, const KeyContext& key)
{
    if (chord.noChord)
        return "N.C.";

    static const char* majorRef[12] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    static const char* minorRef[12] = { "I", "bII", "II", "III", "#III", "IV", "#IV", "V", "VI", "#VI", "VII", "#VII" };

    const int offset = mod12 (chord.root - key.tonic);
    std::string numeral = key.minor ? minorRef[offset] : majorRef[offset];

    // In minor, the diminished chord on the raised leading tone is written vii°, not #vii°.
    if (key.minor && offset == 11 && chord.hasInterval (3) && chord.hasInterval (6))
        numeral = "VII";

    const auto& suf = chord.suffix;
    const bool minorQuality = chord.isMinor();

    if (minorQuality)
        for (auto& ch : numeral)
            if (ch == 'I' || ch == 'V')
                ch = static_cast<char> (std::tolower (static_cast<unsigned char> (ch)));

    std::string rest = suf;
    if (startsWith (rest, "dim7"))
        rest = "\xC2\xB0" "7" + rest.substr (4);
    else if (startsWith (rest, "dim"))
        rest = "\xC2\xB0" + rest.substr (3);
    else if (rest.size() >= 3 && rest[0] == 'm' && rest.find ("b5") != std::string::npos && rest.find ("Maj") == std::string::npos)
        rest = "\xC3\xB8" + rest.substr (1, rest.find ("b5") - 1) + rest.substr (rest.find ("b5") + 2);
    else if (startsWith (rest, "aug"))
        rest = "+" + rest.substr (3);
    else if (minorQuality && ! rest.empty() && rest[0] == 'm')
        rest = rest.substr (1);

    std::string out = numeral + rest;
    if (chord.bass >= 0 && chord.bass != chord.root)
        out += "/" + pitchClassName (chord.bass, chord.flats);
    return out;
}

std::vector<Chord> diatonicChords (const Scale& scale, bool sevenths)
{
    std::vector<Chord> out;
    const auto& iv = scale.intervals();
    const int n = static_cast<int> (iv.size());
    const bool flats = scale.keyContext().preferFlats;

    if (n == 7)
    {
        // Heptatonic scales: stack thirds (every other scale note) on each degree.
        for (int degree = 0; degree < n; ++degree)
        {
            auto noteAt = [&] (int step) {
                const int idx = degree + step;
                return iv[static_cast<size_t> (idx % n)] + 12 * (idx / n);
            };

            const int rootSemis = noteAt (0);
            std::vector<int> rel { 0, noteAt (2) - rootSemis, noteAt (4) - rootSemis };
            if (sevenths)
                rel.push_back (noteAt (6) - rootSemis);

            PitchMask relMask = 0;
            for (int r : rel)
                relMask |= pcBit (r);

            const int root = mod12 (scale.root() + rootSemis);

            if (auto suffix = identifyChordSuffix (relMask))
            {
                if (auto made = makeChord (root, *suffix, flats))
                {
                    out.push_back (*made);
                    continue;
                }
            }

            static const char* names[12] = { "1", "b2", "2", "b3", "3", "4", "b5", "5", "#5", "6", "b7", "7" };
            Chord c;
            c.root = root;
            c.flats = flats;
            c.intervals = rel;
            c.suffix = "(";
            for (size_t i = 1; i < rel.size(); ++i)
                c.suffix += std::string (i > 1 ? " " : "") + names[mod12 (rel[i])];
            c.suffix += ")";
            out.push_back (c);
        }
        return out;
    }

    // Other scale sizes: on each scale note, use the first common chord whose tones all fit the scale.
    static const std::vector<std::string> triadOrder { "", "m", "dim", "aug", "sus4", "sus2", "5" };
    static const std::vector<std::string> seventhOrder { "maj7", "7", "m7", "m7b5", "dim7", "mMaj7", "maj7#5", "6", "m6", "7sus4" };

    for (int step : iv)
    {
        const int root = mod12 (scale.root() + step);
        std::optional<Chord> found;

        auto tryList = [&] (const std::vector<std::string>& list) {
            for (const auto& suffix : list)
            {
                auto c = makeChord (root, suffix, flats);
                if (c && (c->mask() & ~scale.mask()) == 0)
                    return c;
            }
            return std::optional<Chord>();
        };

        if (sevenths)
            found = tryList (seventhOrder);
        if (! found)
            found = tryList (triadOrder);
        if (found)
            out.push_back (*found);
    }

    return out;
}

const std::vector<std::string>& commonChordSuffixes()
{
    static const std::vector<std::string> suffixes {
        "", "m", "dim", "aug", "sus2", "sus4", "5",
        "6", "m6", "6/9", "m6/9",
        "7", "maj7", "m7", "mMaj7", "m7b5", "dim7", "7sus4", "7#5", "7b5", "maj7#5",
        "add9", "madd9", "9", "maj9", "m9", "9sus4", "7b9", "7#9", "7#11", "maj7#11",
        "11", "m11", "13", "maj13", "m13", "13sus4", "7b9b13", "7alt",
    };
    return suffixes;
}

} // namespace melodee
