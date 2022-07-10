//
// Created by bobini on 09.07.2022.
//

#include "BmsChart.h"

#include <utility>

namespace charts::models::bms {
class BmsChartImpl : public BmsChart
{
    ChartInfo chartInfo;
  public:
    explicit BmsChartImpl(ChartInfo chartInfo)
    : chartInfo(std::move(chartInfo))
    {}
    auto getChartInfo() -> const ChartInfo& override {}
};
} // namespace charts::models::bms
