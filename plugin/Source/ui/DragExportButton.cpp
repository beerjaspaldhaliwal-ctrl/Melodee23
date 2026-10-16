#include "DragExportButton.h"

#include "../PluginProcessor.h"
#include "MelodeeLookAndFeel.h"

DragExportButton::DragExportButton (MelodeeProcessor& p, int partsToExport, const juce::String& label, juce::Colour c)
    : juce::TextButton (label), processor (p), parts (partsToExport), colour (c)
{
    setTooltip ("Drag onto a track in your DAW to drop the MIDI there. Click to save a .mid file.");
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

juce::File DragExportButton::writeTempFile() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Melodee");
    dir.createDirectory();

    auto file = dir.getChildFile (processor.getSession().suggestedFileName (parts));
    const auto data = processor.getSession().createMidi (parts, processor.getBpm());
    file.replaceWithData (data.getData(), data.getSize());
    return file;
}

void DragExportButton::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = false;
    juce::TextButton::mouseDown (e);
}

void DragExportButton::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 6)
        return;

    dragStarted = true;
    const auto file = writeTempFile();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

void DragExportButton::clicked()
{
    if (dragStarted)
    {
        dragStarted = false;
        return;
    }

    const auto name = processor.getSession().suggestedFileName (parts);
    chooser = std::make_unique<juce::FileChooser> ("Save MIDI",
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (name),
                                                   "*.mid");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc) {
                              auto file = fc.getResult();
                              if (file == juce::File())
                                  return;
                              if (! file.hasFileExtension ("mid"))
                                  file = file.withFileExtension ("mid");
                              const auto data = processor.getSession().createMidi (parts, processor.getBpm());
                              file.replaceWithData (data.getData(), data.getSize());
                          });
}

void DragExportButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colour.withAlpha (isButtonDown ? 0.45f : (isMouseOver ? 0.32f : 0.2f)));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (colour.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    // Grip dots
    auto grip = r.removeFromLeft (18.0f).reduced (5.0f, r.getHeight() * 0.28f);
    g.setColour (colour);
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 2; ++col)
            g.fillEllipse (grip.getX() + static_cast<float> (col) * 5.0f, grip.getY() + static_cast<float> (row) * grip.getHeight() / 2.5f, 2.6f, 2.6f);

    g.setColour (Colours::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
}
