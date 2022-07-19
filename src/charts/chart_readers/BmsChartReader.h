//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>
#include <memory>
#include "charts/models/Chart.h"

namespace charts::chart_readers {
class BmsChartReader
{
  public:
    [[nodiscard]] auto readBmsChart(std::string chart) const
      -> charts::models::Chart;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
