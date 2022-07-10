//
// Created by bobini on 09.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include "Chart.h"

namespace charts::models {
    class BmsChart : public Chart {
        std::unique_ptr<ChartInfo> chartInfo;
    public:
        BmsChart(std::unique_ptr<ChartInfo> chartInfo);
        auto getChartInfo() -> std::unique_ptr<ChartInfo> override;
    };
} // namespace charts::models

#endif // RHYTHMGAME_BMSCHART_H
