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
    auto readBmsChart(std::string& chart) -> models::bms::BmsChart;
};
} // namespace charts::chart_readers::bms

#endif // RHYTHMGAME_BMSCHARTREADER_H
