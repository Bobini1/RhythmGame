#include "gameplay_logic/SinglePlayerChartBuilder.h"

#include "support/GeneratePermutation.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {

using gameplay_logic::ChartData;
using gameplay_logic::PlayerChartBuildOptions;
using resource_managers::DpOptions;
using resource_managers::NoteOrderAlgorithm;
using Note = charts::BmsNotesData::Note;
using NoteType = charts::BmsNotesData::NoteType;

struct ExpectedBuild
{
    std::array<std::vector<Note>, charts::BmsNotesData::columnNumber> rawNotes;
    std::array<support::ShuffleResult, 2> shuffleResults;
    DpOptions dpOptions;
    ChartData::Keymode keymode;
};

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
applyExpectedOrder(std::span<std::vector<Note>>& notes,
                   const NoteOrderAlgorithm algorithm,
                   const std::uint64_t seed,
                   const bool k5,
                   const bool usePre130,
                   support::Lr2Random* lr2Random) -> support::ShuffleResult
{
    auto original = notes;
    auto working = notes;
    if (k5 && (support::isBeatorajaNoteOrderAlgorithm(algorithm) ||
               support::isLr2NoteOrderAlgorithm(algorithm))) {
        notes[5].swap(notes[7]);
        working = notes.subspan(0, 6);
    }

    auto result = support::ShuffleResult{};
    if (support::isBeatorajaNoteOrderAlgorithm(algorithm)) {
        result = support::generateBeatorajaLanePermutation(
          working, algorithm, static_cast<std::int64_t>(seed));
    } else if (support::isLr2NoteOrderAlgorithm(algorithm)) {
        auto local = support::Lr2Random{ static_cast<std::uint32_t>(seed) };
        result = support::generateLr2LanePermutation(
          working, algorithm, lr2Random != nullptr ? *lr2Random : local);
    } else {
        result =
          support::generatePermutation(notes, algorithm, seed, k5, usePre130);
    }

    if (k5 && (support::isBeatorajaNoteOrderAlgorithm(algorithm) ||
               support::isLr2NoteOrderAlgorithm(algorithm))) {
        notes[5].swap(notes[7]);
        notes = original;
    }
    return result;
}

auto
expectedBuild(const charts::BmsNotesData& notesData,
              const ChartData& chartData,
              const PlayerChartBuildOptions& options) -> ExpectedBuild
{
    auto visibleNotes = notesData.notes;
    auto dpOptions = options.dpOptions;
    const auto isDpFlip =
      dpOptions == DpOptions::Flip || dpOptions == DpOptions::Lr2Flip;
    if ((dpOptions == DpOptions::Battle &&
         gameplay_logic::isDp(chartData.getKeymode())) ||
        (isDpFlip && !gameplay_logic::isDp(chartData.getKeymode()))) {
        dpOptions = DpOptions::Off;
    }

    auto keymode = chartData.getKeymode();
    if (dpOptions == DpOptions::Battle) {
        keymode = keymode == ChartData::Keymode::K5 ? ChartData::Keymode::K10
                                                    : ChartData::Keymode::K14;
    } else if (dpOptions == DpOptions::Flip) {
        support::flipBeatorajaDpPlayfields(visibleNotes);
    } else if (dpOptions == DpOptions::Lr2Flip) {
        support::flipLr2DpPlayfields(visibleNotes);
    }
    if (dpOptions == DpOptions::Battle) {
        for (auto column = 0; column < 7; ++column) {
            visibleNotes[14 - column] = visibleNotes[column];
        }
        visibleNotes[15] = visibleNotes[7];
    }

    const auto randomIs5k =
      !options.usePre130 &&
      (keymode == ChartData::Keymode::K5 || keymode == ChartData::Keymode::K10);
    auto lr2Random =
      support::isLr2NoteOrderAlgorithm(options.noteOrderP1) ||
          support::isLr2NoteOrderAlgorithm(options.noteOrderP2)
        ? std::optional<support::Lr2Random>{ static_cast<std::uint32_t>(
            options.randomSeed) }
        : std::nullopt;
    if (lr2Random) {
        lr2Random->discard(
          static_cast<std::size_t>(chartData.getRandomSequence().size()));
    }
    auto* lr2RandomPtr = lr2Random ? &*lr2Random : nullptr;

    auto first = std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
    auto firstResult = applyExpectedOrder(first,
                                          options.noteOrderP1,
                                          options.randomSeed,
                                          randomIs5k,
                                          options.usePre130,
                                          lr2RandomPtr);
    auto secondResult = support::ShuffleResult{};
    if (gameplay_logic::isDp(keymode)) {
        auto second = std::span{ visibleNotes.data() + visibleNotes.size() / 2,
                                 visibleNotes.size() / 2 };
        secondResult = applyExpectedOrder(second,
                                          options.noteOrderP2,
                                          firstResult.seed + 1,
                                          randomIs5k,
                                          options.usePre130,
                                          lr2RandomPtr);
    }

    return { std::move(visibleNotes),
             { std::move(firstResult), std::move(secondResult) },
             dpOptions,
             keymode };
}

auto
noteCount(const std::array<std::vector<Note>,
                           charts::BmsNotesData::columnNumber>& notes)
  -> std::size_t
{
    return std::accumulate(
      notes.begin(),
      notes.end(),
      std::size_t{},
      [](auto count, const auto& lane) { return count + lane.size(); });
}

struct BuildCase
{
    std::string_view name;
    ChartData::Keymode keymode;
    PlayerChartBuildOptions options;
};

} // namespace

TEST_CASE("single-player chart builder preserves production lane fixtures",
          "[SinglePlayerChartBuilder]")
{
    const auto cases = std::array{
        BuildCase{ "7K normal", ChartData::Keymode::K7, { .randomSeed = 391 } },
        BuildCase{
          "7K mirror",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::Mirror, .randomSeed = 391 } },
        BuildCase{
          "7K seeded random",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::Random, .randomSeed = 391 } },
        BuildCase{
          "7K seeded S-random",
          ChartData::Keymode::K7,
          { .noteOrderP1 = NoteOrderAlgorithm::SRandom, .randomSeed = 391 } },
        BuildCase{ "14K DP flip",
                   ChartData::Keymode::K14,
                   { .noteOrderP1 = NoteOrderAlgorithm::Mirror,
                     .noteOrderP2 = NoteOrderAlgorithm::Random,
                     .dpOptions = DpOptions::Flip,
                     .randomSeed = 391 } },
        BuildCase{ "14K LR2 flip with shared RNG",
                   ChartData::Keymode::K14,
                   { .noteOrderP1 = NoteOrderAlgorithm::Lr2Random,
                     .noteOrderP2 = NoteOrderAlgorithm::Lr2Random,
                     .dpOptions = DpOptions::Lr2Flip,
                     .randomSeed = 7 } },
        BuildCase{ "7K battle",
                   ChartData::Keymode::K7,
                   { .noteOrderP1 = NoteOrderAlgorithm::Random,
                     .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                     .dpOptions = DpOptions::Battle,
                     .randomSeed = 391 } },
        BuildCase{ "legacy pre-1.3.0 5K replay",
                   ChartData::Keymode::K5,
                   { .noteOrderP1 = NoteOrderAlgorithm::Random,
                     .randomSeed = 391,
                     .usePre130 = true } },
        BuildCase{ "second player forced DP Off",
                   ChartData::Keymode::K14,
                   { .noteOrderP1 = NoteOrderAlgorithm::Random,
                     .noteOrderP2 = NoteOrderAlgorithm::Mirror,
                     .dpOptions = DpOptions::Off,
                     .randomSeed = 947 } },
    };

    for (const auto& fixture : cases) {
        DYNAMIC_SECTION(fixture.name)
        {
            const auto notesData = makeNotesData();
            const auto chartData = makeChartData(fixture.keymode);
            const auto expected =
              expectedBuild(notesData, chartData, fixture.options);

            const auto result = gameplay_logic::buildPlayerChart(
              notesData, chartData, fixture.options);

            CHECK(result.effectiveDpOptions == expected.dpOptions);
            CHECK(result.effectiveKeymode == expected.keymode);
            CHECK(result.shuffleResults[0].seed ==
                  expected.shuffleResults[0].seed);
            CHECK(result.shuffleResults[0].columns ==
                  expected.shuffleResults[0].columns);
            CHECK(result.shuffleResults[1].seed ==
                  expected.shuffleResults[1].seed);
            CHECK(result.shuffleResults[1].columns ==
                  expected.shuffleResults[1].columns);
            CHECK(result.storedSeed() == expected.shuffleResults[0].seed);
            CHECK(result.storedPermutation() ==
                  expected.shuffleResults[0].columns +
                    expected.shuffleResults[1].columns);
            for (auto column = std::size_t{}; column < result.rawNotes.size();
                 ++column) {
                REQUIRE(result.rawNotes[column].size() ==
                        expected.rawNotes[column].size());
                for (auto note = std::size_t{};
                     note < result.rawNotes[column].size();
                     ++note) {
                    const auto& actualNote = result.rawNotes[column][note];
                    const auto& expectedNote = expected.rawNotes[column][note];
                    CHECK(actualNote.time == expectedNote.time);
                    CHECK(actualNote.snap.numerator ==
                          expectedNote.snap.numerator);
                    CHECK(actualNote.snap.denominator ==
                          expectedNote.snap.denominator);
                    CHECK(actualNote.noteType == expectedNote.noteType);
                    CHECK(actualNote.sound == expectedNote.sound);
                }
            }

            REQUIRE(result.notes != nullptr);
            REQUIRE(result.state != nullptr);
            CHECK(result.notes->getNotes().size() ==
                  charts::BmsNotesData::columnNumber);
            CHECK(result.state->getColumnStates().size() ==
                  charts::BmsNotesData::columnNumber);
            CHECK(result.state->getBarLinesState()->getBarlines().size() == 2);
            CHECK(noteCount(result.rawNotes) ==
                  static_cast<std::size_t>(
                    std::accumulate(result.notes->getNotes().begin(),
                                    result.notes->getNotes().end(),
                                    qsizetype{},
                                    [](const auto count, const auto& lane) {
                                        return count + lane.size();
                                    })));

            CHECK(notesData.bgmNotes.front().second == 900);
            CHECK(notesData.bpmChanges[1].bpm == 225.0);
            CHECK(notesData.bpmChanges[1].scroll == 0.5);
            CHECK(notesData.barLines[1].timestamp ==
                  std::chrono::nanoseconds{ 16'000 });
        }
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
