#include "melodee/Progression.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace melodee
{

namespace
{
    int romanValue (std::string_view letters)
    {
        static const char* upper[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        for (int i = 0; i < 7; ++i)
            if (equalsIgnoreCase (letters, upper[i]))
                return i + 1;
        return 0;
    }

    bool isRomanLetter (char c) noexcept { return c == 'I' || c == 'V' || c == 'i' || c == 'v'; }

    /** Pitch class of a scale degree (1-7) in a key, with an accidental.
        In a minor key, degrees are read from natural minor and a flat on 3, 6 or 7 changes nothing
        (so "bVI" and "VI" both mean the 6th of natural minor). */
    int degreePc (const KeyContext& key, int degree, int accidental, bool diminishedQuality)
    {
        static constexpr int majorSteps[7] = { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr int minorSteps[7] = { 0, 2, 3, 5, 7, 8, 10 };

        const int idx = degree - 1;

        if (! key.minor)
            return mod12 (key.tonic + majorSteps[idx] + accidental);

        const bool loweredInMinor = degree == 3 || degree == 6 || degree == 7;
        int acc = accidental;

        if (loweredInMinor && acc < 0)
            acc = 0;
        if (degree == 7 && acc == 0 && diminishedQuality)
            acc = 1; // vii° in minor sits on the raised leading tone

        return mod12 (key.tonic + minorSteps[idx] + acc);
    }

    int readAccidentals (std::string_view s, size_t& pos)
    {
        int acc = 0;
        while (pos < s.size() && (s[pos] == 'b' || s[pos] == '#'))
        {
            acc += s[pos] == 'b' ? -1 : 1;
            ++pos;
        }
        return acc;
    }

    bool suffixIsDiminished (std::string_view suffix)
    {
        return startsWith (suffix, "dim") || (startsWith (suffix, "o") && ! startsWith (suffix, "omit"))
            || startsWith (suffix, "m7b5") || startsWith (suffix, "m9b5");
    }

    std::optional<Chord> parseRomanImpl (std::string_view token, const KeyContext& key, bool allowSlash)
    {
        size_t pos = 0;
        const int acc = readAccidentals (token, pos);

        const size_t numStart = pos;
        while (pos < token.size() && isRomanLetter (token[pos]))
            ++pos;

        if (pos == numStart)
            return std::nullopt;

        const auto letters = token.substr (numStart, pos - numStart);
        const bool allUpper = std::all_of (letters.begin(), letters.end(), [] (char c) { return std::isupper (static_cast<unsigned char> (c)) != 0; });
        const bool allLower = std::all_of (letters.begin(), letters.end(), [] (char c) { return std::islower (static_cast<unsigned char> (c)) != 0; });

        if (! allUpper && ! allLower)
            return std::nullopt;

        const int degree = romanValue (letters);
        if (degree == 0)
            return std::nullopt;

        std::string_view rest = token.substr (pos);
        std::string_view slashPart;
        bool hasSlash = false;

        const auto slash = rest.find ('/');
        if (slash != std::string_view::npos)
        {
            // "6/9" is part of the suffix, not a slash.
            const bool isSixNine = slash > 0 && rest[slash - 1] == '6' && slash + 1 < rest.size() && rest[slash + 1] == '9';
            if (! isSixNine)
            {
                if (! allowSlash)
                    return std::nullopt;
                slashPart = rest.substr (slash + 1);
                rest = rest.substr (0, slash);
                hasSlash = true;
            }
        }

        KeyContext ctx = key;
        int bassPc = -1;

        if (hasSlash)
        {
            size_t p = 0;
            const int bassAcc = readAccidentals (slashPart, p);
            if (p < slashPart.size() && std::isdigit (static_cast<unsigned char> (slashPart[p])) && slashPart.size() == p + 1)
            {
                const int bassDegree = slashPart[p] - '0';
                if (bassDegree < 1 || bassDegree > 7)
                    return std::nullopt;
                bassPc = degreePc (key, bassDegree, bassAcc, false);
            }
            else if (auto target = parseRomanImpl (slashPart, key, false))
            {
                ctx.tonic = target->root;
                ctx.minor = target->isMinor();
                ctx.preferFlats = keyUsesFlats (ctx.tonic, ctx.minor);
            }
            else
            {
                return std::nullopt;
            }
        }

        std::string suffix (rest);
        const bool dimQuality = suffixIsDiminished (suffix);

        if (allLower)
        {
            const bool alreadyQualified = dimQuality
                || (startsWith (suffix, "m") && ! startsWith (suffix, "maj"))
                || startsWith (suffix, "5")
                || startsWith (suffix, "+")
                || startsWith (suffix, "aug")
                || startsWith (suffix, "-");
            if (! alreadyQualified)
                suffix = "m" + suffix;
        }

        const int root = degreePc (ctx, degree, acc, allLower && dimQuality);
        const bool flats = acc < 0 ? true : (acc > 0 ? false : ctx.preferFlats);

        auto chord = makeChord (root, suffix, flats);
        if (! chord)
            return std::nullopt;

        if (bassPc >= 0 && bassPc != chord->root)
            chord->bass = bassPc;

        return chord;
    }

    struct Token
    {
        std::string text;
        double weight = 1.0;
        bool explicitWeight = false;
    };

    std::vector<std::string> splitWords (std::string_view text)
    {
        std::vector<std::string> words;
        std::string current;

        auto flush = [&] {
            if (! current.empty())
                words.push_back (current);
            current.clear();
        };

        int depth = 0; // keep "C7(b9, #11)" together

        for (char c : text)
        {
            if (c == '(')
                ++depth;
            else if (c == ')' && depth > 0)
                --depth;

            const bool separator = std::isspace (static_cast<unsigned char> (c)) || c == ',' || c == ';' || c == '>';
            if (separator && depth == 0)
                flush();
            else if (! (separator && std::isspace (static_cast<unsigned char> (c))))
                current += c;
        }

        flush();
        return words;
    }

    bool parsesAsChord (std::string_view word, const KeyContext& key)
    {
        if (word == "%")
            return true;
        const auto colon = word.find (':');
        return parseChordToken (word.substr (0, colon), key).has_value();
    }

    /** Splits "ii-V-I" into separate tokens, but keeps "C-7" (C minor 7) together. */
    std::vector<std::string> splitDashes (const std::vector<std::string>& words, const KeyContext& key)
    {
        std::vector<std::string> out;

        for (const auto& w : words)
        {
            if (w.find ('-') == std::string::npos || w == "-")
            {
                if (w != "-")
                    out.push_back (w);
                continue;
            }

            std::vector<std::string> parts;
            std::string part;
            for (char c : w)
            {
                if (c == '-')
                {
                    parts.push_back (part);
                    part.clear();
                }
                else
                {
                    part += c;
                }
            }
            parts.push_back (part);

            bool allChords = parts.size() > 1;
            for (const auto& p : parts)
                if (p.empty() || ! parsesAsChord (p, key))
                    allChords = false;

            if (allChords)
                out.insert (out.end(), parts.begin(), parts.end());
            else
                out.push_back (w);
        }

        return out;
    }

    bool isRepeatToken (std::string_view w, int& times)
    {
        if (w.size() < 2 || (w[0] != 'x' && w[0] != 'X' && w[0] != '*'))
            return false;

        int n = 0;
        for (size_t i = 1; i < w.size(); ++i)
        {
            if (! std::isdigit (static_cast<unsigned char> (w[i])))
                return false;
            n = n * 10 + (w[i] - '0');
        }

        times = n;
        return n >= 1 && n <= 64;
    }

    bool readWeight (std::string& word, double& weight)
    {
        const auto colon = word.find (':');
        if (colon == std::string::npos)
            return false;

        const auto number = word.substr (colon + 1);
        word = word.substr (0, colon);

        char* end = nullptr;
        const double v = std::strtod (number.c_str(), &end);
        if (end == number.c_str() || v <= 0.0 || v > 64.0)
            return false;

        weight = v;
        return true;
    }
} // namespace

//==============================================================================
int Progression::indexAt (double beat) const noexcept
{
    for (size_t i = 0; i < chords.size(); ++i)
        if (beat >= chords[i].start - 1e-9 && beat < chords[i].start + chords[i].length - 1e-9)
            return static_cast<int> (i);

    return chords.empty() ? -1 : (beat >= totalBeats - 1e-9 ? static_cast<int> (chords.size()) - 1 : -1);
}

const ChordEvent* Progression::at (double beat) const noexcept
{
    const int i = indexAt (beat);
    return i >= 0 ? &chords[static_cast<size_t> (i)] : nullptr;
}

int Progression::numBars() const noexcept
{
    if (beatsPerBar <= 0)
        return 0;
    return static_cast<int> (std::ceil (totalBeats / beatsPerBar - 1e-9));
}

std::string Progression::toChordNames() const
{
    std::ostringstream out;
    for (size_t i = 0; i < chords.size(); ++i)
    {
        if (i > 0)
            out << ' ';
        out << chords[i].chord.name();
        const double bars = chords[i].length / beatsPerBar;
        if (std::abs (bars - 1.0) > 1e-6)
            out << ':' << bars;
    }
    return out.str();
}

std::string Progression::toRomanNumerals (const KeyContext& key) const
{
    std::string out;
    for (size_t i = 0; i < chords.size(); ++i)
    {
        if (i > 0)
            out += ' ';
        out += romanNumeralFor (chords[i].chord, key);
    }
    return out;
}

Progression Progression::fromChords (const std::vector<Chord>& list, int beatsPerBar, double barsEach)
{
    Progression p;
    p.beatsPerBar = beatsPerBar;
    double t = 0.0;
    for (const auto& c : list)
    {
        ChordEvent e;
        e.chord = c;
        e.start = t;
        e.length = barsEach * beatsPerBar;
        e.token = c.name();
        t += e.length;
        p.chords.push_back (e);
    }
    p.totalBeats = t;
    return p;
}

std::optional<Chord> parseRomanNumeral (std::string_view token, const KeyContext& key)
{
    const auto text = trim (normaliseMusicText (token));
    return parseRomanImpl (text, key, true);
}

std::optional<Chord> parseChordToken (std::string_view tokenIn, const KeyContext& key)
{
    const auto token = trim (normaliseMusicText (tokenIn));
    if (token.empty())
        return std::nullopt;

    size_t p = 0;
    while (p < token.size() && (token[p] == 'b' || token[p] == '#'))
        ++p;

    if (p < token.size() && isRomanLetter (token[p]))
        if (auto r = parseRomanImpl (token, key, true))
            return r;

    return parseChordSymbol (token);
}

ParseResult parseProgression (std::string_view textIn, const KeyContext& key, int beatsPerBar)
{
    ParseResult result;
    auto& prog = result.progression;
    prog.beatsPerBar = beatsPerBar > 0 ? beatsPerBar : 4;
    const double barBeats = prog.beatsPerBar;

    std::string text = normaliseMusicText (textIn);
    for (auto& c : text)
        if (c == '\n' || c == '\r' || c == '\t')
            c = ' ';

    // "->" and the arrow are separators too.
    for (size_t pos; (pos = text.find ("->")) != std::string::npos;)
        text.replace (pos, 2, " ");
    for (size_t pos; (pos = text.find ("\xE2\x86\x92")) != std::string::npos;)
        text.replace (pos, 3, " ");

    const bool barMode = text.find ('|') != std::string::npos;
    double t = 0.0;
    size_t repeatFrom = 0;

    auto addChord = [&] (const Chord& chord, double length, const std::string& tokenText) {
        ChordEvent e;
        e.chord = chord;
        e.start = t;
        e.length = length;
        e.token = tokenText;
        prog.chords.push_back (e);
        t += length;
    };

    auto applyRepeat = [&] (int times) {
        const std::vector<ChordEvent> section (prog.chords.begin() + static_cast<std::ptrdiff_t> (repeatFrom), prog.chords.end());
        for (int r = 1; r < times; ++r)
            for (const auto& e : section)
                addChord (e.chord, e.length, e.token);
        repeatFrom = prog.chords.size();
    };

    if (barMode)
    {
        std::vector<std::string> bars;
        std::string current;
        for (char c : text)
        {
            if (c == '|')
            {
                bars.push_back (current);
                current.clear();
            }
            else
            {
                current += c;
            }
        }
        bars.push_back (current);

        std::vector<ChordEvent> previousBar;

        for (const auto& barText : bars)
        {
            auto words = splitDashes (splitWords (barText), key);
            if (words.empty())
                continue;

            // A bar that is only "%" repeats the previous bar.
            if (words.size() == 1 && words[0] == "%" && ! previousBar.empty())
            {
                for (const auto& e : previousBar)
                    addChord (e.chord, e.length, e.token);
                continue;
            }

            std::vector<Token> tokens;
            int repeatTimes = 0;
            for (auto w : words)
            {
                int times = 0;
                if (isRepeatToken (w, times))
                {
                    repeatTimes = times;
                    continue;
                }

                Token tk;
                tk.explicitWeight = readWeight (w, tk.weight);
                tk.text = w;
                tokens.push_back (tk);
            }

            double totalWeight = 0.0;
            for (const auto& tk : tokens)
                totalWeight += tk.weight;

            const size_t barStartIndex = prog.chords.size();

            for (const auto& tk : tokens)
            {
                const double length = totalWeight > 0.0 ? barBeats * tk.weight / totalWeight : barBeats;

                if (tk.text == "%")
                {
                    if (! prog.chords.empty())
                        addChord (prog.chords.back().chord, length, prog.chords.back().token);
                    continue;
                }

                if (auto chord = parseChordToken (tk.text, key))
                    addChord (*chord, length, tk.text);
                else
                    result.errors.push_back ("Could not read \"" + tk.text + "\"");
            }

            previousBar.assign (prog.chords.begin() + static_cast<std::ptrdiff_t> (barStartIndex), prog.chords.end());

            if (repeatTimes > 0)
                applyRepeat (repeatTimes);
        }
    }
    else
    {
        auto words = splitDashes (splitWords (text), key);

        for (auto w : words)
        {
            int times = 0;
            if (isRepeatToken (w, times))
            {
                applyRepeat (times);
                continue;
            }

            double bars = 1.0;
            readWeight (w, bars);
            const double length = bars * barBeats;

            if (w == "%")
            {
                if (! prog.chords.empty())
                    addChord (prog.chords.back().chord, length, prog.chords.back().token);
                continue;
            }

            if (auto chord = parseChordToken (w, key))
                addChord (*chord, length, w);
            else
                result.errors.push_back ("Could not read \"" + w + "\"");
        }
    }

    prog.totalBeats = t;
    return result;
}

} // namespace melodee
