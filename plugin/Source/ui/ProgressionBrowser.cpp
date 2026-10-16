#include "ProgressionBrowser.h"

#include "MelodeeLookAndFeel.h"
#include "melodee/ProgressionLibrary.h"

class ProgressionBrowser::GenreModel : public juce::ListBoxModel
{
public:
    explicit GenreModel (ProgressionBrowser& b) : browser (b) {}

    int getNumRows() override { return static_cast<int> (melodee::progressionGenres().size()) + 1; }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (selected)
        {
            g.setColour (Colours::melody.withAlpha (0.3f));
            g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 1.0f, static_cast<float> (width) - 4.0f, static_cast<float> (height) - 2.0f), 4.0f);
        }

        const auto name = row == 0 ? juce::String ("All genres") : juce::String (melodee::progressionGenres()[static_cast<size_t> (row - 1)]);
        const auto count = row == 0 ? melodee::progressionLibrary().size() : melodee::progressionsInGenre (name.toStdString()).size();

        g.setColour (selected ? juce::Colours::white : Colours::text);
        g.setFont (juce::FontOptions (13.5f));
        g.drawText (name, 8, 0, width - 40, height, juce::Justification::centredLeft, true);
        g.setColour (Colours::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (juce::String (count), width - 34, 0, 28, height, juce::Justification::centredRight);
    }

    void selectedRowsChanged (int row) override
    {
        browser.currentGenre = row <= 0 ? juce::String() : juce::String (melodee::progressionGenres()[static_cast<size_t> (row - 1)]);
        browser.updateResults();
    }

private:
    ProgressionBrowser& browser;
};

class ProgressionBrowser::ProgressionModel : public juce::ListBoxModel
{
public:
    explicit ProgressionModel (ProgressionBrowser& b) : browser (b) {}

    int getNumRows() override { return static_cast<int> (browser.results.size()); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (row < 0 || row >= getNumRows())
            return;

        const auto& p = melodee::progressionLibrary()[static_cast<size_t> (browser.results[static_cast<size_t> (row)])];

        if (selected)
        {
            g.setColour (Colours::chords.withAlpha (0.25f));
            g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 1.0f, static_cast<float> (width) - 4.0f, static_cast<float> (height) - 2.0f), 4.0f);
        }

        g.setColour (selected ? juce::Colours::white : Colours::text);
        g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
        g.drawText (p.name, 8, 2, width - 12, height / 2, juce::Justification::centredLeft, true);

        g.setColour (Colours::chords);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (p.numerals, 8, height / 2, width - 12, height / 2 - 2, juce::Justification::centredLeft, true);

        if (browser.currentGenre.isEmpty())
        {
            g.setColour (Colours::textDim);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText (p.genre, 8, 2, width - 14, height / 2, juce::Justification::centredRight, true);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override { load (row); }
    void returnKeyPressed (int row) override { load (row); }

    juce::String getTooltipForRow (int row) override
    {
        if (row < 0 || row >= getNumRows())
            return {};
        const auto& p = melodee::progressionLibrary()[static_cast<size_t> (browser.results[static_cast<size_t> (row)])];
        return juce::String (p.description) + "  (" + p.scale + ")";
    }

private:
    void load (int row)
    {
        if (row >= 0 && row < getNumRows() && browser.onLoad)
            browser.onLoad (browser.results[static_cast<size_t> (row)]);
    }

    ProgressionBrowser& browser;
};

ProgressionBrowser::ProgressionBrowser()
    : genreModel (std::make_unique<GenreModel> (*this)),
      progressionModel (std::make_unique<ProgressionModel> (*this))
{
    title.setText ("PROGRESSION LIBRARY", juce::dontSendNotification);
    title.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, Colours::textDim);
    addAndMakeVisible (title);

    search.setTextToShowWhenEmpty ("Search: jazz, sad, i VI, trap...", Colours::textDim);
    search.setFont (juce::FontOptions (13.5f));
    search.onTextChange = [this] { updateResults(); };
    addAndMakeVisible (search);

    genreList.setModel (genreModel.get());
    genreList.setRowHeight (22);
    genreList.setColour (juce::ListBox::backgroundColourId, Colours::panel);
    addAndMakeVisible (genreList);

    progressionList.setModel (progressionModel.get());
    progressionList.setRowHeight (38);
    progressionList.setColour (juce::ListBox::backgroundColourId, Colours::panel);
    addAndMakeVisible (progressionList);

    genreList.selectRow (0);
    updateResults();
}

ProgressionBrowser::~ProgressionBrowser()
{
    genreList.setModel (nullptr);
    progressionList.setModel (nullptr);
}

void ProgressionBrowser::selectGenre (const juce::String& genre)
{
    const auto& genres = melodee::progressionGenres();
    for (size_t i = 0; i < genres.size(); ++i)
        if (genre == juce::String (genres[i]))
            genreList.selectRow (static_cast<int> (i) + 1);
}

void ProgressionBrowser::updateResults()
{
    const auto query = search.getText().trim();
    std::vector<int> found = melodee::searchProgressions (query.toStdString());

    results.clear();
    for (int i : found)
        if (currentGenre.isEmpty() || juce::String (melodee::progressionLibrary()[static_cast<size_t> (i)].genre) == currentGenre)
            results.push_back (i);

    progressionList.updateContent();
    progressionList.deselectAllRows();
    progressionList.repaint();
}

void ProgressionBrowser::paint (juce::Graphics& g)
{
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
}

void ProgressionBrowser::resized()
{
    auto r = getLocalBounds().reduced (8);
    title.setBounds (r.removeFromTop (18));
    r.removeFromTop (4);
    search.setBounds (r.removeFromTop (28));
    r.removeFromTop (6);
    genreList.setBounds (r.removeFromTop (juce::jmax (120, r.getHeight() * 2 / 5)));
    r.removeFromTop (6);
    progressionList.setBounds (r);
}
