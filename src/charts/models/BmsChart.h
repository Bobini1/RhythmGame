//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include <map>
#include <random>
#include "charts/models/Chart.h"
namespace charts::models {
class BmsChart : public Chart
{
  public:
    using RandomRange = std::uniform_int_distribution<long>;
    using IfTag = long;
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
          randomBlocks;
    };
    explicit BmsChart(Tags tags);
    virtual auto writeFullData(behaviour::SongDataWriter writer) const
      -> void override;

  private:
    BmsChart::Tags tags;
};
} // namespace charts::models
#endif // RHYTHMGAME_BMSCHART_H
