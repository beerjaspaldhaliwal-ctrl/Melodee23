#include "melodee/Theory.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace melodee
{

int maskCount (PitchMask mask) noexcept
{
    int n = 0;
    for (int pc = 0; pc < 12; ++pc)
        n += maskHas (mask, pc) ? 1 : 0;
    return n;
}

PitchMask maskTranspose (PitchMask mask, int semitones) noexcept
{
    PitchMask out = 0;
    for (int pc = 0; pc < 12; ++pc)
        if (maskHas (mask, pc))
            out |= pcBit (pc + semitones);
    return out;
}

int parseNoteName (std::string_view text, size_t& consumed, bool* wasFlat)
{
    consumed = 0;
    if (wasFlat != nullptr)
        *wasFlat = false;

    if (text.empty())
        return -1;

    static constexpr int naturals[7] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    const char letter = static_cast<char> (std::toupper (static_cast<unsigned char> (text[0])));

    if (letter < 'A' || letter > 'G')
        return -1;

    int pc = naturals[letter - 'A'];
    size_t pos = 1;

    // Up to two accidentals directly after the letter: #, b and their Unicode forms.
    for (int accidentals = 0; accidentals < 2 && pos < text.size(); ++accidentals)
    {
        if (text[pos] == '#')
        {
            ++pc;
            ++pos;
        }
        else if (text[pos] == 'b')
        {
            --pc;
            ++pos;
            if (wasFlat != nullptr)
                *wasFlat = true;
        }
        else if (text.substr (pos, 3) == "\xE2\x99\xAF") // ♯
        {
            ++pc;
            pos += 3;
        }
        else if (text.substr (pos, 3) == "\xE2\x99\xAD") // ♭
        {
            --pc;
            pos += 3;
            if (wasFlat != nullptr)
                *wasFlat = true;
        }
        else
        {
            break;
        }
    }

    consumed = pos;
    return mod12 (pc);
}

std::string pitchClassName (int pc, bool preferFlats)
{
    static const char* sharps[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* flats[12]  = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    return preferFlats ? flats[mod12 (pc)] : sharps[mod12 (pc)];
}

std::string midiNoteName (int midiNote, bool preferFlats)
{
    const int octave = static_cast<int> (std::floor (midiNote / 12.0)) - 1;
    return pitchClassName (midiNote, preferFlats) + std::to_string (octave);
}

bool keyUsesFlats (int tonicPc, bool minor) noexcept
{
    const int pc = mod12 (tonicPc);
    if (minor)
        return pc == 2 || pc == 7 || pc == 0 || pc == 5 || pc == 10 || pc == 3;

    return pc == 5 || pc == 10 || pc == 3 || pc == 8 || pc == 1;
}

std::string normaliseMusicText (std::string_view text)
{
    struct Rep { std::string_view from, to; };
    static const Rep reps[] = {
        { "\xC3\xB8" "7", "m7b5" },    // ø7
        { "\xC3\xB8",     "m7b5" },    // ø
        { "\xC3\x98" "7", "m7b5" },    // Ø7
        { "\xC3\x98",     "m7b5" },    // Ø
        { "\xC2\xB0",     "dim" },     // °
        { "\xE2\x99\xAD", "b" },       // ♭
        { "\xE2\x99\xAF", "#" },       // ♯
        { "\xE2\x99\xAE", "" },        // ♮
        { "\xE2\x80\x93", "-" },       // –
        { "\xE2\x80\x94", "-" },       // —
        { "\xE2\x88\x92", "-" },       // − (minus)
        { "\xC3\x97",     "x" },       // ×
        { "\xE2\x80\x99", "'" },
    };

    std::string out;
    out.reserve (text.size());

    size_t i = 0;
    while (i < text.size())
    {
        // A triangle on its own means maj7; followed by a number (Δ9) it means "maj".
        for (std::string_view tri : { std::string_view ("\xCE\x94"), std::string_view ("\xE2\x88\x86"), std::string_view ("^") })
        {
            if (text.substr (i, tri.size()) == tri)
            {
                const size_t after = i + tri.size();
                const bool numberFollows = after < text.size() && std::isdigit (static_cast<unsigned char> (text[after]));
                out += numberFollows ? "maj" : "maj7";
                i = after;
                break;
            }
        }

        if (i >= text.size())
            break;

        bool replaced = false;
        for (const auto& r : reps)
        {
            if (text.substr (i, r.from.size()) == r.from)
            {
                out += r.to;
                i += r.from.size();
                replaced = true;
                break;
            }
        }

        if (! replaced)
            out += text[i++];
    }

    return out;
}

std::string toLower (std::string_view text)
{
    std::string s (text);
    std::transform (s.begin(), s.end(), s.begin(), [] (unsigned char c) { return static_cast<char> (std::tolower (c)); });
    return s;
}

std::string trim (std::string_view text)
{
    size_t start = 0, end = text.size();
    while (start < end && std::isspace (static_cast<unsigned char> (text[start])))
        ++start;
    while (end > start && std::isspace (static_cast<unsigned char> (text[end - 1])))
        --end;
    return std::string (text.substr (start, end - start));
}

bool startsWith (std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && text.substr (0, prefix.size()) == prefix;
}

bool equalsIgnoreCase (std::string_view a, std::string_view b) noexcept
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower (static_cast<unsigned char> (a[i])) != std::tolower (static_cast<unsigned char> (b[i])))
            return false;

    return true;
}

} // namespace melodee
