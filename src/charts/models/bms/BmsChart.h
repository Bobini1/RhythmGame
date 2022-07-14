//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include "../Chart.h"

namespace charts::models::bms {
class BmsChart : public Chart
{
    ChartInfo chartInfo;

  public:
    explicit BmsChart(ChartInfo chartInfo)
      : chartInfo(std::move(chartInfo))
    {
    }
    auto getChartInfo() -> const ChartInfo& override { return chartInfo; }
};
} // namespace charts::models::bms

#endif // RHYTHMGAME_BMSCHART_H
