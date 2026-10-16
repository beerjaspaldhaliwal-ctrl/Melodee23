#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "ui/ChordStrip.h"
#include "ui/DragExportButton.h"
#include "ui/GeneratorPanel.h"
#include "ui/MelodeeLookAndFeel.h"
#include "ui/PianoRoll.h"
#include "ui/ProgressionBrowser.h"

class MelodeeEditor : public juce::AudioProcessorEditor,
                      private juce::ChangeListener,
                      private juce::Timer
{
public:
    explicit MelodeeEditor (MelodeeProcessor&);
    ~MelodeeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void refreshAll();
    void applyProgressionText();
    void setupCombo (juce::ComboBox& box, const juce::Identifier& id, const std::vector<std::string>& items, const juce::String& prefix);

    MelodeeProcessor& processor;
    MelodeeLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltips;

    // Header
    juce::Label logo, tagline;
    juce::ComboBox keyBox, scaleBox, timeBox, loopsBox;
    juce::Label scaleNotes;
    juce::TextButton playButton { "Play" };
    juce::ToggleButton soundToggle { "Sound" }, syncToggle { "Follow DAW" };
    juce::Slider tempoSlider { juce::Slider::LinearBar, juce::Slider::TextBoxRight };

    // Left
    ProgressionBrowser browser;

    // Centre
    SectionLabel progressionTitle { "Your chord progression  (chord names or roman numerals)" };
    juce::TextEditor progressionEditor;
    juce::TextButton applyButton { "Use chords" };
    juce::Label hintLabel;
    ChordStrip chordStrip;
    SectionLabel scaleChordsTitle { "Chords in this scale  (click to add)" };
    ScaleChordsBar scaleChords;
    PianoRoll pianoRoll;
    juce::ComboBox voicingBox, rhythmBox, bassBox;
    juce::Slider chordRegister { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };

    // Right
    GeneratorPanel generator;

    // Bottom
    juce::ToggleButton playMelodyToggle { "Melody" }, playChordsToggle { "Chords" }, playBassToggle { "Bass" };
    DragExportButton dragMelody, dragChords, dragBass, dragAll;
    juce::ComboBox keyboardModeBox, outputModeBox;
    juce::Label statusLabel;

    bool refreshing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelodeeEditor)
};
