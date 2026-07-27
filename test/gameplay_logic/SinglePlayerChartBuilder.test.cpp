#include "resource_managers/DesktopPlayerChartCoordinator.h"
#include "gameplay_logic/SinglePlayerChartBuilder.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using gameplay_logic::ChartData;
using gameplay_logic::PlayerChartBuildOptions;
using resource_managers::DesktopPlayerBuildOptions;
using resource_managers::DesktopScoreIdentity;
using resource_managers::DpOptions;
using resource_managers::NoteOrderAlgorithm;
using Note = charts::BmsNotesData::Note;
using NoteType = charts::BmsNotesData::NoteType;
using LaneSounds =
  std::array<std::vector<std::uint16_t>, charts::BmsNotesData::columnNumber>;

auto
makeNote(const std::int64_t timestamp,
         const NoteType type,
         const std::uint16_t sound) -> Note
{
    return {
        .time = { std::chrono::nanoseconds{ timestamp },
                  static_cast<double>(timestamp) / 100.0,
                  static_cast<double>(timestamp) / 50.0 },
        .snap = { 1.0, 16.0 },
        .noteType = type,
        .sound = sound,
    };
}

auto
makeNotesData() -> charts::BmsNotesData
{
    auto data = charts::BmsNotesData{};
    for (auto column = std::size_t{}; column < data.notes.size(); ++column) {
        data.notes[column].push_back(
          makeNote(1'000 + static_cast<std::int64_t>(column) * 100,
                   NoteType::Normal,
                   static_cast<std::uint16_t>(100 + column)));
    }
    data.notes[1].push_back(makeNote(4'000, NoteType::Invisible, 401));
    data.notes[2].push_back(makeNote(5'000, NoteType::Landmine, 402));
    data.notes[3].push_back(makeNote(6'000, NoteType::LongNoteBegin, 403));
    data.notes[3].push_back(makeNote(7'000, NoteType::LongNoteEnd, 404));
    data.notes[7].push_back(makeNote(8'000, NoteType::Normal, 407));
    data.notes[11].push_back(makeNote(9'000, NoteType::LongNoteBegin, 411));
    data.notes[11].push_back(makeNote(10'000, NoteType::LongNoteEnd, 412));

    data.bgmNotes = {
        { charts::BmsNotesData::Time{
            std::chrono::nanoseconds{ 500 }, 0.5, 0.75 },
          900 },
    };
    data.bpmChanges = {
        { 150.0,
          1.0,
          charts::BmsNotesData::Time{
            std::chrono::nanoseconds{ 0 }, 0.0, 0.0 } },
        { 225.0,
          0.5,
          charts::BmsNotesData::Time{
            std::chrono::nanoseconds{ 12'000 }, 12.0, 6.0 } },
    };
    data.barLines = {
        charts::BmsNotesData::Time{ std::chrono::nanoseconds{ 0 }, 0.0, 0.0 },
        charts::BmsNotesData::Time{
          std::chrono::nanoseconds{ 16'000 }, 16.0, 8.0 },
    };
    return data;
}

auto
makeChartData(const ChartData::Keymode keymode,
              QList<qint64> randomSequence = { 2, 1 }) -> ChartData
{
    return ChartData{ QStringLiteral("builder fixture"),
                      QStringLiteral("artist"),
                      {},
                      {},
                      {},
                      {},
                      {},
                      {},
                      100.0,
                      300.0,
                      12,
                      4,
                      !randomSequence.empty(),
                      std::move(randomSequence),
                      16,
                      2,
                      2,
                      0,
                      1,
                      20'000,
                      150.0,
                      225.0,
                      150.0,
                      150.0,
                      168.75,
                      3.0,
                      2.0,
                      1.0,
                      QStringLiteral("fixture.bms"),
                      0,
                      QStringLiteral("SHA256"),
                      QStringLiteral("MD5"),
                      keymode,
                      {},
                      {},
                      1 };
}

auto
laneSounds(const std::array<std::vector<Note>,
                            charts::BmsNotesData::columnNumber>& notes)
  -> LaneSounds
{
    auto result = LaneSounds{};
    for (auto column = std::size_t{}; column < notes.size(); ++column) {
        for (const auto& note : notes[column]) {
            result[column].push_back(note.sound);
        }
    }
    return result;
}

auto
noteCount(const LaneSounds& notes) -> std::size_t
{
    return std::accumulate(
      notes.begin(),
      notes.end(),
      std::size_t{},
      [](const auto count, const auto& lane) { return count + lane.size(); });
}

struct GoldenBuild
{
    std::string_view name;
    ChartData::Keymode sourceKeymode;
    PlayerChartBuildOptions options;
    DpOptions effectiveDpOptions;
    ChartData::Keymode effectiveKeymode;
    std::array<std::uint64_t, 2> shuffleSeeds;
    std::array<QList<int>, 2> shuffleColumns;
    std::uint64_t storedSeed;
    QList<int> storedPermutation;
    LaneSounds lanes;
};

auto
goldens() -> std::array<GoldenBuild, 9>
{
    return {
        GoldenBuild{ "7K normal",
                     ChartData::Keymode::K7,
                     { .randomSeed = 391 },
                     DpOptions::Off,
                     ChartData::Keymode::K7,
                     { 0, 0 },
                     { QList<int>{ 0, 1, 2, 3, 4, 5, 6, 7 }, QList<int>{} },
                     0,
                     { 0, 1, 2, 3, 4, 5, 6, 7 },
                     { std::vector<std::uint16_t>{ 100 },
                       { 101, 401 },
                       { 102, 402 },
                       { 103, 403, 404 },
                       { 104 },
                       { 105 },
                       { 106 },
                       { 107, 407 },
                       { 108 },
                       { 109 },
                       { 110 },
                       { 111, 411, 412 },
                       { 112 },
                       { 113 },
                       { 114 },
                       { 115 } } },
        GoldenBuild{
          "7K mirror",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::Mirror, .randomSeed = 391 },
          DpOptions::Off,
          ChartData::Keymode::K7,
          { 0, 0 },
          { QList<int>{ 6, 5, 4, 3, 2, 1, 0, 7 }, QList<int>{} },
          0,
          { 6, 5, 4, 3, 2, 1, 0, 7 },
          { std::vector<std::uint16_t>{ 106 },
            { 105 },
            { 104 },
            { 103, 403, 404 },
            { 102, 402 },
            { 101, 401 },
            { 100 },
            { 107, 407 },
            { 108 },
            { 109 },
            { 110 },
            { 111, 411, 412 },
            { 112 },
            { 113 },
            { 114 },
            { 115 } } },
        GoldenBuild{
          "7K seeded random",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::Random, .randomSeed = 391 },
          DpOptions::Off,
          ChartData::Keymode::K7,
          { 391, 0 },
          { QList<int>{ 1, 2, 6, 0, 4, 3, 5, 7 }, QList<int>{} },
          391,
          { 1, 2, 6, 0, 4, 3, 5, 7 },
          { std::vector<std::uint16_t>{ 101, 401 },
            { 102, 402 },
            { 106 },
            { 100 },
            { 104 },
            { 103, 403, 404 },
            { 105 },
            { 107, 407 },
            { 108 },
            { 109 },
            { 110 },
            { 111, 411, 412 },
            { 112 },
            { 113 },
            { 114 },
            { 115 } } },
        GoldenBuild{
          "7K seeded S-random",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::SRandom, .randomSeed = 391 },
          DpOptions::Off,
          ChartData::Keymode::K7,
          { 391, 0 },
          { QList<int>{ 0, 1, 2, 3, 4, 5, 6, 7 }, QList<int>{} },
          391,
          { 0, 1, 2, 3, 4, 5, 6, 7 },
          { std::vector<std::uint16_t>{ 102, 403, 404 },
            { 100, 401 },
            { 103 },
            { 105 },
            { 106 },
            { 104 },
            { 101, 402 },
            { 107, 407 },
            { 108 },
            { 109 },
            { 110 },
            { 111, 411, 412 },
            { 112 },
            { 113 },
            { 114 },
            { 115 } } },
        GoldenBuild{ "14K DP flip",
                     ChartData::Keymode::K14,
                     { .noteOrderP1 = NoteOrderAlgorithm::Mirror,
                       .noteOrderP2 = NoteOrderAlgorithm::Random,
                       .dpOptions = DpOptions::Flip,
                       .randomSeed = 391 },
                     DpOptions::Flip,
                     ChartData::Keymode::K14,
                     { 0, 1 },
                     { QList<int>{ 6, 5, 4, 3, 2, 1, 0, 7 },
                       QList<int>{ 3, 4, 1, 5, 2, 6, 0, 7 } },
                     0,
                     { 6, 5, 4, 3, 2, 1, 0, 7, 3, 4, 1, 5, 2, 6, 0, 7 },
                     { std::vector<std::uint16_t>{ 108 },
                       { 109 },
                       { 110 },
                       { 111, 411, 412 },
                       { 112 },
                       { 113 },
                       { 114 },
                       { 115 },
                       { 103, 403, 404 },
                       { 102, 402 },
                       { 105 },
                       { 101, 401 },
                       { 104 },
                       { 100 },
                       { 106 },
                       { 107, 407 } } },
        GoldenBuild{ "14K LR2 flip with shared RNG",
                     ChartData::Keymode::K14,
                     { .noteOrderP1 = NoteOrderAlgorithm::Lr2Random,
                       .noteOrderP2 = NoteOrderAlgorithm::Lr2Random,
                       .dpOptions = DpOptions::Lr2Flip,
                       .randomSeed = 7 },
                     DpOptions::Lr2Flip,
                     ChartData::Keymode::K14,
                     { 7, 7 },
                     { QList<int>{ 2, 3, 0, 6, 4, 5, 1, 7 },
                       QList<int>{ 3, 6, 2, 5, 1, 4, 0, 7 } },
                     7,
                     { 2, 3, 0, 6, 4, 5, 1, 7, 3, 6, 2, 5, 1, 4, 0, 7 },
                     { std::vector<std::uint16_t>{ 110 },
                       { 111, 411, 412 },
                       { 108 },
                       { 114 },
                       { 112 },
                       { 113 },
                       { 109 },
                       { 115 },
                       { 103, 403, 404 },
                       { 106 },
                       { 102, 402 },
                       { 105 },
                       { 101, 401 },
                       { 104 },
                       { 100 },
                       { 107, 407 } } },
        GoldenBuild{ "7K battle",
                     ChartData::Keymode::K7,
                     { .noteOrderP1 = NoteOrderAlgorithm::Random,
                       .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                       .dpOptions = DpOptions::Battle,
                       .randomSeed = 391 },
                     DpOptions::Battle,
                     ChartData::Keymode::K14,
                     { 391, 0 },
                     { QList<int>{ 1, 2, 6, 0, 4, 3, 5, 7 },
                       QList<int>{ 6, 5, 4, 3, 2, 1, 0, 7 } },
                     391,
                     { 1, 2, 6, 0, 4, 3, 5, 7, 6, 5, 4, 3, 2, 1, 0, 7 },
                     { std::vector<std::uint16_t>{ 101, 401 },
                       { 102, 402 },
                       { 106 },
                       { 100 },
                       { 104 },
                       { 103, 403, 404 },
                       { 105 },
                       { 107, 407 },
                       { 100 },
                       { 101, 401 },
                       { 102, 402 },
                       { 103, 403, 404 },
                       { 104 },
                       { 105 },
                       { 106 },
                       { 107, 407 } } },
        GoldenBuild{ "legacy pre-1.3.0 5K replay",
                     ChartData::Keymode::K5,
                     { .noteOrderP1 = NoteOrderAlgorithm::Random,
                       .randomSeed = 391,
                       .usePre130 = true },
                     DpOptions::Off,
                     ChartData::Keymode::K5,
                     { 391, 0 },
                     { QList<int>{ 1, 2, 6, 0, 5, 3, 4, 7 }, QList<int>{} },
                     391,
                     { 1, 2, 6, 0, 5, 3, 4, 7 },
                     { std::vector<std::uint16_t>{ 101, 401 },
                       { 102, 402 },
                       { 106 },
                       { 100 },
                       { 105 },
                       { 103, 403, 404 },
                       { 104 },
                       { 107, 407 },
                       { 108 },
                       { 109 },
                       { 110 },
                       { 111, 411, 412 },
                       { 112 },
                       { 113 },
                       { 114 },
                       { 115 } } },
        GoldenBuild{ "second player forced DP Off",
                     ChartData::Keymode::K14,
                     { .noteOrderP1 = NoteOrderAlgorithm::Random,
                       .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                       .dpOptions = DpOptions::Off,
                       .randomSeed = 947 },
                     DpOptions::Off,
                     ChartData::Keymode::K14,
                     { 947, 0 },
                     { QList<int>{ 6, 2, 0, 3, 5, 1, 4, 7 },
                       QList<int>{ 6, 5, 4, 3, 2, 1, 0, 7 } },
                     947,
                     { 6, 2, 0, 3, 5, 1, 4, 7, 6, 5, 4, 3, 2, 1, 0, 7 },
                     { std::vector<std::uint16_t>{ 106 },
                       { 102, 402 },
                       { 100 },
                       { 103, 403, 404 },
                       { 105 },
                       { 101, 401 },
                       { 104 },
                       { 107, 407 },
                       { 114 },
                       { 113 },
                       { 112 },
                       { 111, 411, 412 },
                       { 110 },
                       { 109 },
                       { 108 },
                       { 115 } } },
    };
}

} // namespace

TEST_CASE("single-player builder matches frozen pre-extraction goldens",
          "[SinglePlayerChartBuilder][Golden]")
{
    for (const auto& golden : goldens()) {
        DYNAMIC_SECTION(golden.name)
        {
            const auto notesData = makeNotesData();
            const auto originalLaneSounds = laneSounds(notesData.notes);
            const auto chartData = makeChartData(golden.sourceKeymode);

            const auto result = gameplay_logic::buildPlayerChart(
              notesData, chartData, golden.options);

            CHECK(result.effectiveDpOptions == golden.effectiveDpOptions);
            CHECK(result.effectiveKeymode == golden.effectiveKeymode);
            CHECK(result.shuffleResults[0].seed == golden.shuffleSeeds[0]);
            CHECK(result.shuffleResults[1].seed == golden.shuffleSeeds[1]);
            CHECK(result.shuffleResults[0].columns == golden.shuffleColumns[0]);
            CHECK(result.shuffleResults[1].columns == golden.shuffleColumns[1]);
            CHECK(result.storedSeed() == golden.storedSeed);
            CHECK(result.storedPermutation() == golden.storedPermutation);
            CHECK(laneSounds(result.rawNotes) == golden.lanes);
            CHECK(laneSounds(notesData.notes) == originalLaneSounds);

            REQUIRE(result.notes != nullptr);
            REQUIRE(result.state != nullptr);
            CHECK(result.notes->getNotes().size() ==
                  charts::BmsNotesData::columnNumber);
            CHECK(result.state->getColumnStates().size() ==
                  charts::BmsNotesData::columnNumber);
            CHECK(result.state->getBarLinesState()->getBarlines().size() == 2);
            CHECK(noteCount(golden.lanes) ==
                  static_cast<std::size_t>(
                    std::accumulate(result.notes->getNotes().begin(),
                                    result.notes->getNotes().end(),
                                    qsizetype{},
                                    [](const auto count, const auto& lane) {
                                        return count + lane.size();
                                    })));
        }
    }
}

TEST_CASE("desktop coordinator builds Battle score metadata",
          "[SinglePlayerChartBuilder][DesktopCoordinator]")
{
    const auto notesData = makeNotesData();
    const auto chartData = makeChartData(ChartData::Keymode::K7);
    auto pair = resource_managers::buildDesktopPlayerChartPair(
      notesData,
      chartData,
      DesktopPlayerBuildOptions{
        .chart = { .noteOrderP1 = NoteOrderAlgorithm::Random,
                   .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                   .dpOptions = DpOptions::Battle,
                   .randomSeed = 391 },
        .generatedGuid = QStringLiteral("battle-guid"),
      },
      std::nullopt,
      2.0);

    REQUIRE_FALSE(pair.player2.has_value());
    CHECK(pair.player1.chart.effectiveDpOptions == DpOptions::Battle);
    CHECK(pair.player1.chart.effectiveKeymode == ChartData::Keymode::K14);
    CHECK(pair.player1.scoreMetadata.countMultiplier == 2);
    CHECK(pair.player1.scoreMetadata.dpOptions == DpOptions::Battle);
    CHECK(pair.player1.scoreMetadata.keymode == ChartData::Keymode::K14);
    CHECK(pair.player1.scoreMetadata.permutation ==
          pair.player1.chart.storedPermutation());
    CHECK(pair.player1.scoreMetadata.randomSeed ==
          pair.player1.chart.shuffleResults[0].seed);

    REQUIRE(pair.player1.score != nullptr);
    CHECK(pair.player1.score->getNormalNoteCount() == 32);
    CHECK(pair.player1.score->getScratchCount() == 4);
    CHECK(pair.player1.score->getLnCount() == 4);
    CHECK(pair.player1.score->getMineCount() == 2);
    CHECK(pair.player1.score->getMaxHits() == 40);
    CHECK(pair.player1.score->getPermutation() ==
          pair.player1.chart.storedPermutation());
    CHECK(pair.player1.score->getRandomSeed() ==
          pair.player1.chart.shuffleResults[0].seed);
    CHECK(pair.player1.score->getKeymode() == ChartData::Keymode::K14);
    CHECK(pair.player1.score->getNoteOrderAlgorithm() ==
          NoteOrderAlgorithm::Random);
    CHECK(pair.player1.score->getNoteOrderAlgorithmP2() ==
          NoteOrderAlgorithm::Mirror);
}

TEST_CASE("desktop coordinator forces optional player two DP Off",
          "[SinglePlayerChartBuilder][DesktopCoordinator]")
{
    const auto notesData = makeNotesData();
    const auto chartData = makeChartData(ChartData::Keymode::K14);
    auto pair = resource_managers::buildDesktopPlayerChartPair(
      notesData,
      chartData,
      DesktopPlayerBuildOptions{
        .chart = { .noteOrderP1 = NoteOrderAlgorithm::Random,
                   .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                   .dpOptions = DpOptions::Flip,
                   .randomSeed = 391 },
        .generatedGuid = QStringLiteral("p1-guid"),
      },
      DesktopPlayerBuildOptions{
        .chart = { .noteOrderP1 = NoteOrderAlgorithm::Normal,
                   .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                   .dpOptions = DpOptions::Battle,
                   .randomSeed = 947 },
        .generatedGuid = QStringLiteral("p2-guid"),
      },
      2.0);

    REQUIRE(pair.player2.has_value());
    CHECK(pair.player1.chart.effectiveDpOptions == DpOptions::Flip);
    CHECK(pair.player2->chart.effectiveDpOptions == DpOptions::Off);
    CHECK(pair.player2->scoreMetadata.dpOptions == DpOptions::Off);
    CHECK(pair.player2->scoreMetadata.countMultiplier == 1);
    CHECK(pair.player2->scoreMetadata.permutation ==
          pair.player2->chart.storedPermutation());
    CHECK(pair.player2->scoreMetadata.randomSeed ==
          pair.player2->chart.shuffleResults[0].seed);
    REQUIRE(pair.player2->score != nullptr);
    CHECK(pair.player2->score->getNoteOrderAlgorithm() ==
          NoteOrderAlgorithm::Normal);
    CHECK(pair.player2->score->getNoteOrderAlgorithmP2() ==
          NoteOrderAlgorithm::Mirror);
    CHECK(pair.player2->score->getPermutation() ==
          pair.player2->chart.storedPermutation());
    CHECK(pair.player2->score->getRandomSeed() ==
          pair.player2->chart.shuffleResults[0].seed);
}

TEST_CASE("desktop coordinator owns replay autoplay and normal identity",
          "[SinglePlayerChartBuilder][DesktopCoordinator]")
{
    const auto notesData = makeNotesData();
    const auto chartData = makeChartData(ChartData::Keymode::K7);

    SECTION("replay identity wins")
    {
        const auto replay = DesktopScoreIdentity{
            .savedTimestamp = 1'234'567,
            .guid = QStringLiteral("replay-guid"),
            .submissionState =
              gameplay_logic::BmsScore::SubmissionState::Submitted,
        };
        auto pair = resource_managers::buildDesktopPlayerChartPair(
          notesData,
          chartData,
          DesktopPlayerBuildOptions{
            .chart = { .randomSeed = 23 },
            .autoPlay = true,
            .replayIdentity = replay,
            .generatedGuid = QStringLiteral("unused-guid"),
          },
          std::nullopt,
          2.0);
        CHECK(pair.player1.scoreMetadata.identity == replay);
        CHECK(pair.player1.score->getGuid() == QStringLiteral("replay-guid"));
    }

    SECTION("autoplay has an empty non-submitted identity")
    {
        auto pair = resource_managers::buildDesktopPlayerChartPair(
          notesData,
          chartData,
          DesktopPlayerBuildOptions{
            .chart = { .randomSeed = 23 },
            .autoPlay = true,
            .generatedGuid = QStringLiteral("unused-guid"),
          },
          std::nullopt,
          2.0);
        CHECK(pair.player1.scoreMetadata.identity ==
              (DesktopScoreIdentity{
                .savedTimestamp = 0,
                .guid = {},
                .submissionState =
                  gameplay_logic::BmsScore::SubmissionState::NotSubmitted,
              }));
        CHECK(pair.player1.score->getGuid().isEmpty());
    }

    SECTION("ordinary play uses the supplied generated GUID")
    {
        auto pair = resource_managers::buildDesktopPlayerChartPair(
          notesData,
          chartData,
          DesktopPlayerBuildOptions{
            .chart = { .randomSeed = 23 },
            .generatedGuid = QStringLiteral("normal-guid"),
          },
          std::nullopt,
          2.0);
        CHECK(pair.player1.scoreMetadata.identity ==
              (DesktopScoreIdentity{
                .savedTimestamp = 0,
                .guid = QStringLiteral("normal-guid"),
                .submissionState =
                  gameplay_logic::BmsScore::SubmissionState::NotSubmitted,
              }));
        CHECK(pair.player1.score->getGuid() == QStringLiteral("normal-guid"));
    }
}

TEST_CASE("invalid DP combinations retain the production effective mode",
          "[SinglePlayerChartBuilder][DP]")
{
    const auto notesData = makeNotesData();

    SECTION("battle is ignored for an existing DP chart")
    {
        const auto chartData = makeChartData(ChartData::Keymode::K14);
        const auto result = gameplay_logic::buildPlayerChart(
          notesData,
          chartData,
          { .dpOptions = DpOptions::Battle, .randomSeed = 23 });
        CHECK(result.effectiveDpOptions == DpOptions::Off);
        CHECK(result.effectiveKeymode == ChartData::Keymode::K14);
    }

    SECTION("flip is ignored for an SP chart")
    {
        const auto chartData = makeChartData(ChartData::Keymode::K7);
        const auto result = gameplay_logic::buildPlayerChart(
          notesData,
          chartData,
          { .dpOptions = DpOptions::Flip, .randomSeed = 23 });
        CHECK(result.effectiveDpOptions == DpOptions::Off);
        CHECK(result.effectiveKeymode == ChartData::Keymode::K7);
    }
}
