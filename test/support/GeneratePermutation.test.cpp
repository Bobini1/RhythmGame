#include "support/GeneratePermutation.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace {

using Note = charts::BmsNotesData::Note;
using NoteType = charts::BmsNotesData::NoteType;
using resource_managers::NoteOrderAlgorithm;

auto
makeNotes(const int columnCount) -> std::vector<std::vector<Note>>
{
    auto notes = std::vector<std::vector<Note>>{};
    notes.resize(static_cast<std::size_t>(columnCount));
    for (int column = 0; column < columnCount; ++column) {
        notes[static_cast<std::size_t>(column)].push_back({
          .time =
            charts::BmsNotesData::Time{ std::chrono::nanoseconds{ column },
                                        static_cast<double>(column),
                                        static_cast<double>(column) },
          .snap = {},
          .noteType = NoteType::Normal,
          .sound = static_cast<uint16_t>(column),
        });
    }
    return notes;
}

auto
makeMarkerNote(const int64_t timestamp,
               const NoteType noteType,
               const uint16_t sound) -> Note
{
    return {
        .time =
          charts::BmsNotesData::Time{ std::chrono::nanoseconds{ timestamp },
                                      static_cast<double>(timestamp),
                                      static_cast<double>(timestamp) },
        .snap = {},
        .noteType = noteType,
        .sound = sound,
    };
}

auto
columnToLr2Lane(const int column, const int keyCount) -> int
{
    return column == keyCount ? 0 : column + 1;
}

auto
layoutFromColumns(const QList<int>& columns,
                  const int keyCount,
                  const int displayKeyCount) -> int
{
    auto sourceAtDestination = std::vector<int>{};
    sourceAtDestination.resize(static_cast<std::size_t>(displayKeyCount) + 1);
    for (int lane = 1; lane <= displayKeyCount; ++lane) {
        sourceAtDestination[static_cast<std::size_t>(lane)] = lane;
    }

    for (int destColumn = 0; destColumn < columns.size(); ++destColumn) {
        const auto destLane = columnToLr2Lane(destColumn, keyCount);
        if (destLane < 1 || destLane > displayKeyCount) {
            continue;
        }
        const auto sourceLane = columnToLr2Lane(columns[destColumn], keyCount);
        sourceAtDestination[static_cast<std::size_t>(destLane)] = sourceLane;
    }

    auto layout = 0;
    for (int lane = 1; lane <= displayKeyCount; ++lane) {
        layout =
          layout * 10 + sourceAtDestination[static_cast<std::size_t>(lane)];
    }
    return layout;
}

auto
generateLr2Layout(const uint32_t seed, const int keyCount) -> int
{
    auto notes = makeNotes(keyCount + 1);
    auto notesSpan = std::span{ notes };
    auto rng = support::Lr2Random{ seed };
    const auto result = support::generateLr2LanePermutation(
      notesSpan, NoteOrderAlgorithm::Lr2Random, rng);

    for (int destColumn = 0; destColumn < result.columns.size(); ++destColumn) {
        CHECK(notes[static_cast<std::size_t>(destColumn)].front().sound ==
              result.columns[destColumn]);
    }

    return layoutFromColumns(result.columns, keyCount, 7);
}

} // namespace

TEST_CASE("LR2 random matches OpenLR2 7K seed map",
          "[GeneratePermutation][LR2]")
{
    const auto fixtures = std::vector<std::pair<uint32_t, int>>{
        { 391, 1234567 },
        { 7, 1256374 },
        { 3884, 1234576 },
        { 231, 1246375 },
    };

    for (const auto& [seed, expectedLayout] : fixtures) {
        INFO("seed = " << seed);
        CHECK(generateLr2Layout(seed, 7) == expectedLayout);
    }
}

TEST_CASE("LR2 random matches OpenLR2 5K seed map",
          "[GeneratePermutation][LR2]")
{
    const auto fixtures = std::vector<std::pair<uint32_t, int>>{
        { 88, 1234567 },  { 92, 1425367 },    { 19, 3215467 },
        { 524, 3125467 }, { 14394, 1543267 },
    };

    for (const auto& [seed, expectedLayout] : fixtures) {
        INFO("seed = " << seed);
        CHECK(generateLr2Layout(seed, 5) == expectedLayout);
    }
}

TEST_CASE("LR2 random discard matches consumed chart RANDOM draws",
          "[GeneratePermutation][LR2]")
{
    auto consumedRng = support::Lr2Random{ 7 };
    (void)consumedRng.getRand(3);
    (void)consumedRng.getRand(1);

    auto consumedNotes = makeNotes(8);
    auto consumedSpan = std::span{ consumedNotes };
    const auto consumedResult = support::generateLr2LanePermutation(
      consumedSpan, NoteOrderAlgorithm::Lr2Random, consumedRng);

    auto discardedRng = support::Lr2Random{ 7 };
    discardedRng.discard(2);

    auto discardedNotes = makeNotes(8);
    auto discardedSpan = std::span{ discardedNotes };
    const auto discardedResult = support::generateLr2LanePermutation(
      discardedSpan, NoteOrderAlgorithm::Lr2Random, discardedRng);

    CHECK(discardedResult.seed == uint64_t{ 7 });
    CHECK(discardedResult.columns == consumedResult.columns);

    auto zeroRangeRng = support::Lr2Random{ 7 };
    CHECK(zeroRangeRng.getRand(-1) == 0);

    auto oneDiscardRng = support::Lr2Random{ 7 };
    oneDiscardRng.discard(1);

    auto zeroRangeNotes = makeNotes(8);
    auto zeroRangeSpan = std::span{ zeroRangeNotes };
    const auto zeroRangeResult = support::generateLr2LanePermutation(
      zeroRangeSpan, NoteOrderAlgorithm::Lr2Random, zeroRangeRng);

    auto oneDiscardNotes = makeNotes(8);
    auto oneDiscardSpan = std::span{ oneDiscardNotes };
    const auto oneDiscardResult = support::generateLr2LanePermutation(
      oneDiscardSpan, NoteOrderAlgorithm::Lr2Random, oneDiscardRng);

    CHECK(zeroRangeResult.columns == oneDiscardResult.columns);
}

TEST_CASE("LR2 random keeps long-note endpoints together",
          "[GeneratePermutation][LR2][LN]")
{
    auto notes = makeNotes(8);
    static constexpr auto beginSound = uint16_t{ 900 };
    static constexpr auto endSound = uint16_t{ 901 };
    notes[4].push_back(
      makeMarkerNote(100, NoteType::LongNoteBegin, beginSound));
    notes[4].push_back(makeMarkerNote(200, NoteType::LongNoteEnd, endSound));

    auto notesSpan = std::span{ notes };
    auto rng = support::Lr2Random{ 7 };
    (void)support::generateLr2LanePermutation(
      notesSpan, NoteOrderAlgorithm::Lr2Random, rng);

    auto beginColumn = std::optional<std::size_t>{};
    auto endColumn = std::optional<std::size_t>{};
    auto beginPosition = std::optional<std::size_t>{};
    auto endPosition = std::optional<std::size_t>{};
    for (auto column = std::size_t{ 0 }; column < notes.size(); ++column) {
        for (auto position = std::size_t{ 0 }; position < notes[column].size();
             ++position) {
            const auto& note = notes[column][position];
            if (note.sound == beginSound) {
                beginColumn = column;
                beginPosition = position;
                CHECK(note.noteType == NoteType::LongNoteBegin);
            } else if (note.sound == endSound) {
                endColumn = column;
                endPosition = position;
                CHECK(note.noteType == NoteType::LongNoteEnd);
            }
        }
    }

    REQUIRE(beginColumn.has_value());
    REQUIRE(endColumn.has_value());
    REQUIRE(beginPosition.has_value());
    REQUIRE(endPosition.has_value());
    CHECK(beginColumn == endColumn);
    CHECK(*beginPosition < *endPosition);
}

TEST_CASE("beatoraja DP flip mirrors playfields", "[GeneratePermutation][DP]")
{
    auto notes =
      std::array<std::vector<Note>, charts::BmsNotesData::columnNumber>{};
    for (auto column = std::size_t{ 0 };
         column < charts::BmsNotesData::columnNumber;
         ++column) {
        notes[column].push_back({
          .time = charts::BmsNotesData::Time{ std::chrono::nanoseconds{
                                                static_cast<int64_t>(column) },
                                              static_cast<double>(column),
                                              static_cast<double>(column) },
          .snap = {},
          .noteType = NoteType::Normal,
          .sound = static_cast<uint16_t>(column),
        });
    }

    support::flipBeatorajaDpPlayfields(notes);

    for (auto column = std::size_t{ 0 }; column < 7; ++column) {
        CHECK(notes[column].front().sound == 14 - column);
        CHECK(notes[14 - column].front().sound == column);
    }
    CHECK(notes[7].front().sound == 15);
    CHECK(notes[15].front().sound == 7);
}

TEST_CASE("beatoraja DP flip keeps long-note endpoints together",
          "[GeneratePermutation][DP][LN]")
{
    auto notes =
      std::array<std::vector<Note>, charts::BmsNotesData::columnNumber>{};
    static constexpr auto beginSound = uint16_t{ 910 };
    static constexpr auto endSound = uint16_t{ 911 };
    notes[3].push_back(
      makeMarkerNote(100, NoteType::LongNoteBegin, beginSound));
    notes[3].push_back(makeMarkerNote(200, NoteType::LongNoteEnd, endSound));

    support::flipBeatorajaDpPlayfields(notes);

    REQUIRE(notes[11].size() == 2);
    CHECK(notes[11][0].sound == beginSound);
    CHECK(notes[11][0].noteType == NoteType::LongNoteBegin);
    CHECK(notes[11][1].sound == endSound);
    CHECK(notes[11][1].noteType == NoteType::LongNoteEnd);
}

TEST_CASE("LR2 DP flip swaps sides without mirroring lane order",
          "[GeneratePermutation][DP]")
{
    auto notes =
      std::array<std::vector<Note>, charts::BmsNotesData::columnNumber>{};
    for (auto column = std::size_t{ 0 };
         column < charts::BmsNotesData::columnNumber;
         ++column) {
        notes[column].push_back({
          .time = charts::BmsNotesData::Time{ std::chrono::nanoseconds{
                                                static_cast<int64_t>(column) },
                                              static_cast<double>(column),
                                              static_cast<double>(column) },
          .snap = {},
          .noteType = NoteType::Normal,
          .sound = static_cast<uint16_t>(column),
        });
    }

    support::flipLr2DpPlayfields(notes);

    static constexpr auto sideColumnCount =
      charts::BmsNotesData::columnNumber / 2;
    for (auto column = std::size_t{ 0 }; column < sideColumnCount; ++column) {
        CHECK(notes[column].front().sound == column + sideColumnCount);
        CHECK(notes[column + sideColumnCount].front().sound == column);
    }
}

TEST_CASE("LR2 DP flip keeps long-note endpoints together",
          "[GeneratePermutation][DP][LN]")
{
    auto notes =
      std::array<std::vector<Note>, charts::BmsNotesData::columnNumber>{};
    static constexpr auto beginSound = uint16_t{ 920 };
    static constexpr auto endSound = uint16_t{ 921 };
    notes[7].push_back(
      makeMarkerNote(100, NoteType::LongNoteBegin, beginSound));
    notes[7].push_back(makeMarkerNote(200, NoteType::LongNoteEnd, endSound));

    support::flipLr2DpPlayfields(notes);

    REQUIRE(notes[15].size() == 2);
    CHECK(notes[15][0].sound == beginSound);
    CHECK(notes[15][0].noteType == NoteType::LongNoteBegin);
    CHECK(notes[15][1].sound == endSound);
    CHECK(notes[15][1].noteType == NoteType::LongNoteEnd);
}
