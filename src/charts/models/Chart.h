//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include "ChartInfo.h"

namespace charts::models {
class Chart
{
  public:
    virtual ~Chart() = default;
    virtual auto getChartInfo() -> const ChartInfo& = 0;
};
} // namespace charts::models

#endif // RHYTHMGAME_CHART_H
