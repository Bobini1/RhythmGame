//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include <charts/chart_readers/bms/BmsChartReader.h>
#include <string>

TEST_CASE("Check if #PLAYER is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::bms::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(static_cast<const std::string&>(res.getChartInfo().title) == "END TIME"s);
}