//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>
#include <memory>
#include "../../models/bms/BmsChart.h"

namespace charts::chart_readers::bms {
class BmsChartReader
{
  public:
    virtual ~BmsChartReader() = default;
    virtual auto readBmsChart(std::string& chart) -> models::bms::BmsChart;
};

auto
createBmsChartReader() -> std::unique_ptr<BmsChartReader>;
} // namespace charts::chart_readers::bms

#endif // RHYTHMGAME_BMSCHARTREADER_H
