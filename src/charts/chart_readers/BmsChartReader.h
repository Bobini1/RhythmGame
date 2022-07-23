//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>
#include <memory>
#include "charts/models/BmsChart.h"
#include <random>
#include <map>

namespace charts::chart_readers {

class BmsChartReader
{
  public:
    [[nodiscard]] auto readBmsChart(const std::string& chart) const
      -> std::unique_ptr<charts::models::BmsChart>;

    [[nodiscard]] auto readBmsChartTags(std::string chart) const
      -> std::optional<models::BmsChart::Tags>;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
