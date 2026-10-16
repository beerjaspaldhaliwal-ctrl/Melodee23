#include "PianoRoll.h"

#include "../PluginProcessor.h"
#include "MelodeeLookAndFeel.h"

namespace
{
    constexpr int keyboardWidth = 36;
    constexpr int headerHeight = 20;

    bool isBlackKey (int pitch)
    {
        const int pc = pitch % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }
} // namespace

PianoRoll::PianoRoll (MelodeeProcessor& p) : processor (p)
{
    setOpaque (true);
    refresh();
}

void PianoRoll::refresh()
{
    auto& session = processor.getSession();

    const auto& prog = session.progression();
    total = prog.totalBeats > 0.0 ? prog.totalBeats : 16.0;
    beatsPerBar = juce::jmax (1, session.getInt (IDs::beatsPerBar));
    chordEvents = prog.chords;
    flats = session.prefersFlats();

    showMelody = session.getBool (IDs::playMelody);
    showChords = session.getBool (IDs::playChords);
    showBass = session.getBool (IDs::playBass);

    if (const auto* m = session.selectedMelody())
    {
        melody = m->notes;
        melodyName = m->name;
    }
    else
    {
        melody.clear();
        melodyName = {};
    }

    chords = session.chordNotes();
    bass = session.bassNotes();

    int lo = 127, hi = 0;
    auto include = [&] (const std::vector<melodee::NoteEvent>& notes, bool visible) {
        if (! visible)
            return;
        for (const auto& n : notes)
        {
            lo = juce::jmin (lo, n.pitch);
            hi = juce::jmax (hi, n.pitch);
        }
    };
    include (melody, true);
    include (chords, showChords);
    include (bass, showBass);

    if (lo > hi)
    {
        lo = 48;
        hi = 84;
    }

    lo -= 2;
    hi += 2;
    while (hi - lo < 24)
    {
        --lo;
        ++hi;
    }
    lowPitch = juce::jmax (0, lo);
    highPitch = juce::jmin (127, hi);

    repaint();
}

void PianoRoll::setPlayhead (double beat)
{
    if (std::abs (beat - playhead) < 1e-4)
        return;
    playhead = beat;
    repaint();
}

juce::Rectangle<float> PianoRoll::noteArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (static_cast<float> (keyboardWidth)).withTrimmedTop (static_cast<float> (headerHeight));
}

float PianoRoll::xForBeat (double beat) const
{
    const auto area = noteArea();
    return area.getX() + static_cast<float> (beat / total) * area.getWidth();
}

float PianoRoll::yForPitch (int pitch) const
{
    const auto area = noteArea();
    const float rowH = area.getHeight() / static_cast<float> (highPitch - lowPitch + 1);
    return area.getBottom() - static_cast<float> (pitch - lowPitch + 1) * rowH;
}

int PianoRoll::pitchForY (float y) const
{
    const auto area = noteArea();
    const float rowH = area.getHeight() / static_cast<float> (highPitch - lowPitch + 1);
    return lowPitch + static_cast<int> ((area.getBottom() - y) / rowH);
}

void PianoRoll::paint (juce::Graphics& g)
{
    g.fillAll (Colours::panel);

    const auto area = noteArea();
    const float rowH = area.getHeight() / static_cast<float> (highPitch - lowPitch + 1);

    // Rows and keyboard
    for (int p = lowPitch; p <= highPitch; ++p)
    {
        const float y = yForPitch (p);
        if (isBlackKey (p))
        {
            g.setColour (Colours::background.withAlpha (0.55f));
            g.fillRect (area.getX(), y, area.getWidth(), rowH);
        }

        g.setColour (isBlackKey (p) ? juce::Colour (0xff2a2a35) : juce::Colour (0xffcfcfd8));
        g.fillRect (0.0f, y, static_cast<float> (keyboardWidth - (isBlackKey (p) ? 12 : 2)), rowH - 0.5f);

        if (p % 12 == 0 && rowH > 5.0f)
        {
            g.setColour (juce::Colour (0xff3a3a48));
            g.setFont (juce::FontOptions (juce::jmin (10.0f, rowH + 3.0f)));
            g.drawText (melodee::midiNoteName (p, false), 2, static_cast<int> (y), keyboardWidth - 4, static_cast<int> (rowH), juce::Justification::centredRight);
            g.setColour (Colours::outline.withAlpha (0.7f));
            g.drawHorizontalLine (static_cast<int> (y + rowH), area.getX(), area.getRight());
        }
    }

    // Chord regions with names in the header
    for (size_t i = 0; i < chordEvents.size(); ++i)
    {
        const auto& e = chordEvents[i];
        const float x1 = xForBeat (e.start);
        const float x2 = xForBeat (e.start + e.length);

        g.setColour (i % 2 == 0 ? Colours::chords.withAlpha (0.05f) : juce::Colours::transparentBlack);
        g.fillRect (x1, area.getY(), x2 - x1, area.getHeight());

        const bool current = playhead >= e.start && playhead < e.start + e.length;
        g.setColour (current ? Colours::chords.withAlpha (0.35f) : Colours::panelLight);
        g.fillRect (x1 + 1.0f, 1.0f, x2 - x1 - 2.0f, static_cast<float> (headerHeight - 3));
        g.setColour (current ? juce::Colours::white : Colours::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (e.chord.name(), juce::Rectangle<float> (x1 + 3.0f, 0.0f, x2 - x1 - 6.0f, static_cast<float> (headerHeight - 2)).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.6f);
    }

    // Bar and beat lines
    for (double b = 0.0; b <= total + 1e-6; b += 1.0)
    {
        const bool bar = std::fmod (b, static_cast<double> (beatsPerBar)) < 1e-6;
        g.setColour (bar ? Colours::outline.brighter (0.25f) : Colours::outline.withAlpha (0.45f));
        g.drawVerticalLine (static_cast<int> (xForBeat (b)), area.getY(), area.getBottom());
    }

    auto drawNotes = [&] (const std::vector<melodee::NoteEvent>& notes, juce::Colour colour, float alpha, bool outline) {
        for (const auto& n : notes)
        {
            if (n.pitch < lowPitch || n.pitch > highPitch)
                continue;
            const float x = xForBeat (n.start);
            const float w = juce::jmax (2.0f, xForBeat (n.start + n.length) - x - 1.0f);
            const auto r = juce::Rectangle<float> (x, yForPitch (n.pitch) + 0.5f, w, juce::jmax (2.0f, rowH - 1.0f));
            const float velAlpha = alpha * (0.55f + 0.45f * static_cast<float> (n.velocity) / 127.0f);
            g.setColour (colour.withAlpha (velAlpha));
            g.fillRoundedRectangle (r, juce::jmin (3.0f, rowH * 0.4f));
            if (outline)
            {
                g.setColour (colour.brighter (0.6f));
                g.drawRoundedRectangle (r, juce::jmin (3.0f, rowH * 0.4f), 1.0f);
            }
        }
    };

    if (showBass)
        drawNotes (bass, Colours::bass, 0.55f, false);
    if (showChords)
        drawNotes (chords, Colours::chords, 0.38f, false);
    drawNotes (melody, Colours::melody, showMelody ? 0.95f : 0.35f, showMelody);

    // Playhead
    if (playhead >= 0.0)
    {
        const float x = xForBeat (playhead);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.5f);
    }

    if (melody.empty())
    {
        g.setColour (Colours::textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("Press GENERATE to write melody variations for these chords", area.reduced (20.0f).toNearestInt(),
                          juce::Justification::centred, 2);
    }
    else
    {
        g.setColour (Colours::melody.withAlpha (0.9f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (melodyName, area.reduced (6.0f, 4.0f).toNearestInt(), juce::Justification::topRight);
    }

    g.setColour (Colours::outline);
    g.drawRect (getLocalBounds());
}

void PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    if (! noteArea().contains (e.position))
        return;

    const int pitch = pitchForY (e.position.y);
    processor.audition ({ pitch }, 1, 0.35);
}
