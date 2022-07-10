//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include "../Chart.h"

namespace charts::models::bms {
class BmsChart : public Chart
{
  public:
    BmsChart(std::unique_ptr<ChartInfo> chartInfo);
    auto getChartInfo() -> const ChartInfo& override;
};
} // namespace charts::models::bms

#endif // RHYTHMGAME_BMSCHART_H
