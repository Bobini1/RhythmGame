//
// Created by bobini on 16.06.23.
//

#include "resource_managers/loadBmsSounds.h"
#include "sounds/SoundBuffer.h"
#include "charts/BmsNotesData.h"
#include "charts/ParsedBmsChart.h"
#include "charts/ReadBmsFile.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace {
auto randomGenerator = [](charts::ParsedBmsChart::RandomRange range) {
    return range;
};
} // namespace

TEST_CASE("SPEED interpolates time keyframes without altering chart timing",
          "[BmsNotesData][SPEED]")
{
    using namespace std::chrono_literals;
    using charts::BmsNotesData;
    const auto read = [](std::string_view text) {
        return BmsNotesData::fromParsedChart(
          { charts::readBmsChart(text, randomGenerator) });
    };
    SECTION("implicit starting point and final hold")
    {
        const auto chart = read("#BPM 120\n#SPEED01 2\n#002SP:01\n");
        REQUIRE(chart.speedChanges.size() == 1);
        CHECK(chart.speedChanges[0].timestamp == 4s);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, -1s) == 1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 0s) == 1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 1s) == 1.25);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 4s) == 2);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 8s) == 2);
        CHECK(BmsNotesData::speedMultiplierAt({}, 8s) == 1);
    }
    SECTION("repeated values delimit a hold before the ramp")
    {
        const auto chart = read("#SPEED01 1\n#SPEED02 2\n"
                                "#000SP:01\n#001SP:01\n#003SP:02\n");
        REQUIRE(chart.speedChanges.size() == 3);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 1s) == 1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 2s) == 1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 4s) == 1.5);
    }
    SECTION("last placement at a shared position wins before interpolation")
    {
        const auto chart = read("#SPEED01 2\n#SPEED02 3\n"
                                "#001SP:01\n#001SP:02\n");
        REQUIRE(chart.speedChanges.size() == 1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 1s) == 2);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 2s) == 3);
    }
    SECTION("BPM meter SCROLL and STOP determine keyframe timestamps")
    {
        const std::string base =
          "#BPM 120\n#BPM01 60\n#STOP01 48\n"
          "#SCROLL01 2\n#00102:0.5\n#00108:01\n#00109:0001\n"
          "#001SC:01\n#00111:00010001\n#00211:01\n";
        const auto plain = read(base);
        const auto chart = read(base + "#SPEED01 1\n#SPEED02 3\n"
                                       "#000SP:01\n#001SP:0002\n#002SP:01\n");
        REQUIRE(chart.speedChanges.size() == 3);
        CHECK(chart.speedChanges[1].timestamp == 3s);
        CHECK(chart.speedChanges[2].timestamp == 5s);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 1500ms) == 2);
        // The ramp continues during the STOP from 3 to 4 seconds.
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 3500ms) ==
              2.5);
        REQUIRE(chart.notes[0].size() == plain.notes[0].size());
        for (size_t i = 0; i < chart.notes[0].size(); ++i)
            CHECK(chart.notes[0][i].time == plain.notes[0][i].time);
        REQUIRE(chart.bpmChanges.size() == plain.bpmChanges.size());
        for (size_t i = 0; i < chart.bpmChanges.size(); ++i) {
            CHECK(chart.bpmChanges[i].timestamp ==
                  plain.bpmChanges[i].timestamp);
            CHECK(chart.bpmChanges[i].bpm == plain.bpmChanges[i].bpm);
            CHECK(chart.bpmChanges[i].scroll == plain.bpmChanges[i].scroll);
        }
    }
    SECTION(
      "conditional placements survive merging and BASE62 stays case sensitive")
    {
        const auto chart =
          read("#BASE 62\n#SPEEDaa -1\n#SPEEDAA 0\n"
               "#RANDOM 2\n#IF 1\n#000SP:AA\n#ENDIF\n"
               "#IF 2\n#000SP:aa\n#001SP:AA\n#ENDIF\n#ENDRANDOM\n");
        REQUIRE(chart.speedChanges.size() == 2);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 0s) == -1);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 1s) == -0.5);
        CHECK(BmsNotesData::speedMultiplierAt(chart.speedChanges, 2s) == 0);
    }
}

TEST_CASE("An empty chart is created successfully", "[BmsNotesData]")
{
    auto tags = charts::readBmsChart("", randomGenerator);
    auto parsedChart = charts::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::BmsNotesData::fromParsedChart(parsedChart);
    REQUIRE(chart.bgmNotes.empty());
    for (const auto& column : chart.notes) {
        REQUIRE(column.empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].timestamp.timestamp ==
            std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].bpm == Catch::Approx(120.0));
    REQUIRE(chart.barLines.empty());
}

TEST_CASE("A chart with a single note is created successfully",
          "[BmsNotesData]")
{
    auto tags = charts::readBmsChart("#00111:0011", randomGenerator);
    auto parsedChart = charts::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::BmsNotesData::fromParsedChart(parsedChart);
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.notes[0].size() == 1);
    static constexpr auto bpm = chart.defaultBpm;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    REQUIRE(chart.notes[0][0].time.timestamp == measureLength * 3 / 2);
    for (auto index = 1ul; index < charts::BmsNotesData::columnNumber;
         index++) {
        REQUIRE(chart.notes[index].empty());
    }
    REQUIRE(chart.bpmChanges.size() == 1);
    REQUIRE(chart.bpmChanges[0].timestamp.timestamp ==
            std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].bpm == Catch::Approx(bpm));
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength * 2);
}

TEST_CASE("A chart with a bpm change and a note is created successfully",
          "[BmsNotesData]")
{
    auto tags = charts::readBmsChart(
      "#BPM 240\n#BPM11 60\n#00111:0011\n#00108:0011", randomGenerator);
    auto parsedChart = charts::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::BmsNotesData::fromParsedChart(parsedChart);
    REQUIRE(chart.bgmNotes.empty());
    REQUIRE(chart.notes[0].size() == 1);
    static constexpr auto bpm = 240.0;
    static constexpr auto bpm2 = 60.0;
    static constexpr auto measureLength = std::chrono::nanoseconds(
      static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm));
    static constexpr auto measureLength2 =
      std::chrono::nanoseconds(
        static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / bpm2)) /
        2 +
      measureLength / 2;
    REQUIRE(chart.notes[0].size() == 1);
    REQUIRE(chart.notes[0][0].time.timestamp == measureLength * 3 / 2);
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength + measureLength2);
    REQUIRE(chart.bpmChanges.size() == 2);
    REQUIRE(chart.bpmChanges[0].timestamp.timestamp ==
            std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].bpm == Catch::Approx(bpm));
    REQUIRE(chart.bpmChanges[1].timestamp.timestamp == measureLength * 3 / 2);
    REQUIRE(chart.bpmChanges[1].bpm == Catch::Approx(bpm2));
}

TEST_CASE("Multiple BPM changes mid-measure are handled correctly",
          "[BmsNotesData]")
{
    auto tags =
      charts::readBmsChart("#00111:00110011\n#00103:3c78", randomGenerator);
    auto parsedChart = charts::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::BmsNotesData::fromParsedChart(parsedChart);
    static constexpr auto bpm = charts::BmsNotesData::defaultBpm;
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
    REQUIRE(chart.notes[0].size() == 2);
    REQUIRE(chart.notes[0][0].time.timestamp ==
            measureLength + halvedBpmPeriod / 2);
    REQUIRE(chart.notes[0][1].time.timestamp ==
            measureLength + halvedBpmPeriod + measureLength / 4);
    REQUIRE(chart.barLines.size() == 2);
    REQUIRE(chart.barLines[0].timestamp == measureLength);
    REQUIRE(chart.barLines[1].timestamp == measureLength + measureLength2);
    REQUIRE(chart.bpmChanges.size() == 3);
    REQUIRE(chart.bpmChanges[0].timestamp.timestamp ==
            std::chrono::nanoseconds(0));
    REQUIRE(chart.bpmChanges[0].bpm == Catch::Approx(bpm));
    REQUIRE(chart.bpmChanges[1].timestamp.timestamp == measureLength);
    REQUIRE(chart.bpmChanges[1].bpm == Catch::Approx(bpm2));
    REQUIRE(chart.bpmChanges[2].timestamp.timestamp ==
            measureLength + halvedBpmPeriod);
    REQUIRE(chart.bpmChanges[2].bpm == Catch::Approx(bpm3));
}

TEST_CASE("Bgm notes have the right timestamps", "[BmsNotesData]")
{
    auto tags = charts::readBmsChart("#00101:0011\n#00101:1111\n#00103:3c",
                                     randomGenerator);
    auto parsedChart = charts::ParsedBmsChart{ std::move(tags) };
    auto chart = charts::BmsNotesData::fromParsedChart(parsedChart);
    static constexpr auto bpm = charts::BmsNotesData::defaultBpm;
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
