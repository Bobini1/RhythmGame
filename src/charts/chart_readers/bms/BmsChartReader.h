//
// Created by bobini on 07.07.2022.
//

#ifndef RHYTHMGAME_BMSCHARTREADER_H
#define RHYTHMGAME_BMSCHARTREADER_H

#include <string>

namespace charts {
    class BmsChart {};
}


namespace charts::chart_readers {
class BmsChartReader
{
  public:
    virtual ~BmsChartReader() = default;
    virtual BmsChart readBmsChart(std::string& chart) = 0;
};
} // namespace charts::chart_readers

#endif // RHYTHMGAME_BMSCHARTREADER_H
