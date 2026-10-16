#include "PluginEditor.h"

namespace
{
    const char* keyNames[12] = { "C", "C# / Db", "D", "D# / Eb", "E", "F", "F# / Gb", "G", "G# / Ab", "A", "A# / Bb", "B" };

    void styleCaption (juce::Label& l, float size, juce::Colour c)
    {
        l.setFont (juce::FontOptions (size));
        l.setColour (juce::Label::textColourId, c);
        l.setMinimumHorizontalScale (0.7f);
    }
} // namespace

MelodeeEditor::MelodeeEditor (MelodeeProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      chordStrip (p),
      scaleChords (p),
      pianoRoll (p),
      generator (p),
      dragMelody (p, Session::MelodyPart, "Melody", Colours::melody),
      dragChords (p, Session::ChordsPart, "Chords", Colours::chords),
      dragBass (p, Session::BassPart, "Bass", Colours::bass),
      dragAll (p, Session::AllParts, "All", Colours::text)
{
    setLookAndFeel (&lookAndFeel);
    tooltips = std::make_unique<juce::TooltipWindow> (this, 500);

    auto& session = processor.getSession();

    // ---- Header ------------------------------------------------------------
    logo.setText ("MELODEE", juce::dontSendNotification);
    logo.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    logo.setColour (juce::Label::textColourId, Colours::melody);
    addAndMakeVisible (logo);

    tagline.setText (juce::String (juce::CharPointer_UTF8 ("scales \xC2\xB7 chords \xC2\xB7 melody variations")), juce::dontSendNotification);
    styleCaption (tagline, 11.5f, Colours::textDim);
    addAndMakeVisible (tagline);

    for (int i = 0; i < 12; ++i)
        keyBox.addItem (juce::String ("Key: ") + keyNames[i], i + 1);
    keyBox.setTooltip ("The key (home note). Changing it transposes everything.");
    keyBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::key, keyBox.getSelectedItemIndex());
    };
    addAndMakeVisible (keyBox);

    // Scales grouped into sub-menus by category.
    {
        const auto& scales = melodee::allScales();
        for (const auto& category : melodee::scaleCategories())
        {
            juce::PopupMenu sub;
            for (size_t i = 0; i < scales.size(); ++i)
                if (scales[i].category == category)
                    sub.addItem (static_cast<int> (i) + 1, scales[i].name);
            scaleBox.getRootMenu()->addSubMenu (category, sub);
        }
    }
    scaleBox.setTooltip ("Every scale and mode, grouped by family");
    scaleBox.onChange = [this] {
        if (! refreshing && scaleBox.getSelectedId() > 0)
            processor.getSession().set (IDs::scale, scaleBox.getSelectedId() - 1);
    };
    addAndMakeVisible (scaleBox);

    styleCaption (scaleNotes, 12.5f, Colours::chords);
    addAndMakeVisible (scaleNotes);

    for (int beats : { 2, 3, 4, 5, 6, 7 })
        timeBox.addItem (juce::String (beats) + "/4", beats);
    timeBox.setTooltip ("Beats per bar");
    timeBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::beatsPerBar, timeBox.getSelectedId());
    };
    addAndMakeVisible (timeBox);

    for (int loops : { 1, 2, 4 })
        loopsBox.addItem ("Length " + juce::String (loops) + "x", loops);
    loopsBox.setTooltip ("Write melodies over the progression played 1, 2 or 4 times (longer, developing phrases)");
    loopsBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::loops, loopsBox.getSelectedId());
    };
    addAndMakeVisible (loopsBox);

    playButton.setColour (juce::TextButton::buttonColourId, Colours::chords.withAlpha (0.8f));
    playButton.setColour (juce::TextButton::textColourOffId, Colours::background);
    playButton.setTooltip ("Preview the loop (when the DAW is stopped)");
    playButton.onClick = [this] { processor.setPreviewing (! processor.isPreviewing()); };
    addAndMakeVisible (playButton);

    soundToggle.setTooltip ("Play the built-in sound. Turn off to only send MIDI to another instrument.");
    soundToggle.onClick = [this] { processor.getSession().set (IDs::soundOn, soundToggle.getToggleState()); };
    addAndMakeVisible (soundToggle);

    syncToggle.setTooltip ("Play along when the DAW transport is running");
    syncToggle.onClick = [this] { processor.getSession().set (IDs::syncToHost, syncToggle.getToggleState()); };
    addAndMakeVisible (syncToggle);

    tempoSlider.setRange (50.0, 200.0, 1.0);
    tempoSlider.setTextValueSuffix (" bpm");
    tempoSlider.setTooltip ("Preview tempo (the DAW's tempo is used when it is available)");
    tempoSlider.setColour (juce::Slider::trackColourId, Colours::chords.withAlpha (0.3f));
    tempoSlider.onValueChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::previewBpm, tempoSlider.getValue());
    };
    addAndMakeVisible (tempoSlider);

    // ---- Library -------------------------------------------------------------
    browser.onLoad = [this] (int index) { processor.getSession().loadLibraryProgression (index); };
    addAndMakeVisible (browser);

    // ---- Progression ----------------------------------------------------------
    addAndMakeVisible (progressionTitle);
    progressionEditor.setFont (juce::FontOptions (18.0f));
    progressionEditor.setTextToShowWhenEmpty ("e.g.  Am F C G   |   vi IV I V   |   ii7 V7 Imaj7   |   | Dm7 G7 | Cmaj7 |", Colours::textDim);
    progressionEditor.setIndents (10, 7);
    progressionEditor.onReturnKey = [this] { applyProgressionText(); };
    progressionEditor.onFocusLost = [this] { applyProgressionText(); };
    progressionEditor.setTooltip ("Type chords (Am7 D9 Gmaj7 F#m7b5 C/E ...) or roman numerals (ii V I, bVII, V/V, I/3).\n"
                                  "Use | to mark bars, :2 for two bars, % to repeat, x2 to repeat everything. Press Enter.");
    addAndMakeVisible (progressionEditor);

    applyButton.setColour (juce::TextButton::buttonColourId, Colours::chords.withAlpha (0.25f));
    applyButton.onClick = [this] { applyProgressionText(); };
    addAndMakeVisible (applyButton);

    styleCaption (hintLabel, 12.0f, Colours::textDim);
    addAndMakeVisible (hintLabel);

    addAndMakeVisible (chordStrip);
    addAndMakeVisible (scaleChordsTitle);
    addAndMakeVisible (scaleChords);
    addAndMakeVisible (pianoRoll);

    setupCombo (voicingBox, IDs::voicing, melodee::voicingNames(), "Voicing: ");
    voicingBox.setTooltip ("How the chord notes are spread out");
    setupCombo (rhythmBox, IDs::chordRhythm, melodee::chordRhythmNames(), "Chord rhythm: ");
    rhythmBox.setTooltip ("Rhythm pattern for the chords");
    setupCombo (bassBox, IDs::bass, melodee::bassStyleNames(), "Bass: ");
    bassBox.setTooltip ("Bass line style");

    chordRegister.setRange (48.0, 72.0, 1.0);
    chordRegister.setTextBoxStyle (juce::Slider::TextBoxLeft, true, 100, 20);
    chordRegister.textFromValueFunction = [] (double v) { return "Chords " + juce::String (melodee::midiNoteName (static_cast<int> (v), false)); };
    chordRegister.setTooltip ("Chord register: lower or higher voicings");
    chordRegister.setColour (juce::Slider::trackColourId, Colours::chords.withAlpha (0.35f));
    chordRegister.onValueChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::chordCenter, static_cast<int> (chordRegister.getValue()));
    };
    addAndMakeVisible (chordRegister);

    // ---- Generator ------------------------------------------------------------
    addAndMakeVisible (generator);

    // ---- Bottom bar -----------------------------------------------------------
    auto bindToggle = [this] (juce::ToggleButton& t, const juce::Identifier& id, juce::Colour c) {
        t.setColour (juce::ToggleButton::tickColourId, c);
        t.setTooltip ("Play and send this part");
        t.onClick = [this, &t, id] { processor.getSession().set (id, t.getToggleState()); };
        addAndMakeVisible (t);
    };
    bindToggle (playMelodyToggle, IDs::playMelody, Colours::melody);
    bindToggle (playChordsToggle, IDs::playChords, Colours::chords);
    bindToggle (playBassToggle, IDs::playBass, Colours::bass);

    for (auto* d : { &dragMelody, &dragChords, &dragBass, &dragAll })
        addAndMakeVisible (d);

    keyboardModeBox.addItem ("Keys: Normal", 1);
    keyboardModeBox.addItem ("Keys: Lock to scale", 2);
    keyboardModeBox.addItem ("Keys: Play chords (white keys)", 3);
    keyboardModeBox.setTooltip ("What your MIDI keyboard does: play normally, snap to the scale, or trigger the progression's chords");
    keyboardModeBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::keyboardMode, keyboardModeBox.getSelectedItemIndex());
    };
    addAndMakeVisible (keyboardModeBox);

    outputModeBox.addItem ("MIDI out: ch 1/2/3", 1);
    outputModeBox.addItem ("MIDI out: all on ch 1", 2);
    outputModeBox.setTooltip ("Melody on channel 1, chords on 2, bass on 3 - or everything on channel 1");
    outputModeBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::outputMode, outputModeBox.getSelectedItemIndex());
    };
    addAndMakeVisible (outputModeBox);

    styleCaption (statusLabel, 12.5f, Colours::melody.withAlpha (0.9f));
    addAndMakeVisible (statusLabel);

    session.addChangeListener (this);
    browser.selectGenre (session.get (IDs::genre).toString());

    setResizable (true, true);
    setResizeLimits (1120, 720, 2400, 1600);
    setSize (1240, 800);

    refreshAll();
    startTimerHz (30);
}

MelodeeEditor::~MelodeeEditor()
{
    processor.getSession().removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void MelodeeEditor::setupCombo (juce::ComboBox& box, const juce::Identifier& id, const std::vector<std::string>& items, const juce::String& prefix)
{
    for (size_t i = 0; i < items.size(); ++i)
        box.addItem (prefix + juce::String (items[i]), static_cast<int> (i) + 1);
    box.onChange = [this, &box, id] {
        if (! refreshing)
            processor.getSession().set (id, box.getSelectedItemIndex());
    };
    addAndMakeVisible (box);
}

void MelodeeEditor::applyProgressionText()
{
    processor.getSession().set (IDs::progression, progressionEditor.getText().trim());
}

//==============================================================================
void MelodeeEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshAll();
}

void MelodeeEditor::refreshAll()
{
    const juce::ScopedValueSetter<bool> svs (refreshing, true);
    auto& s = processor.getSession();

    keyBox.setSelectedItemIndex (s.getInt (IDs::key), juce::dontSendNotification);
    scaleBox.setSelectedId (s.getInt (IDs::scale) + 1, juce::dontSendNotification);
    timeBox.setSelectedId (s.getInt (IDs::beatsPerBar), juce::dontSendNotification);
    loopsBox.setSelectedId (s.getInt (IDs::loops), juce::dontSendNotification);

    const auto scale = s.scale();
    scaleNotes.setText (juce::String (scale.noteNames (s.prefersFlats())), juce::dontSendNotification);
    scaleNotes.setTooltip (juce::String (scale.def().description) + (scale.def().aliases.empty() ? juce::String() : "  Also called: " + juce::String (scale.def().aliases)));

    soundToggle.setToggleState (s.getBool (IDs::soundOn), juce::dontSendNotification);
    syncToggle.setToggleState (s.getBool (IDs::syncToHost), juce::dontSendNotification);
    tempoSlider.setValue (s.getDouble (IDs::previewBpm), juce::dontSendNotification);

    if (! progressionEditor.hasKeyboardFocus (true))
        progressionEditor.setText (s.get (IDs::progression).toString(), false);

    const auto& parsed = s.parsed();
    if (! parsed.errors.empty())
    {
        hintLabel.setColour (juce::Label::textColourId, Colours::warning);
        hintLabel.setText (juce::String (parsed.errors.front()) + "  - check the spelling (e.g. Am7, F#m, Bbmaj7, vi, bVII)", juce::dontSendNotification);
    }
    else
    {
        hintLabel.setColour (juce::Label::textColourId, Colours::textDim);
        const auto& prog = parsed.progression;
        hintLabel.setText (prog.empty() ? juce::String ("No chords yet")
                                        : juce::String (prog.toChordNames()) + "     " + juce::String (prog.toRomanNumerals (scale.keyContext()))
                                              + "     (" + juce::String (prog.numBars()) + " bars)",
                           juce::dontSendNotification);
    }

    voicingBox.setSelectedItemIndex (s.getInt (IDs::voicing), juce::dontSendNotification);
    rhythmBox.setSelectedItemIndex (s.getInt (IDs::chordRhythm), juce::dontSendNotification);
    bassBox.setSelectedItemIndex (s.getInt (IDs::bass), juce::dontSendNotification);
    chordRegister.setValue (s.getInt (IDs::chordCenter), juce::dontSendNotification);

    playMelodyToggle.setToggleState (s.getBool (IDs::playMelody), juce::dontSendNotification);
    playChordsToggle.setToggleState (s.getBool (IDs::playChords), juce::dontSendNotification);
    playBassToggle.setToggleState (s.getBool (IDs::playBass), juce::dontSendNotification);
    keyboardModeBox.setSelectedItemIndex (s.getInt (IDs::keyboardMode), juce::dontSendNotification);
    outputModeBox.setSelectedItemIndex (s.getInt (IDs::outputMode), juce::dontSendNotification);
    statusLabel.setText (s.getStatus(), juce::dontSendNotification);

    dragMelody.setEnabled (s.selectedMelody() != nullptr);

    chordStrip.refresh();
    scaleChords.refresh();
    pianoRoll.refresh();
    generator.refresh();
}

void MelodeeEditor::timerCallback()
{
    const auto beat = processor.getSequencer().getCurrentBeat();
    pianoRoll.setPlayhead (beat);
    chordStrip.setPlayhead (beat);

    const bool previewing = processor.isPreviewing();
    playButton.setButtonText (previewing ? "Stop" : "Play");
}

//==============================================================================
void MelodeeEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    g.setColour (Colours::panel);
    g.fillRect (getLocalBounds().removeFromTop (54));
    g.fillRect (getLocalBounds().removeFromBottom (52));
    g.setColour (Colours::outline);
    g.drawHorizontalLine (54, 0.0f, static_cast<float> (getWidth()));
    g.drawHorizontalLine (getHeight() - 52, 0.0f, static_cast<float> (getWidth()));

    g.setColour (Colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("DRAG TO DAW", dragMelody.getX() - 84, getHeight() - 52, 80, 52, juce::Justification::centredRight);
}

void MelodeeEditor::resized()
{
    auto r = getLocalBounds();

    // Header
    {
        auto h = r.removeFromTop (54).reduced (12, 10);
        auto brand = h.removeFromLeft (172);
        logo.setBounds (brand.removeFromTop (22));
        tagline.setBounds (brand);

        keyBox.setBounds (h.removeFromLeft (116));
        h.removeFromLeft (6);
        scaleBox.setBounds (h.removeFromLeft (200));
        h.removeFromLeft (6);
        timeBox.setBounds (h.removeFromLeft (66));
        h.removeFromLeft (6);
        loopsBox.setBounds (h.removeFromLeft (108));

        playButton.setBounds (h.removeFromRight (76));
        h.removeFromRight (8);
        tempoSlider.setBounds (h.removeFromRight (96).reduced (0, 3));
        h.removeFromRight (8);
        syncToggle.setBounds (h.removeFromRight (108));
        soundToggle.setBounds (h.removeFromRight (74));

        h.removeFromLeft (10);
        scaleNotes.setBounds (h);
    }

    // Bottom bar
    {
        auto b = r.removeFromBottom (52).reduced (12, 11);
        playMelodyToggle.setBounds (b.removeFromLeft (84));
        playChordsToggle.setBounds (b.removeFromLeft (84));
        playBassToggle.setBounds (b.removeFromLeft (70));
        b.removeFromLeft (12);
        keyboardModeBox.setBounds (b.removeFromLeft (220));
        b.removeFromLeft (6);
        outputModeBox.setBounds (b.removeFromLeft (170));

        auto drags = b.removeFromRight (380);
        const int w = drags.getWidth() / 4;
        for (auto* d : { &dragMelody, &dragChords, &dragBass, &dragAll })
            d->setBounds (drags.removeFromLeft (w).reduced (3, 0));
    }

    r.reduce (10, 10);

    browser.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (10);
    generator.setBounds (r.removeFromRight (330));
    r.removeFromRight (10);

    // Centre column
    progressionTitle.setBounds (r.removeFromTop (16));
    r.removeFromTop (4);
    auto editRow = r.removeFromTop (38);
    applyButton.setBounds (editRow.removeFromRight (100));
    editRow.removeFromRight (6);
    progressionEditor.setBounds (editRow);
    hintLabel.setBounds (r.removeFromTop (22));
    r.removeFromTop (2);
    chordStrip.setBounds (r.removeFromTop (58));
    r.removeFromTop (8);
    scaleChordsTitle.setBounds (r.removeFromTop (16));
    r.removeFromTop (3);
    scaleChords.setBounds (r.removeFromTop (40));
    r.removeFromTop (10);

    statusLabel.setBounds (r.removeFromBottom (18));
    r.removeFromBottom (4);
    auto accRow = r.removeFromBottom (28);
    const int third = (accRow.getWidth() - 100) / 3;
    voicingBox.setBounds (accRow.removeFromLeft (third - 4));
    accRow.removeFromLeft (6);
    rhythmBox.setBounds (accRow.removeFromLeft (third - 4));
    accRow.removeFromLeft (6);
    bassBox.setBounds (accRow.removeFromLeft (third - 4));
    accRow.removeFromLeft (6);
    chordRegister.setBounds (accRow.reduced (0, 4));
    r.removeFromBottom (8);

    pianoRoll.setBounds (r);
}
