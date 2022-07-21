//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>
#include <memory>
#include "charts/models/Chart.h"
#include <random>
#include <map>

namespace charts::chart_readers {
using RandomRange = std::uniform_int_distribution<long>;
using IfTag = long;
struct tags
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
      std::pair<RandomRange, std::unique_ptr<std::multimap<IfTag, tags>>>>
      randomBlocks;
};
class BmsChartReader
{
  public:
    [[nodiscard]] auto readBmsChart(std::string chart) const
      -> charts::models::Chart;

    [[nodiscard]] auto readBmsChartTags(std::string chart) const -> tags;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
