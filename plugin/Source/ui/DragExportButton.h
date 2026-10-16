#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MelodeeProcessor;

/** Drag this button onto a DAW track to drop a MIDI file there; click it to save the file instead. */
class DragExportButton : public juce::TextButton
{
public:
    DragExportButton (MelodeeProcessor& processor, int parts, const juce::String& label, juce::Colour colour);

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    void clicked() override;
    juce::File writeTempFile() const;

    MelodeeProcessor& processor;
    int parts;
    juce::Colour colour;
    bool dragStarted = false;
    std::unique_ptr<juce::FileChooser> chooser;
};
