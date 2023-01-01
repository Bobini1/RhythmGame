//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include <map>
#include <random>
#include <optional>
#include <memory>
#include <array>
namespace charts::models {

/**
 * @brief Be-Music Source chart.
 */

class BmsChart
{
  public:
    using RandomRange = std::uniform_int_distribution<int64_t>;
    using IfTag = int64_t;

    struct Measure
    {
        static constexpr auto columnNumber = 9;
        std::array<std::vector<std::string>, columnNumber> p1VisibleNotes;
        std::vector<std::vector<std::string>> bgmNotes;

        std::vector<std::string> bgaNotes;
    };

    /**
     * @brief Tags that a BMS chart can have.
     */
    struct Tags
    {
        std::optional<std::string> title;
        std::optional<std::string> artist;
        std::optional<double> bpm;
        std::optional<std::string> subTitle;
        std::optional<std::string> subArtist;
        std::optional<std::string> genre;
        std::map<int64_t, Measure> measures;

        // we have to use std::unique_ptr<std::multimap> because otherwise
        // this doesn't compile on MSVC. :)
        std::vector<
          std::pair<RandomRange, std::unique_ptr<std::multimap<IfTag, Tags>>>>
          randomBlocks; /*< Random blocks can hold any tags, including ones
                           that were already defined. */
    };

    /**
     * @brief Constructs a BMS chart from its tags. The BmsChart is able to
     * manage random blocks on its own.
     */
    explicit BmsChart(Tags tags);

  private:
    Tags tags;
};
} // namespace charts::models
#endif // RHYTHMGAME_BMSCHART_H
