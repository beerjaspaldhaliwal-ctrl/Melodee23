#include "ChordStrip.h"

#include "../PluginProcessor.h"
#include "MelodeeLookAndFeel.h"

ChordStrip::ChordStrip (MelodeeProcessor& p) : processor (p)
{
    refresh();
}

void ChordStrip::refresh()
{
    auto& session = processor.getSession();
    progression = session.baseProgression();
    key = session.scale().keyContext();
    voicings = session.chordVoicings();
    repaint();
}

void ChordStrip::setPlayhead (double beat)
{
    int idx = -1;
    if (beat >= 0.0 && progression.totalBeats > 0.0)
        idx = progression.indexAt (std::fmod (beat, progression.totalBeats));

    if (idx != highlighted)
    {
        highlighted = idx;
        repaint();
    }
    playhead = beat;
}

juce::Rectangle<float> ChordStrip::chipBounds (size_t index) const
{
    const auto area = getLocalBounds().toFloat();
    const double total = juce::jmax (1.0, progression.totalBeats);
    const auto& e = progression.chords[index];
    const float x1 = area.getX() + static_cast<float> (e.start / total) * area.getWidth();
    const float x2 = area.getX() + static_cast<float> ((e.start + e.length) / total) * area.getWidth();
    return { x1 + 2.0f, area.getY() + 2.0f, juce::jmax (4.0f, x2 - x1 - 4.0f), area.getHeight() - 4.0f };
}

void ChordStrip::paint (juce::Graphics& g)
{
    if (progression.empty())
    {
        g.setColour (Colours::textDim);
        g.setFont (juce::FontOptions (14.0f));
        g.drawFittedText ("Type chords above or pick a progression from the library", getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    for (size_t i = 0; i < progression.chords.size(); ++i)
    {
        const auto r = chipBounds (i);
        const bool current = static_cast<int> (i) == highlighted;

        g.setColour (current ? Colours::chords.withAlpha (0.85f) : Colours::panelLight);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (current ? Colours::chords : Colours::outline);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);

        const auto& chord = progression.chords[i].chord;
        auto text = r.reduced (6.0f, 4.0f);

        g.setColour (current ? Colours::background : Colours::text);
        g.setFont (juce::FontOptions (juce::jlimit (12.0f, 19.0f, r.getHeight() * 0.36f), juce::Font::bold));
        g.drawFittedText (chord.name(), text.removeFromTop (text.getHeight() * 0.6f).toNearestInt(), juce::Justification::centred, 1, 0.5f);

        g.setColour (current ? Colours::background.withAlpha (0.8f) : Colours::chords);
        g.setFont (juce::FontOptions (juce::jlimit (10.0f, 14.0f, r.getHeight() * 0.25f)));
        g.drawFittedText (melodee::romanNumeralFor (chord, key), text.toNearestInt(), juce::Justification::centred, 1, 0.5f);
    }
}

void ChordStrip::mouseDown (const juce::MouseEvent& e)
{
    for (size_t i = 0; i < progression.chords.size(); ++i)
    {
        if (chipBounds (i).contains (e.position) && i < voicings.size())
        {
            processor.audition (voicings[i], 2, 1.2);
            highlighted = static_cast<int> (i);
            repaint();
            return;
        }
    }
}

//==============================================================================
ScaleChordsBar::ScaleChordsBar (MelodeeProcessor& p) : processor (p)
{
    seventhsToggle.setTooltip ("Show 7th chords instead of triads");
    seventhsToggle.onClick = [this] { processor.getSession().set (IDs::sevenths, seventhsToggle.getToggleState()); };
    addAndMakeVisible (seventhsToggle);
    refresh();
}

void ScaleChordsBar::refresh()
{
    auto& session = processor.getSession();
    const bool sevenths = session.getBool (IDs::sevenths);
    seventhsToggle.setToggleState (sevenths, juce::dontSendNotification);

    const auto scale = session.scale();
    chords = melodee::diatonicChords (scale, sevenths);
    const auto key = scale.keyContext();

    // Use roman numerals if the progression is written in numerals, otherwise chord names.
    const auto text = session.get (IDs::progression).toString().trim();
    const bool numerals = text.isEmpty() || (text.containsAnyOf ("IViv") && ! juce::String ("ABCDEFG").containsChar (text[0]));

    buttons.clear();
    for (const auto& c : chords)
    {
        const auto roman = juce::String (melodee::romanNumeralFor (c, key));
        auto* b = buttons.add (new juce::TextButton (juce::String (c.name()) + "\n" + roman));
        b->setTooltip ("Click to add " + juce::String (c.name()) + " (" + roman + ") to your progression");
        b->setColour (juce::TextButton::textColourOffId, Colours::text);

        const auto token = numerals ? roman : juce::String (c.name());
        const auto voicing = melodee::voiceProgression (melodee::Progression::fromChords ({ c }), melodee::VoicingStyle::Close, 60);
        b->onClick = [this, token, voicing] {
            if (! voicing.empty())
                processor.audition (voicing.front(), 2, 0.9);
            processor.getSession().appendChord (token);
        };
        addAndMakeVisible (b);
    }

    resized();
}

void ScaleChordsBar::resized()
{
    auto r = getLocalBounds();
    seventhsToggle.setBounds (r.removeFromRight (64));
    r.removeFromRight (6);

    if (buttons.isEmpty())
        return;

    const int w = r.getWidth() / buttons.size();
    for (auto* b : buttons)
        b->setBounds (r.removeFromLeft (w).reduced (2, 0));
}
