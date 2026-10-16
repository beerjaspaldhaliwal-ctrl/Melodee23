#include "melodee/MidiFile.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>

namespace melodee
{

namespace
{
    void put16 (std::vector<std::uint8_t>& out, int v)
    {
        out.push_back (static_cast<std::uint8_t> ((v >> 8) & 0xff));
        out.push_back (static_cast<std::uint8_t> (v & 0xff));
    }

    void put32 (std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        for (int shift = 24; shift >= 0; shift -= 8)
            out.push_back (static_cast<std::uint8_t> ((v >> shift) & 0xff));
    }

    void putVarLen (std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        std::uint8_t bytes[5];
        int n = 0;
        bytes[n++] = static_cast<std::uint8_t> (v & 0x7f);
        while ((v >>= 7) != 0)
            bytes[n++] = static_cast<std::uint8_t> ((v & 0x7f) | 0x80);
        while (n > 0)
            out.push_back (bytes[--n]);
    }

    struct Event
    {
        std::uint32_t tick;
        int order;  // note-offs before note-ons at the same tick
        std::vector<std::uint8_t> bytes;
    };

    std::vector<std::uint8_t> trackChunk (std::vector<Event> events)
    {
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b) {
            return a.tick < b.tick || (a.tick == b.tick && a.order < b.order);
        });

        std::vector<std::uint8_t> data;
        std::uint32_t last = 0;
        for (const auto& e : events)
        {
            putVarLen (data, e.tick - last);
            data.insert (data.end(), e.bytes.begin(), e.bytes.end());
            last = e.tick;
        }

        // End of track
        putVarLen (data, 0);
        data.insert (data.end(), { 0xff, 0x2f, 0x00 });

        std::vector<std::uint8_t> chunk { 'M', 'T', 'r', 'k' };
        put32 (chunk, static_cast<std::uint32_t> (data.size()));
        chunk.insert (chunk.end(), data.begin(), data.end());
        return chunk;
    }

    Event metaText (std::uint8_t type, const std::string& text)
    {
        Event e { 0, 0, { 0xff, type } };
        putVarLen (e.bytes, static_cast<std::uint32_t> (text.size()));
        e.bytes.insert (e.bytes.end(), text.begin(), text.end());
        return e;
    }
} // namespace

std::vector<std::uint8_t> writeMidiFile (const std::vector<MidiTrack>& tracks, double bpm, int beatsPerBar, int ppq)
{
    std::vector<std::uint8_t> out { 'M', 'T', 'h', 'd' };
    put32 (out, 6);
    put16 (out, 1);
    put16 (out, static_cast<int> (tracks.size()) + 1);
    put16 (out, ppq);

    // Conductor track: tempo + time signature.
    {
        std::vector<Event> events;
        events.push_back (metaText (0x03, "Melodee"));

        const auto usPerQuarter = static_cast<std::uint32_t> (std::lround (60000000.0 / std::clamp (bpm, 20.0, 400.0)));
        events.push_back ({ 0, 1, { 0xff, 0x51, 0x03,
                                    static_cast<std::uint8_t> ((usPerQuarter >> 16) & 0xff),
                                    static_cast<std::uint8_t> ((usPerQuarter >> 8) & 0xff),
                                    static_cast<std::uint8_t> (usPerQuarter & 0xff) } });
        events.push_back ({ 0, 2, { 0xff, 0x58, 0x04, static_cast<std::uint8_t> (std::clamp (beatsPerBar, 1, 32)), 0x02, 0x18, 0x08 } });

        const auto chunk = trackChunk (events);
        out.insert (out.end(), chunk.begin(), chunk.end());
    }

    for (const auto& track : tracks)
    {
        std::vector<Event> events;
        events.push_back (metaText (0x03, track.name));

        const auto ch = static_cast<std::uint8_t> (std::clamp (track.channel, 0, 15));
        if (track.program >= 0)
            events.push_back ({ 0, 1, { static_cast<std::uint8_t> (0xc0 | ch), static_cast<std::uint8_t> (track.program & 0x7f) } });

        for (const auto& n : track.notes)
        {
            const auto on = static_cast<std::uint32_t> (std::lround (std::max (0.0, n.start) * ppq));
            const auto off = std::max (on + 1, static_cast<std::uint32_t> (std::lround (std::max (0.0, n.start + n.length) * ppq)));
            const auto pitch = static_cast<std::uint8_t> (std::clamp (n.pitch, 0, 127));
            const auto vel = static_cast<std::uint8_t> (std::clamp (n.velocity, 1, 127));

            events.push_back ({ on, 3, { static_cast<std::uint8_t> (0x90 | ch), pitch, vel } });
            events.push_back ({ off, 2, { static_cast<std::uint8_t> (0x80 | ch), pitch, 0 } });
        }

        const auto chunk = trackChunk (events);
        out.insert (out.end(), chunk.begin(), chunk.end());
    }

    return out;
}

bool saveMidiFile (const std::string& path, const std::vector<MidiTrack>& tracks, double bpm, int beatsPerBar)
{
    const auto data = writeMidiFile (tracks, bpm, beatsPerBar);
    std::ofstream file (path, std::ios::binary);
    if (! file)
        return false;
    file.write (reinterpret_cast<const char*> (data.data()), static_cast<std::streamsize> (data.size()));
    return static_cast<bool> (file);
}

std::vector<MidiTrack> readMidiFile (const std::vector<std::uint8_t>& d)
{
    std::vector<MidiTrack> tracks;
    if (d.size() < 14 || d[0] != 'M' || d[1] != 'T' || d[2] != 'h' || d[3] != 'd')
        return tracks;

    auto rd16 = [&] (size_t p) { return (d[p] << 8) | d[p + 1]; };
    auto rd32 = [&] (size_t p) { return (static_cast<std::uint32_t> (d[p]) << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]; };

    const int ntracks = rd16 (10);
    const int ppq = rd16 (12);
    size_t pos = 14;

    for (int t = 0; t < ntracks && pos + 8 <= d.size(); ++t)
    {
        const auto len = rd32 (pos + 4);
        size_t p = pos + 8;
        const size_t end = std::min (d.size(), p + len);
        pos = end;

        MidiTrack track;
        std::uint32_t tick = 0;
        std::uint8_t running = 0;
        std::map<int, std::pair<std::uint32_t, int>> open; // pitch -> (tick, velocity)

        while (p < end)
        {
            std::uint32_t delta = 0;
            while (p < end)
            {
                const auto b = d[p++];
                delta = (delta << 7) | (b & 0x7f);
                if ((b & 0x80) == 0)
                    break;
            }
            tick += delta;
            if (p >= end)
                break;

            std::uint8_t status = d[p];
            if (status == 0xff)
            {
                const auto type = d[p + 1];
                p += 2;
                std::uint32_t mlen = 0;
                while (p < end)
                {
                    const auto b = d[p++];
                    mlen = (mlen << 7) | (b & 0x7f);
                    if ((b & 0x80) == 0)
                        break;
                }
                if (type == 0x03)
                    track.name.assign (d.begin() + static_cast<std::ptrdiff_t> (p), d.begin() + static_cast<std::ptrdiff_t> (p + mlen));
                p += mlen;
                continue;
            }

            if ((status & 0x80) != 0)
            {
                running = status;
                ++p;
            }
            else
            {
                status = running;
            }

            const int type = status & 0xf0;
            track.channel = status & 0x0f;

            if (type == 0xc0 || type == 0xd0)
            {
                if (type == 0xc0)
                    track.program = d[p];
                p += 1;
                continue;
            }

            const int a = d[p];
            const int b = d[p + 1];
            p += 2;

            if (type == 0x90 && b > 0)
            {
                open[a] = { tick, b };
            }
            else if (type == 0x80 || (type == 0x90 && b == 0))
            {
                auto it = open.find (a);
                if (it != open.end())
                {
                    NoteEvent n;
                    n.pitch = a;
                    n.start = it->second.first / static_cast<double> (ppq);
                    n.length = (tick - it->second.first) / static_cast<double> (ppq);
                    n.velocity = it->second.second;
                    track.notes.push_back (n);
                    open.erase (it);
                }
            }
        }

        std::stable_sort (track.notes.begin(), track.notes.end(), [] (const NoteEvent& x, const NoteEvent& y) { return x.start < y.start; });
        tracks.push_back (track);
    }

    return tracks;
}

} // namespace melodee
