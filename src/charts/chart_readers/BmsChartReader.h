//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <optional>
#include "charts/parser_models/ParsedBmsChart.h"
#include <functional>

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
    [[nodiscard]] auto readBmsChart(
      std::string_view chart,
      std::function<parser_models::ParsedBmsChart::RandomRange(
        parser_models::ParsedBmsChart::RandomRange)> randomGenerator) const
      -> parser_models::ParsedBmsChart;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
