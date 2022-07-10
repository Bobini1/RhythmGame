//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_CHARTREADER_H
#define RHYTHMGAME_CHARTREADER_H

#include <folly/FBString.h>

class SMChart;

namespace charts::cha

class SMChartReader
{
    virtual ~SMChartReader() = default;
    virtual SMChart readSMChart(folly::fbstring& chart);
};

#endif // RHYTHMGAME_CHARTREADER_H
