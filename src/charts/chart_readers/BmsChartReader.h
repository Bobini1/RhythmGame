//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>
#include <memory>
#include "charts/models/BmsChart.h"
#include <random>
#include <map>

namespace charts::chart_readers {

/**
 * @brief Reads BMS charts.
 */
class BmsChartReader
{
  public:
    /**
     * @brief Reads BMS chart from string.
     * @param chart String to read.
     * @return BMS chart.
     */
    [[nodiscard]] auto readBmsChart(const std::string& chart) const
      -> std::unique_ptr<charts::models::BmsChart>;

    /**
     * @brief Used for testing, gives raw tag data.
     * @param chart String to read.
     * @return Raw tag data.
     */
    [[nodiscard]] auto readBmsChartTags(const std::string& chart) const
      -> std::optional<models::BmsChart::Tags>;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
