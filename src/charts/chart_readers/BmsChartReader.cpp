//
// Created by bobini on 07.07.2022.
//

#include "BmsChartReader.h"
#include <tao/pegtl.hpp>

namespace {

} // namespace

namespace charts::chart_readers {
namespace {








class BmsChartReaderImpl : public BmsChartReader
{
  public:
    ~BmsChartReaderImpl() override = default;
    virtual auto readChart(std::string& chart) -> BmsChart
    {
        return BmsChart{};
    };
};
} // namespace
} // namespace charts::chart_readers
