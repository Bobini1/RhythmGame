//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include "ParsedBmsChart.h"
#include <functional>
namespace charts {
auto
readBmsChart(
  std::string_view chart,
  const std::function<ParsedBmsChart::RandomRange(ParsedBmsChart::RandomRange)>&
    randomGenerator) -> ParsedBmsChart;
} // namespace charts
#endif // RHYTHMGAME_BMSCHARTREADER_H
