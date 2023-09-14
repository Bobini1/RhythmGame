//
// Created by bobini on 16.06.23.
//

#include "charts/helper_functions/loadBmsSounds.h"
#include "sounds/OpenAlSoundBuffer.h"
#include "charts/gameplay_models/BmsNotesData.h"
#include "charts/parser_models/ParsedBmsChart.h"
#include "charts/chart_readers/BmsChartReader.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

TEST_CASE("An empty chart is created successfully", "[BmsNotesData]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("");
    auto parsedChart = charts::parser_models::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::gameplay_models::BmsNotesData(parsedChart);
    REQUIRE(chart.bgmNotes.empty());
    for (const auto& column : chart.visibleNotes) {
        REQUIRE(column.empty());
    }
    for (const auto& column : chart.invisibleNotes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].first.timestamp == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(120.0));
    REQUIRE(chart.barLines.empty());
}

TEST_CASE("A chart with a single note is created successfully",
          "[BmsNotesData]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("#00111:0011");
    auto parsedChart = charts::parser_models::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::gameplay_models::BmsNotesData(parsedChart);
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.visibleNotes[0].size() == 1);
    static constexpr auto bpm = chart.defaultBpm;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    REQUIRE(chart.visibleNotes[0][0].time.timestamp == measureLength * 3 / 2);
    for (auto index = 1ul;
         index < charts::gameplay_models::BmsNotesData::columnNumber;
         index++) {
        REQUIRE(chart.visibleNotes[index].empty());
    }
    for (const auto& column : chart.invisibleNotes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].first.timestamp == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(bpm));
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength * 2);
}

TEST_CASE("A chart with a bpm change and a note is created successfully",
          "[BmsNotesData]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags =
      reader.readBmsChart("#BPM 240\n#BPM11 60\n#00111:0011\n#00108:0011");
    auto parsedChart = charts::parser_models::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::gameplay_models::BmsNotesData(parsedChart);
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
    REQUIRE(chart.visibleNotes[0][0].time.timestamp == measureLength * 3 / 2);
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength + measureLength2);
    REQUIRE(chart.bpmChanges.size() == 2);
    REQUIRE(chart.bpmChanges[0].first.timestamp == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(bpm));
    REQUIRE(chart.bpmChanges[1].first.timestamp == measureLength * 3 / 2);
    REQUIRE(chart.bpmChanges[1].second == Catch::Approx(bpm2));
}

TEST_CASE("Multiple BPM changes mid-measure are handled correctly",
          "[BmsNotesData]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("#00111:00110011\n#00103:3c78");
    auto parsedChart = charts::parser_models::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::gameplay_models::BmsNotesData(parsedChart);
    static constexpr auto bpm =
      charts::gameplay_models::BmsNotesData::defaultBpm;
    static constexpr auto bpm2 = 60.0;
    static constexpr auto bpm3 = 120.0;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    static constexpr auto halvedBpmPeriod =
      std::chrono::nanoseconds(
        static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm2)) /
      2;
    static constexpr auto measureLength2 = halvedBpmPeriod + measureLength / 2;
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.visibleNotes[0].size() == 2);
    REQUIRE(chart.visibleNotes[0][0].time.timestamp ==
            measureLength + halvedBpmPeriod / 2);
    REQUIRE(chart.visibleNotes[0][1].time.timestamp ==
            measureLength + halvedBpmPeriod + measureLength / 4);
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength + measureLength2);
    REQUIRE(chart.bpmChanges.size() == 3);
    REQUIRE(chart.bpmChanges[0].first.timestamp == std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].second == Catch::Approx(bpm));
    REQUIRE(chart.bpmChanges[1].first.timestamp == measureLength);
    REQUIRE(chart.bpmChanges[1].second == Catch::Approx(bpm2));
    REQUIRE(chart.bpmChanges[2].first.timestamp ==
            measureLength + halvedBpmPeriod);
    REQUIRE(chart.bpmChanges[2].second == Catch::Approx(bpm3));
}

TEST_CASE("Bgm notes have the right timestamps", "[BmsNotesData]")
{
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart("#00101:0011\n#00101:1111\n#00103:3c");
    auto parsedChart = charts::parser_models::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::gameplay_models::BmsNotesData(parsedChart);
    static constexpr auto bpm =
      charts::gameplay_models::BmsNotesData::defaultBpm;
    static constexpr auto bpm2 = 60.0;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    static constexpr auto halvedBpmPeriod =
      std::chrono::nanoseconds(
        static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm2)) /
      2;
    REQUIRE(chart.bgmNotes.size() == 3);
    REQUIRE(chart.bgmNotes[0].first.timestamp == measureLength);
    REQUIRE(chart.bgmNotes[1].first.timestamp ==
            measureLength + halvedBpmPeriod);
    REQUIRE(chart.bgmNotes[2].first.timestamp ==
            measureLength + halvedBpmPeriod);
}