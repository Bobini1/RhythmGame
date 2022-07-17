//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include "charts/chart_readers/BmsChartReader.h"
#include <string>

TEST_CASE("Check if Title is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.getTitle() == "END TIME"s);
}

TEST_CASE("Check if Artist is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.getArtist() == "cres"s);
}