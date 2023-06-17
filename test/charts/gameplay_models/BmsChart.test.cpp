//
// Created by bobini on 16.06.23.
//

#include "charts/helper_functions/loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"
#include "charts/gameplay_models/BmsChart.h"
#include "charts/parser_models/ParsedBmsChart.h"
#include "charts/chart_readers/BmsChartReader.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
// range
#include <catch2/generators/catch_generators_range.hpp>

TEST_CASE("An empty chart is created successfully", "[BmsChart]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("");
    auto parsedChart = charts::parser_models::ParsedBmsChart(std::move(tags));
    auto chart = charts::gameplay_models::BmsChart(parsedChart, {});
    REQUIRE(chart.bgmNotes.empty());
    for (const auto& column : chart.visibleNotes) {
        REQUIRE(column.empty());
    }
    for (const auto& column : chart.invisibleNotes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].first == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(120.0));
    REQUIRE(chart.barLines.empty());
}

TEST_CASE("A chart with a single note is created successfully", "[BmsChart]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("#00111:0011");
    auto parsedChart = charts::parser_models::ParsedBmsChart(std::move(tags));
    auto chart = charts::gameplay_models::BmsChart(parsedChart, {});
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.visibleNotes[0].size() == 1);
    static constexpr auto bpm = chart.defaultBpm;
    auto timestamp = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm)); // measure 0
    timestamp += std::chrono::nanoseconds(
                   static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm)) /
                 2; // measure 1
    REQUIRE(chart.visibleNotes[0][0].first == timestamp);
    auto index = GENERATE(range(1, charts::gameplay_models::BmsChart::columnNumber));
    REQUIRE(chart.visibleNotes[index].empty());
    for (const auto& column : chart.invisibleNotes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
}