//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <optional>
#include "charts/models/BmsChart.h"

namespace charts::chart_readers {

/**
 * @brief Reads BMS charts.
 */
class BmsChartReader final
{
  public:
    /**
     * @brief Reads a BMS chart from a string.
     * @param chart String to read.
     * @return Raw tag data.
     */
    [[nodiscard]] auto readBmsChart(const std::string& chart) const
      -> models::BmsChart::Tags;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
