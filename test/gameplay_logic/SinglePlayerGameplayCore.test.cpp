#include "gameplay_logic/SinglePlayerGameplayCore.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QFile>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using namespace std::chrono_literals;
using Catch::Approx;
using gameplay_logic::GameplayKeyAction;
using gameplay_logic::SinglePlayerGameplayCore;

constexpr auto fixturePath =
  RHYTHMGAME_SOURCE_DIR "/testOnlyAssets/webPlaytest/core-fixture.bms";

class RecordingSound final : public sounds::Sound
{
  public:
    std::vector<std::chrono::nanoseconds> playTimes;
    std::vector<std::chrono::nanoseconds> stopTimes;
    float volume{ 1.0F };

    void play() override
    {
        playTimes.push_back(std::chrono::nanoseconds::min());
    }
    void playAt(const std::chrono::nanoseconds time) override
    {
        playTimes.push_back(time);
    }
    void stop() override
    {
        stopTimes.push_back(std::chrono::nanoseconds::min());
    }
    void stopAt(const std::chrono::nanoseconds time) override
    {
        stopTimes.push_back(time);
    }
    void setVolume(const float newVolume) override { volume = newVolume; }
    auto isPlaying() const -> bool override { return false; }
    auto getVolume() const -> float override { return volume; }
};

struct SoundBank
{
    std::shared_ptr<RecordingSound> bgm = std::make_shared<RecordingSound>();
    std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>> sounds;

    SoundBank()
    {
        sounds.emplace(0, std::make_shared<RecordingSound>());
        sounds.emplace(1, bgm);
        for (auto id = std::uint64_t{ 2 }; id <= 5; ++id) {
            sounds.emplace(id, std::make_shared<RecordingSound>());
        }
    }
};

auto
fixtureBytes() -> QByteArray
{
    auto file = QFile{ QString::fromUtf8(fixturePath) };
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Unable to open gameplay core fixture");
    }
    return file.readAll();
}

auto
makeConfig() -> gameplay_logic::GameplayCoreConfig
{
    return {
        .play = { .randomSequence = { 2 },
                  .noteOrderP1 = resource_managers::NoteOrderAlgorithm::Normal,
                  .noteOrderP2 = resource_managers::NoteOrderAlgorithm::Normal,
                  .dpMode = resource_managers::DpOptions::Off,
                  .laneSeed = 91 },
        .savedTimestampSeconds = 1'700'000'123,
        .scoreGuid = QStringLiteral("fixed-core-guid"),
        .maxHitValue = 2.0,
    };
}

auto
makeCore(SoundBank* bank = nullptr) -> std::unique_ptr<SinglePlayerGameplayCore>
{
    const auto bytes = fixtureBytes();
    return SinglePlayerGameplayCore::create(
      std::string_view{ bytes.constData(),
                        static_cast<std::size_t>(bytes.size()) },
      std::filesystem::path{ "memory/core-fixture.bms" },
      makeConfig(),
      bank
        ? bank->sounds
        : std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>>{});
}

void
runCanonicalInputs(SinglePlayerGameplayCore& core)
{
    core.advanceTo(-1s);
    core.passKey(input::BmsKey::Col11, GameplayKeyAction::Press, 2s);
    core.passKey(input::BmsKey::Col12, GameplayKeyAction::Press, 2030ms);
    core.passKey(input::BmsKey::Col12, GameplayKeyAction::Release, 2100ms);
    core.passKey(input::BmsKey::Col13, GameplayKeyAction::Press, 2450ms);
    core.advanceTo(2500ms);
    core.passKey(input::BmsKey::Col13, GameplayKeyAction::Release, 2600ms);
    core.passKey(input::BmsKey::Col1sUp, GameplayKeyAction::Press, 2970ms);
    core.passKey(input::BmsKey::Col1sUp, GameplayKeyAction::Release, 3100ms);
    core.passKey(input::BmsKey::Col11, GameplayKeyAction::Release, 3500ms);
    core.advanceTo(4s);
    core.advanceTo(5100ms);
    core.advanceTo(10s);
}

auto
noteById(const gameplay_logic::GameplaySnapshot& snapshot,
         const std::uint32_t id)
  -> const gameplay_logic::GameplaySnapshot::VisibleNote&
{
    const auto note =
      std::ranges::find(snapshot.visibleNotes,
                        id,
                        &decltype(snapshot.visibleNotes)::value_type::stableId);
    if (note == snapshot.visibleNotes.end()) {
        throw std::runtime_error("Expected visible note is missing");
    }
    return *note;
}

TEST_CASE("gameplay core exposes exact immutable production snapshots",
          "[SinglePlayerGameplayCore][Snapshot]")
{
    auto core = makeCore();

    core->advanceTo(-1s);
    const auto countdown = core->snapshot();
    CHECK(countdown.chartTimeNs == -1'000'000'000);
    CHECK(countdown.beatPosition == Approx(-2.0));
    CHECK(countdown.scrollPosition == Approx(-2.0));
    CHECK(countdown.points == 0.0);
    CHECK(countdown.maxPointsNow == 0.0);
    CHECK(countdown.gauge == 20.0);
    CHECK(countdown.combo == 0);
    CHECK(countdown.maxCombo == 0);
    CHECK(countdown.mineHits == 0);
    CHECK_FALSE(countdown.latestJudgement);
    CHECK_FALSE(countdown.latestDeviationNs);
    CHECK(std::ranges::none_of(countdown.pressedColumns,
                               [](const bool value) { return value; }));
    REQUIRE(countdown.visibleNotes.size() == 6);
    CHECK(noteById(countdown, 0).scrollPosition == Approx(4.0));
    REQUIRE(noteById(countdown, 0).pairedScrollPosition);
    CHECK(*noteById(countdown, 0).pairedScrollPosition == Approx(7.0));
    CHECK(noteById(countdown, 1).scrollPosition == Approx(7.0));
    REQUIRE(noteById(countdown, 1).pairedScrollPosition);
    CHECK(*noteById(countdown, 1).pairedScrollPosition == Approx(4.0));
    CHECK_FALSE(noteById(countdown, 65'536).pairedScrollPosition);
    CHECK(noteById(countdown, 65'537).beatPosition == Approx(10.0));
    CHECK(noteById(countdown, 65'537).scrollPosition == Approx(9.5));
    CHECK(countdown.visibleNotes ==
          std::vector<gameplay_logic::GameplaySnapshot::VisibleNote>{
            { 0,
              0,
              charts::BmsNotesData::NoteType::LongNoteBegin,
              2'000'000'000,
              4.0,
              4.0,
              7.0,
              false,
              false },
            { 65'536,
              1,
              charts::BmsNotesData::NoteType::Normal,
              2'000'000'000,
              4.0,
              4.0,
              std::nullopt,
              false,
              false },
            { 131'072,
              2,
              charts::BmsNotesData::NoteType::Landmine,
              2'500'000'000,
              5.0,
              5.0,
              std::nullopt,
              false,
              false },
            { 458'752,
              7,
              charts::BmsNotesData::NoteType::Normal,
              3'000'000'000,
              6.0,
              6.0,
              std::nullopt,
              false,
              false },
            { 1,
              0,
              charts::BmsNotesData::NoteType::LongNoteEnd,
              3'500'000'000,
              7.0,
              7.0,
              4.0,
              false,
              false },
            { 65'537,
              1,
              charts::BmsNotesData::NoteType::Normal,
              4'833'333'333,
              10.0,
              9.5,
              std::nullopt,
              false,
              false },
          });

    core->passKey(input::BmsKey::Col11, GameplayKeyAction::Press, 2s);
    core->passKey(input::BmsKey::Col12, GameplayKeyAction::Press, 2030ms);
    const auto firstNote = core->snapshot();
    CHECK(firstNote.chartTimeNs == 2'030'000'000);
    CHECK(firstNote.beatPosition == Approx(4.06));
    CHECK(firstNote.scrollPosition == Approx(4.06));
    CHECK(firstNote.points == 1.0);
    CHECK(firstNote.maxPointsNow == 2.0);
    CHECK(firstNote.gauge == 95.0);
    CHECK(firstNote.combo == 1);
    CHECK(firstNote.maxCombo == 1);
    CHECK(firstNote.latestJudgement == gameplay_logic::Judgement::Great);
    CHECK(firstNote.latestDeviationNs == 30'000'000);
    CHECK(firstNote.pressedColumns[0]);
    CHECK(firstNote.pressedColumns[1]);
    CHECK(noteById(firstNote, 0).removed);
    CHECK(noteById(firstNote, 0).holding);
    CHECK(noteById(firstNote, 1).holding);
    CHECK(noteById(firstNote, 65'536).removed);

    core->passKey(input::BmsKey::Col12, GameplayKeyAction::Release, 2100ms);
    core->passKey(input::BmsKey::Col13, GameplayKeyAction::Press, 2450ms);
    core->advanceTo(2500ms);
    core->passKey(input::BmsKey::Col13, GameplayKeyAction::Release, 2600ms);
    core->passKey(input::BmsKey::Col1sUp, GameplayKeyAction::Press, 2970ms);
    const auto longNoteHold = core->snapshot();
    CHECK(longNoteHold.chartTimeNs == 2'970'000'000);
    CHECK(longNoteHold.points == 2.0);
    CHECK(longNoteHold.maxPointsNow == 4.0);
    CHECK(longNoteHold.gauge == 100.0);
    CHECK(longNoteHold.combo == 2);
    CHECK(longNoteHold.mineHits == 1);
    CHECK(longNoteHold.latestJudgement == gameplay_logic::Judgement::Great);
    CHECK(longNoteHold.latestDeviationNs == -30'000'000);
    CHECK(longNoteHold.pressedColumns[0]);
    CHECK_FALSE(longNoteHold.pressedColumns[1]);
    CHECK_FALSE(longNoteHold.pressedColumns[2]);
    CHECK(longNoteHold.pressedColumns[7]);
    CHECK(noteById(longNoteHold, 131'072).removed);
    CHECK(noteById(longNoteHold, 458'752).removed);
    CHECK(noteById(longNoteHold, 0).holding);
    CHECK(noteById(longNoteHold, 1).holding);

    core->passKey(input::BmsKey::Col1sUp, GameplayKeyAction::Release, 3100ms);
    core->passKey(input::BmsKey::Col11, GameplayKeyAction::Release, 3500ms);
    const auto longNoteRelease = core->snapshot();
    CHECK(longNoteRelease.points == 4.0);
    CHECK(longNoteRelease.maxPointsNow == 6.0);
    CHECK(longNoteRelease.combo == 3);
    CHECK(longNoteRelease.maxCombo == 3);
    CHECK(longNoteRelease.latestJudgement ==
          gameplay_logic::Judgement::Perfect);
    CHECK(longNoteRelease.latestDeviationNs == 0);
    CHECK_FALSE(noteById(longNoteRelease, 1).holding);
    CHECK(noteById(longNoteRelease, 1).removed);

    core->advanceTo(4s);
    const auto bpmChange = core->snapshot();
    CHECK(bpmChange.beatPosition == Approx(8.0));
    CHECK(bpmChange.scrollPosition == Approx(8.0));
    CHECK_FALSE(bpmChange.finished);

    core->advanceTo(5100ms);
    const auto miss = core->snapshot();
    CHECK(miss.beatPosition == Approx(10.0));
    CHECK(miss.scrollPosition == Approx(9.5));
    CHECK(miss.gauge == 94.0);
    CHECK(miss.combo == 0);
    CHECK(miss.maxCombo == 3);
    CHECK(miss.latestJudgement == gameplay_logic::Judgement::Poor);
    CHECK(miss.latestDeviationNs == 266'666'667);
    REQUIRE(miss.visibleNotes.size() == 1);
    CHECK(miss.visibleNotes.front().stableId == 65'537);
    CHECK(miss.visibleNotes.front().removed);

    core->advanceTo(10s);
    const auto result = core->snapshot();
    CHECK(result.finished);
    CHECK(result.points == 4.0);
    CHECK(result.maxPointsNow == 8.0);
    CHECK(result.gauge == 94.0);
    CHECK(result.mineHits == 1);
    CHECK(result.visibleNotes.empty());

    CHECK(countdown.visibleNotes.front().removed == false);
    CHECK(countdown.visibleNotes.front().holding == false);
}

TEST_CASE("gameplay core BGM pre-scheduling is exactly once",
          "[SinglePlayerGameplayCore][ScheduledSound]")
{
    auto bank = SoundBank{};
    auto core = makeCore(&bank);

    core->preScheduleBgm();
    core->preScheduleBgm();
    runCanonicalInputs(*core);

    CHECK(bank.bgm->playTimes == std::vector<std::chrono::nanoseconds>{ 2s });
}

TEST_CASE("gameplay snapshot marks only the active same-lane LN pair holding",
          "[SinglePlayerGameplayCore][Snapshot][LongNote]")
{
    constexpr auto twoLongNotes =
      std::string_view{ "#TITLE Pair-specific holding\n"
                        "#BPM 120\n"
                        "#LNTYPE 1\n"
                        "#WAV01 long.ogg\n"
                        "#00151:0101\n"
                        "#00251:0101\n" };
    auto config = makeConfig();
    config.play.randomSequence.clear();
    auto core = SinglePlayerGameplayCore::create(
      twoLongNotes,
      std::filesystem::path{ "memory/two-long-notes.bms" },
      config,
      {});

    core->passKey(input::BmsKey::Col11, GameplayKeyAction::Press, 2s);
    const auto snapshot = core->snapshot();
    auto longNotes =
      snapshot.visibleNotes | std::views::filter([](const auto& note) {
          return note.type == charts::BmsNotesData::NoteType::LongNoteBegin ||
                 note.type == charts::BmsNotesData::NoteType::LongNoteEnd;
      }) |
      std::ranges::to<std::vector>();

    REQUIRE(longNotes.size() == 4);
    CHECK(longNotes[0].holding);
    CHECK(longNotes[1].holding);
    CHECK_FALSE(longNotes[2].holding);
    CHECK_FALSE(longNotes[3].holding);
}

TEST_CASE("gameplay snapshot keeps an LN body that crosses the render window",
          "[SinglePlayerGameplayCore][Snapshot][LongNote][Scroll]")
{
    constexpr auto crossingLongNote =
      std::string_view{ "#TITLE Crossing long note\n"
                        "#BPM 120\n"
                        "#LNTYPE 1\n"
                        "#WAV01 long.ogg\n"
                        "#00151:01\n"
                        "#01151:01\n" };
    auto config = makeConfig();
    config.play.randomSequence.clear();
    auto core = SinglePlayerGameplayCore::create(
      crossingLongNote,
      std::filesystem::path{ "memory/crossing-long-note.bms" },
      config,
      {});

    core->advanceTo(10s);
    const auto snapshot = core->snapshot();

    CHECK(snapshot.scrollPosition == Approx(20.0));
    REQUIRE(snapshot.visibleNotes.size() == 2);
    CHECK(snapshot.visibleNotes[0].type ==
          charts::BmsNotesData::NoteType::LongNoteBegin);
    CHECK(snapshot.visibleNotes[0].scrollPosition == Approx(4.0));
    REQUIRE(snapshot.visibleNotes[0].pairedScrollPosition);
    CHECK(*snapshot.visibleNotes[0].pairedScrollPosition == Approx(44.0));
    CHECK(snapshot.visibleNotes[1].type ==
          charts::BmsNotesData::NoteType::LongNoteEnd);
    CHECK(snapshot.visibleNotes[1].scrollPosition == Approx(44.0));
    REQUIRE(snapshot.visibleNotes[1].pairedScrollPosition);
    CHECK(*snapshot.visibleNotes[1].pairedScrollPosition == Approx(4.0));
}

TEST_CASE("gameplay snapshot storage can be reserved and reused",
          "[SinglePlayerGameplayCore][Snapshot][Storage]")
{
    auto core = makeCore();
    auto buffers = std::array<gameplay_logic::GameplaySnapshot, 3>{};

    CHECK(core->snapshotVisibleNoteCapacity() == 6);
    for (auto& buffer : buffers) {
        core->reserveSnapshot(buffer);
        REQUIRE(buffer.visibleNotes.capacity() >=
                core->snapshotVisibleNoteCapacity());
    }

    core->advanceTo(-1s);
    for (auto& buffer : buffers) {
        const auto* const storage = buffer.visibleNotes.data();
        const auto capacity = buffer.visibleNotes.capacity();
        core->fillSnapshot(buffer);
        CHECK(buffer.visibleNotes.size() == 6);
        CHECK(buffer.visibleNotes.data() == storage);
        CHECK(buffer.visibleNotes.capacity() == capacity);
    }

    const auto* const storage = buffers[0].visibleNotes.data();
    const auto capacity = buffers[0].visibleNotes.capacity();
    core->advanceTo(10s);
    core->fillSnapshot(buffers[0]);
    CHECK(buffers[0].finished);
    CHECK(buffers[0].visibleNotes.empty());
    CHECK(buffers[0].visibleNotes.data() == storage);
    CHECK(buffers[0].visibleNotes.capacity() == capacity);
}

TEST_CASE("gameplay core rejects nondeterminism and non-monotonic input",
          "[SinglePlayerGameplayCore][Determinism]")
{
    constexpr auto minimalChart =
      std::string_view{ "#TITLE Core monotonic test\n"
                        "#BPM 120\n"
                        "#00111:0001\n" };
    auto config = makeConfig();
    config.play.randomSequence.clear();
    auto core = SinglePlayerGameplayCore::create(
      minimalChart,
      std::filesystem::path{ "memory/core-monotonic-test.bms" },
      config,
      {});
    core->advanceTo(2s);
    REQUIRE_THROWS_AS(
      core->passKey(input::BmsKey::Col11, GameplayKeyAction::Press, 1999ms),
      std::invalid_argument);

    config.savedTimestampSeconds = 0;
    REQUIRE_THROWS_AS(SinglePlayerGameplayCore::create(
                        minimalChart,
                        std::filesystem::path{ "memory/core-missing-time.bms" },
                        config,
                        {}),
                      std::invalid_argument);
    config.savedTimestampSeconds = 1;
    config.scoreGuid.clear();
    REQUIRE_THROWS_AS(SinglePlayerGameplayCore::create(
                        minimalChart,
                        std::filesystem::path{ "memory/core-missing-guid.bms" },
                        config,
                        {}),
                      std::invalid_argument);
}

TEST_CASE("gameplay core emits a byte-identical canonical trace",
          "[SinglePlayerGameplayCore][Trace]")
{
    auto first = makeCore();
    auto second = makeCore();
    runCanonicalInputs(*first);
    runCanonicalInputs(*second);

    const auto expected = QByteArray{
        R"JSON({"schemaVersion":1,"chart":{"sha256":"A9B1398DE783D07F35D068FE681308023AA49398D94F4B05751EA5155C20A220","md5":"0A767FC88B7BFB1C2023C1C304CACC49"},"play":{"randomSequence":[2],"permutation":[0,1,2,3,4,5,6,5],"laneSeed":91,"noteOrderP1":0,"noteOrderP2":0,"dpMode":0},"inputs":[{"timeNs":2000000000,"key":0,"action":"press"},{"timeNs":2030000000,"key":1,"action":"press"},{"timeNs":2100000000,"key":1,"action":"release"},{"timeNs":2450000000,"key":2,"action":"press"},{"timeNs":2600000000,"key":2,"action":"release"},{"timeNs":2970000000,"key":7,"action":"press"},{"timeNs":3100000000,"key":7,"action":"release"},{"timeNs":3500000000,"key":0,"action":"release"}],"judgements":[{"timeNs":2000000000,"hitOffsetNs":2000000000,"column":0,"key":0,"noteIndex":0,"action":"press","noteRemoved":true,"judgement":9,"deviationNs":0,"value":0},{"timeNs":2030000000,"hitOffsetNs":2060000000,"column":1,"key":1,"noteIndex":0,"action":"press","noteRemoved":true,"judgement":4,"deviationNs":30000000,"value":1},{"timeNs":2100000000,"hitOffsetNs":2100000000,"column":1,"key":1,"noteIndex":-1,"action":"release","noteRemoved":false,"judgement":null,"deviationNs":null,"value":null},{"timeNs":2450000000,"hitOffsetNs":2450000000,"column":2,"key":2,"noteIndex":-1,"action":"press","noteRemoved":false,"judgement":null,"deviationNs":null,"value":null},{"timeNs":2500000000,"hitOffsetNs":2500000000,"column":2,"key":-1,"noteIndex":0,"action":"none","noteRemoved":true,"judgement":6,"deviationNs":0,"value":-2},{"timeNs":2600000000,"hitOffsetNs":2600000000,"column":2,"key":2,"noteIndex":-1,"action":"release","noteRemoved":false,"judgement":null,"deviationNs":null,"value":null},{"timeNs":2970000000,"hitOffsetNs":2940000000,"column":7,"key":7,"noteIndex":0,"action":"press","noteRemoved":true,"judgement":4,"deviationNs":-30000000,"value":1},{"timeNs":3100000000,"hitOffsetNs":3100000000,"column":7,"key":7,"noteIndex":-1,"action":"release","noteRemoved":false,"judgement":null,"deviationNs":null,"value":null},{"timeNs":3500000000,"hitOffsetNs":3500000000,"column":0,"key":0,"noteIndex":1,"action":"release","noteRemoved":true,"judgement":5,"deviationNs":0,"value":2},{"timeNs":5033333333,"hitOffsetNs":5300000000,"column":1,"key":-1,"noteIndex":1,"action":"none","noteRemoved":true,"judgement":0,"deviationNs":266666667,"value":0}],"gaugeSamples":[{"timeNs":0,"value":20},{"timeNs":2000000000,"value":20},{"timeNs":2030000000,"value":95},{"timeNs":2500000000,"value":95},{"timeNs":2970000000,"value":100},{"timeNs":3500000000,"value":100},{"timeNs":5033333333,"value":94}],"result":{"points":4,"maxPoints":8,"maxPointsNow":8,"gauge":94,"combo":0,"maxCombo":3,"mineHits":1,"clearType":"NORMAL","judgementCounts":[1,0,0,0,2,1,1,0,0,1],"savedTimestampSeconds":1700000123,"scoreGuid":"fixed-core-guid","chartLengthNs":4833333333,"keymode":5,"dpMode":0}})JSON"
    };

    REQUIRE(first->finishTrace() == second->finishTrace());
    REQUIRE(first->finishTrace() == expected);
}

} // namespace
