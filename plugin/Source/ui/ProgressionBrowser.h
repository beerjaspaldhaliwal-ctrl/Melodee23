#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/** Search box, genre list and progression list for the built-in library. */
class ProgressionBrowser : public juce::Component
{
public:
    ProgressionBrowser();
    ~ProgressionBrowser() override;

    /** Called with an index into melodee::progressionLibrary(). */
    std::function<void (int)> onLoad;

    void selectGenre (const juce::String& genre);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    class GenreModel;
    class ProgressionModel;

    void updateResults();

    juce::Label title;
    juce::TextEditor search;
    juce::ListBox genreList, progressionList;
    std::unique_ptr<GenreModel> genreModel;
    std::unique_ptr<ProgressionModel> progressionModel;
    std::vector<int> results;
    juce::String currentGenre;
};
