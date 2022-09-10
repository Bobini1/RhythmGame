//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include <map>
#include <random>
#include "charts/models/Chart.h"
namespace charts::models {

/**
 * @brief Be-Music Source chart.
 */
class BmsChart final : public Chart
{
  public:
    using RandomRange = std::uniform_int_distribution<long>;
    using IfTag = long;

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

        // we have to use std::unique_ptr<std::multimap> because otherwise this
        // doesn't compile on MSVC. :)
        std::vector<
          std::pair<RandomRange, std::unique_ptr<std::multimap<IfTag, Tags>>>>
          randomBlocks; /*< Random blocks can hold any tags, including ones that
                           were already defined. */
    };

    /**
     * @brief Constructs a BMS chart from its tags. The BmsChart is able to
     * manage random blocks on its own.
     */
    explicit BmsChart(Tags tags);

    /**
     * @brief Writes the tags to lua. All randoms are resolved during this
     * operation.
     * @param writer
     */
    auto writeFullData(behaviour::SongDataWriterToLua writer) const
      -> void override;

  private:
    Tags tags;
};
} // namespace charts::models
#endif // RHYTHMGAME_BMSCHART_H
