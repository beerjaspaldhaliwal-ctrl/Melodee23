#include "GeneratorPanel.h"

#include "../PluginProcessor.h"

class GeneratorPanel::VariationModel : public juce::ListBoxModel
{
public:
    explicit VariationModel (MelodeeProcessor& p) : processor (p) {}

    int getNumRows() override { return static_cast<int> (processor.getSession().variations().size()); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        const auto& vars = processor.getSession().variations();
        if (row < 0 || row >= static_cast<int> (vars.size()))
            return;

        const auto& m = vars[static_cast<size_t> (row)];

        if (selected)
        {
            g.setColour (Colours::melody.withAlpha (0.3f));
            g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 1.0f, static_cast<float> (width) - 4.0f, static_cast<float> (height) - 2.0f), 5.0f);
        }

        g.setColour (selected ? juce::Colours::white : Colours::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (m.name, 8, 2, width - 50, height / 2, juce::Justification::centredLeft, true);

        g.setColour (Colours::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String (m.notes.size()) + " notes", width - 60, 2, 54, height / 2, juce::Justification::centredRight);
        g.drawText (juce::String (m.description).upToFirstOccurrenceOf (juce::String (juce::CharPointer_UTF8 (" \xC2\xB7 seed")), false, false),
                    8, height / 2, width - 14, height / 2 - 2, juce::Justification::centredLeft, true);
    }

    void selectedRowsChanged (int row) override
    {
        if (row >= 0 && row != processor.getSession().selectedIndex())
            processor.getSession().select (row);
    }

    void deleteKeyPressed (int) override { processor.getSession().removeSelected(); }

private:
    MelodeeProcessor& processor;
};

//==============================================================================
GeneratorPanel::GeneratorPanel (MelodeeProcessor& p)
    : processor (p), variationModel (std::make_unique<VariationModel> (p))
{
    auto& session = processor.getSession();

    addAndMakeVisible (styleTitle);

    for (int i = 0; i < melodee::numMelodyStyles(); ++i)
        styleBox.addItem (melodee::melodyStyleNames()[static_cast<size_t> (i)], i + 1);
    styleBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::style, styleBox.getSelectedItemIndex());
    };
    addAndMakeVisible (styleBox);

    styleDescription.setFont (juce::FontOptions (12.0f));
    styleDescription.setColour (juce::Label::textColourId, Colours::textDim);
    styleDescription.setJustificationType (juce::Justification::topLeft);
    styleDescription.setMinimumHorizontalScale (0.8f);
    addAndMakeVisible (styleDescription);

    bindKnob (density, IDs::density);
    bindKnob (syncopation, IDs::syncopation);
    bindKnob (leaps, IDs::leaps);
    bindKnob (chordTones, IDs::chordTones);
    bindKnob (repetition, IDs::repetition);
    bindKnob (rests, IDs::rests);
    bindKnob (swing, IDs::swing);
    bindKnob (noteLength, IDs::noteLength);
    bindKnob (humanize, IDs::humanize);
    humanize.slider.getProperties().set ("bipolar", false);
    humanize.slider.setDoubleClickReturnValue (true, 0.15);

    for (size_t i = 0; i < melodee::contourNames().size(); ++i)
        contourBox.addItem ("Contour: " + juce::String (melodee::contourNames()[i]), static_cast<int> (i) + 1);
    contourBox.setTooltip ("The shape of each phrase");
    contourBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::contour, contourBox.getSelectedItemIndex());
    };
    addAndMakeVisible (contourBox);

    for (size_t i = 0; i < melodee::phraseFormNames().size(); ++i)
        formBox.addItem ("Form: " + juce::String (melodee::phraseFormNames()[i]), static_cast<int> (i) + 1);
    formBox.setTooltip ("Which bars repeat the motif (A) and which bring new ideas (B, C)");
    formBox.onChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::form, formBox.getSelectedItemIndex());
    };
    addAndMakeVisible (formBox);

    rangeLabel.setFont (juce::FontOptions (12.0f));
    rangeLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (rangeLabel);

    rangeSlider.setRange (36.0, 96.0, 1.0);
    rangeSlider.setTooltip ("Lowest and highest melody note");
    rangeSlider.onValueChange = [this] {
        updateRangeLabel();
        if (refreshing)
            return;
        auto& s = processor.getSession();
        s.set (IDs::lowNote, static_cast<int> (rangeSlider.getMinValue()));
        s.set (IDs::highNote, static_cast<int> (rangeSlider.getMaxValue()));
    };
    addAndMakeVisible (rangeSlider);

    countLabel.setText ("Variations", juce::dontSendNotification);
    countLabel.setFont (juce::FontOptions (12.0f));
    countLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (countLabel);
    countSlider.setRange (1.0, 32.0, 1.0);
    countSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 34, 22);
    countSlider.onValueChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::variationCount, static_cast<int> (countSlider.getValue()));
    };
    addAndMakeVisible (countSlider);

    seedLabel.setText ("Seed", juce::dontSendNotification);
    seedLabel.setFont (juce::FontOptions (12.0f));
    seedLabel.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (seedLabel);
    seedSlider.setRange (1.0, 999999.0, 1.0);
    seedSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 60, 22);
    seedSlider.setTooltip ("The same seed and settings always give the same melodies");
    seedSlider.onValueChange = [this] {
        if (! refreshing)
            processor.getSession().set (IDs::seed, static_cast<int> (seedSlider.getValue()));
    };
    addAndMakeVisible (seedSlider);

    diceButton.setTooltip ("Random seed, then generate");
    diceButton.onClick = [this] {
        processor.getSession().randomiseSeed();
        processor.getSession().generate (false);
    };
    addAndMakeVisible (diceButton);

    tonicToggle.setTooltip ("Resolve the last note to the key's home note");
    tonicToggle.onClick = [this] { processor.getSession().set (IDs::endOnTonic, tonicToggle.getToggleState()); };
    addAndMakeVisible (tonicToggle);

    generateButton.setColour (juce::TextButton::buttonColourId, Colours::melody);
    generateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.setTooltip ("Write a fresh set of melody variations for your chords");
    generateButton.onClick = [this] { processor.getSession().generate (false); };
    addAndMakeVisible (generateButton);

    moreButton.setTooltip ("Add more variations to the list (keeps the ones you have)");
    moreButton.onClick = [this] { processor.getSession().generate (true); };
    addAndMakeVisible (moreButton);

    addAndMakeVisible (variationsTitle);
    variationList.setModel (variationModel.get());
    variationList.setRowHeight (36);
    variationList.setColour (juce::ListBox::backgroundColourId, Colours::background);
    addAndMakeVisible (variationList);

    for (int i = 0; i < melodee::numTransforms(); ++i)
        transformBox.addItem (melodee::transformNames()[static_cast<size_t> (i)], i + 1);
    transformBox.onChange = [this] {
        if (refreshing)
            return;
        const int t = transformBox.getSelectedItemIndex();
        processor.getSession().set (IDs::transform, t);
        transformBox.setTooltip (melodee::transformDescription (static_cast<melodee::MelodyTransform> (juce::jmax (0, t))));
    };
    addAndMakeVisible (transformBox);

    varyButton.setColour (juce::TextButton::buttonColourId, Colours::melody.withAlpha (0.6f));
    varyButton.setTooltip ("Make a new variation of the selected melody");
    varyButton.onClick = [this] { processor.getSession().applyTransform (transformBox.getSelectedItemIndex()); };
    addAndMakeVisible (varyButton);

    deleteButton.onClick = [this] { processor.getSession().removeSelected(); };
    addAndMakeVisible (deleteButton);
    clearButton.onClick = [this] { processor.getSession().clearVariations(); };
    addAndMakeVisible (clearButton);

    juce::ignoreUnused (session);
    refresh();
}

GeneratorPanel::~GeneratorPanel()
{
    variationList.setModel (nullptr);
}

void GeneratorPanel::bindKnob (Knob& knob, const juce::Identifier& id)
{
    knob.slider.getProperties().set ("bipolar", true);
    knob.slider.onValueChange = [this, &knob, id] {
        if (! refreshing)
            processor.getSession().set (id, knob.slider.getValue());
    };
    addAndMakeVisible (knob);
}

void GeneratorPanel::updateRangeLabel()
{
    rangeLabel.setText ("Range  " + juce::String (melodee::midiNoteName (static_cast<int> (rangeSlider.getMinValue()), false))
                            + " - " + juce::String (melodee::midiNoteName (static_cast<int> (rangeSlider.getMaxValue()), false)),
                        juce::dontSendNotification);
}

void GeneratorPanel::refresh()
{
    const juce::ScopedValueSetter<bool> svs (refreshing, true);
    auto& s = processor.getSession();

    styleBox.setSelectedItemIndex (s.getInt (IDs::style), juce::dontSendNotification);
    styleDescription.setText (melodee::melodyStyleDescription (s.style()), juce::dontSendNotification);

    density.slider.setValue (s.getDouble (IDs::density), juce::dontSendNotification);
    syncopation.slider.setValue (s.getDouble (IDs::syncopation), juce::dontSendNotification);
    leaps.slider.setValue (s.getDouble (IDs::leaps), juce::dontSendNotification);
    chordTones.slider.setValue (s.getDouble (IDs::chordTones), juce::dontSendNotification);
    repetition.slider.setValue (s.getDouble (IDs::repetition), juce::dontSendNotification);
    rests.slider.setValue (s.getDouble (IDs::rests), juce::dontSendNotification);
    swing.slider.setValue (s.getDouble (IDs::swing), juce::dontSendNotification);
    noteLength.slider.setValue (s.getDouble (IDs::noteLength), juce::dontSendNotification);
    humanize.slider.setValue (s.getDouble (IDs::humanize), juce::dontSendNotification);

    contourBox.setSelectedItemIndex (s.getInt (IDs::contour), juce::dontSendNotification);
    formBox.setSelectedItemIndex (s.getInt (IDs::form), juce::dontSendNotification);
    rangeSlider.setMinAndMaxValues (s.getInt (IDs::lowNote), s.getInt (IDs::highNote), juce::dontSendNotification);
    updateRangeLabel();
    countSlider.setValue (s.getInt (IDs::variationCount), juce::dontSendNotification);
    seedSlider.setValue (s.getInt (IDs::seed), juce::dontSendNotification);
    tonicToggle.setToggleState (s.getBool (IDs::endOnTonic), juce::dontSendNotification);
    transformBox.setSelectedItemIndex (s.getInt (IDs::transform), juce::dontSendNotification);
    transformBox.setTooltip (melodee::transformDescription (static_cast<melodee::MelodyTransform> (juce::jmax (0, s.getInt (IDs::transform)))));

    variationList.updateContent();
    const int sel = s.selectedIndex();
    if (sel >= 0)
    {
        variationList.selectRow (sel, true, true);
    }
    else
    {
        variationList.deselectAllRows();
    }
    variationList.repaint();

    const bool hasSelection = sel >= 0;
    varyButton.setEnabled (hasSelection);
    deleteButton.setEnabled (hasSelection);
    clearButton.setEnabled (hasSelection);
}

void GeneratorPanel::paint (juce::Graphics& g)
{
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
}

void GeneratorPanel::resized()
{
    auto r = getLocalBounds().reduced (10);

    styleTitle.setBounds (r.removeFromTop (16));
    r.removeFromTop (3);
    styleBox.setBounds (r.removeFromTop (28));
    r.removeFromTop (2);
    styleDescription.setBounds (r.removeFromTop (32));
    r.removeFromTop (2);

    const int knobH = 62;
    auto row1 = r.removeFromTop (knobH);
    auto row2 = r.removeFromTop (knobH);
    const int kw = row1.getWidth() / 4;
    for (auto* k : { &density, &syncopation, &leaps, &chordTones })
        k->setBounds (row1.removeFromLeft (kw));
    for (auto* k : { &repetition, &rests, &swing, &noteLength })
        k->setBounds (row2.removeFromLeft (kw));

    r.removeFromTop (6);
    auto shapeRow = r.removeFromTop (26);
    contourBox.setBounds (shapeRow.removeFromLeft (shapeRow.getWidth() / 2 - 3));
    shapeRow.removeFromLeft (6);
    formBox.setBounds (shapeRow);

    r.removeFromTop (6);
    auto rangeRow = r.removeFromTop (22);
    rangeLabel.setBounds (rangeRow.removeFromLeft (118));
    rangeSlider.setBounds (rangeRow);

    r.removeFromTop (4);
    auto humanRow = r.removeFromTop (48);
    humanize.setBounds (humanRow.removeFromLeft (64));
    humanRow.removeFromLeft (6);
    auto countRow = humanRow.removeFromTop (24);
    countLabel.setBounds (countRow.removeFromLeft (64));
    countSlider.setBounds (countRow.removeFromLeft (100));
    tonicToggle.setBounds (countRow);
    auto seedRow = humanRow;
    seedLabel.setBounds (seedRow.removeFromLeft (64));
    seedSlider.setBounds (seedRow.removeFromLeft (126));
    seedRow.removeFromLeft (4);
    diceButton.setBounds (seedRow.reduced (0, 1));

    r.removeFromTop (8);
    auto genRow = r.removeFromTop (38);
    moreButton.setBounds (genRow.removeFromRight (74));
    genRow.removeFromRight (6);
    generateButton.setBounds (genRow);

    r.removeFromTop (8);
    variationsTitle.setBounds (r.removeFromTop (16));
    r.removeFromTop (3);

    auto bottom = r.removeFromBottom (28);
    r.removeFromBottom (6);
    auto varyRow = r.removeFromBottom (28);
    varyButton.setBounds (varyRow.removeFromRight (60));
    varyRow.removeFromRight (6);
    transformBox.setBounds (varyRow);
    r.removeFromBottom (6);

    variationList.setBounds (r);

    deleteButton.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 - 3));
    bottom.removeFromLeft (6);
    clearButton.setBounds (bottom);
}
