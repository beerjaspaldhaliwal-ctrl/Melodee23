#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace melodee
{

/** A progression from the built-in library. `numerals` are roman numerals read in the key
    context of `scale` (major-type scales: major-scale numerals; minor-type scales: natural minor). */
struct ProgressionDef
{
    std::string genre;
    std::string name;
    std::string numerals;
    std::string scale;
    std::string description;
};

const std::vector<ProgressionDef>& progressionLibrary();

/** Genre names in display order. */
const std::vector<std::string>& progressionGenres();

/** Indices into progressionLibrary() for one genre. */
std::vector<int> progressionsInGenre (std::string_view genre);

/** Indices of progressions whose genre, name, numerals or description contain all words of `query`. */
std::vector<int> searchProgressions (std::string_view query);

} // namespace melodee
