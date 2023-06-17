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
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    REQUIRE(chart.visibleNotes[0][0].first == measureLength * 3 / 2);
    auto index =
      GENERATE(range(1, charts::gameplay_models::BmsChart::columnNumber));
    REQUIRE(chart.visibleNotes[index].empty());
    for (const auto& column : chart.invisibleNotes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].first == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(bpm));
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0] == measureLength);
    REQUIRE(chart.barLines[1] == measureLength * 2);
}

TEST_CASE("A chart with a bpm change and a note is created successfully",
          "[BmsChart]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags =
      reader.readBmsChart("#BPM 240\n#BPM11 60\n#00111:0011\n#00108:0011");
    auto parsedChart = charts::parser_models::ParsedBmsChart(std::move(tags));
    auto chart = charts::gameplay_models::BmsChart(parsedChart, {});
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.visibleNotes[0].size() == 1);
    static constexpr auto bpm = 240.0;
    static constexpr auto bpm2 = 60.0;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    static constexpr auto measureLength2 =
      std::chrono::nanoseconds(
        static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm2)) /
        2 +
      measureLength / 2;
    REQUIRE(chart.visibleNotes[0].size() == 1);
    REQUIRE(chart.visibleNotes[0][0].first == measureLength * 3 / 2);
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0] == measureLength);
    REQUIRE(chart.barLines[1] == measureLength + measureLength2);
    REQUIRE(chart.bpmChanges.size() == 2);
    REQUIRE(chart.bpmChanges[0].first == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(bpm));
    REQUIRE(chart.bpmChanges[1].first == measureLength * 3 / 2);
    REQUIRE(chart.bpmChanges[1].second == Catch::Approx(bpm2));
}