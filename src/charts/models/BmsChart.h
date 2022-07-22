//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H
#include "charts/models/Chart.h"
#include "charts/chart_readers/BmsChartReader.h"
namespace charts::models {
class BmsChart : public Chart
{
    BmsChart(chart_readers::tags tags);
};
} // namespace charts::models
#endif // RHYTHMGAME_BMSCHART_H
