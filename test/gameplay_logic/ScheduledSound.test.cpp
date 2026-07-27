#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "sounds/MultiSound.h"
#include "sounds/Sound.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

using namespace std::chrono_literals;
using gameplay_logic::rules::HitRules;

class RecordingSound final : public sounds::Sound
{
  public:
    std::vector<std::chrono::nanoseconds> playTimes;
    std::vector<std::chrono::nanoseconds> stopTimes;
    int legacyPlayCount = 0;
    int legacyStopCount = 0;
    float volume = 1.0F;

    void play() override { ++legacyPlayCount; }
    void playAt(std::chrono::nanoseconds chartTime) override
    {
        playTimes.push_back(chartTime);
    }
    void stop() override { ++legacyStopCount; }
    void stopAt(std::chrono::nanoseconds chartTime) override
    {
        stopTimes.push_back(chartTime);
    }
    void setVolume(float newVolume) override { volume = newVolume; }
    auto isPlaying() const -> bool override { return false; }
    auto getVolume() const -> float override { return volume; }
};

class LegacyOnlySound final : public sounds::Sound
{
  public:
    int playCount = 0;
    int stopCount = 0;

    void play() override { ++playCount; }
    void stop() override { ++stopCount; }
    void setVolume(float) override {}
    auto isPlaying() const -> bool override { return false; }
    auto getVolume() const -> float override { return 1.0F; }
};

auto makeHitRules() -> HitRules
{
    return HitRules(
      gameplay_logic::rules::lr2_timing_windows::judgeNormal(),
      [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
          return 1.0;
      });
}

auto makeReferee(
  const std::vector<std::pair<charts::BmsNotesData::Time, uint64_t>>& bgms,
  std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>> sounds)
  -> std::unique_ptr<gameplay_logic::BmsGameReferee>
{
    auto notes =
      std::array<std::vector<charts::BmsNotesData::Note>,
                 charts::BmsNotesData::columnNumber>{};
    return std::make_unique<gameplay_logic::BmsGameReferee>(
      std::move(notes),
      bgms,
      std::vector<charts::BmsNotesData::BpmChangeValues>{},
      nullptr,
      nullptr,
      std::move(sounds),
      makeHitRules());
}

TEST_CASE("ScheduledSound press carries the input occurrence timestamp",
          "[ScheduledSound]")
{
    auto sound = RecordingSound{};
    auto rules = makeHitRules();
    auto notes = std::vector<HitRules::Note>{
        { &sound, 1s, HitRules::NoteType::Normal, 0 },
    };

    rules.press(notes, 0, 1007ms);

    REQUIRE(sound.playTimes ==
            std::vector<std::chrono::nanoseconds>{ 1007ms });
    CHECK(sound.legacyPlayCount == 0);
}

TEST_CASE("ScheduledSound release retains no-play semantics",
          "[ScheduledSound]")
{
    auto sound = RecordingSound{};
    auto rules = makeHitRules();
    auto notes = std::vector<HitRules::Note>{
        { &sound, 1s, HitRules::NoteType::LnBegin, 0 },
        { &sound, 2s, HitRules::NoteType::LnEnd, 1 },
    };

    rules.press(notes, 0, 1s, input::BmsKey::Col11);
    rules.release(notes, 0, 1900ms, input::BmsKey::Col11);

    REQUIRE(sound.playTimes == std::vector<std::chrono::nanoseconds>{ 1s });
    CHECK(sound.legacyPlayCount == 0);
    CHECK(sound.stopTimes.empty());
    CHECK(sound.legacyStopCount == 0);
}

TEST_CASE("ScheduledSound mine carries the collision timestamp",
          "[ScheduledSound]")
{
    auto sound = RecordingSound{};
    auto rules = makeHitRules();
    auto mines = std::vector<HitRules::Mine>{
        { -10.0, 1s, 0 },
    };

    rules.processMines(mines, 0, 1011ms, true, &sound);

    REQUIRE(sound.playTimes ==
            std::vector<std::chrono::nanoseconds>{ 1011ms });
    CHECK(sound.legacyPlayCount == 0);
}

TEST_CASE("ScheduledSound LN miss stops at the authored judgement boundary",
          "[ScheduledSound]")
{
    auto sound = RecordingSound{};
    auto rules = makeHitRules();
    auto notes = std::vector<HitRules::Note>{
        { &sound, 1s, HitRules::NoteType::LnBegin, 0 },
        { &sound, 2s, HitRules::NoteType::LnEnd, 1 },
    };

    rules.processMisses(notes, 0, 1500ms);

    REQUIRE(sound.stopTimes ==
            std::vector<std::chrono::nanoseconds>{ 1200ms });
    CHECK(sound.legacyStopCount == 0);
}

TEST_CASE("ScheduledSound update uses each BGM authored timestamp",
          "[ScheduledSound]")
{
    auto sound = std::make_shared<RecordingSound>();
    auto referee = makeReferee(
      { { charts::BmsNotesData::Time{ 100ms, 0.0, 0.0 }, 1 },
        { charts::BmsNotesData::Time{ 250ms, 0.0, 0.0 }, 1 } },
      { { 1, sound } });

    referee->update(500ms);

    REQUIRE(sound->playTimes ==
            std::vector<std::chrono::nanoseconds>{ 100ms, 250ms });
    CHECK(sound->legacyPlayCount == 0);
}

TEST_CASE("ScheduledSound BGM pre-scheduling is idempotent and not replayed",
          "[ScheduledSound]")
{
    auto sound = std::make_shared<RecordingSound>();
    auto referee = makeReferee(
      { { charts::BmsNotesData::Time{ 100ms, 0.0, 0.0 }, 1 },
        { charts::BmsNotesData::Time{ 250ms, 0.0, 0.0 }, 1 } },
      { { 1, sound } });

    referee->preScheduleBgm();
    referee->preScheduleBgm();
    referee->update(500ms);

    REQUIRE(sound->playTimes ==
            std::vector<std::chrono::nanoseconds>{ 100ms, 250ms });
    CHECK(sound->legacyPlayCount == 0);
}

TEST_CASE("ScheduledSound final update keeps future BGM suppressed",
          "[ScheduledSound]")
{
    auto sound = std::make_shared<RecordingSound>();
    auto referee = makeReferee(
      { { charts::BmsNotesData::Time{ 100ms, 0.0, 0.0 }, 1 } },
      { { 1, sound } });

    referee->update(50ms, true);
    referee->preScheduleBgm();
    referee->update(500ms);

    CHECK(sound->playTimes.empty());
    CHECK(sound->legacyPlayCount == 0);
    CHECK(sound->legacyStopCount == 1);
}

TEST_CASE("ScheduledSound rejects pre-scheduling after native BGM advancement",
          "[ScheduledSound]")
{
    auto sound = std::make_shared<RecordingSound>();
    auto referee = makeReferee(
      { { charts::BmsNotesData::Time{ 100ms, 0.0, 0.0 }, 1 },
        { charts::BmsNotesData::Time{ 250ms, 0.0, 0.0 }, 1 } },
      { { 1, sound } });

    referee->update(150ms);

    REQUIRE_THROWS_AS(referee->preScheduleBgm(), std::logic_error);
    REQUIRE(sound->playTimes ==
            std::vector<std::chrono::nanoseconds>{ 100ms });

    referee->update(500ms);
    REQUIRE(sound->playTimes ==
            std::vector<std::chrono::nanoseconds>{ 100ms, 250ms });
}

TEST_CASE("ScheduledSound default scheduling preserves immediate native sound",
          "[ScheduledSound]")
{
    auto sound = LegacyOnlySound{};

    sound.playAt(321ms);
    sound.stopAt(654ms);

    CHECK(sound.playCount == 1);
    CHECK(sound.stopCount == 1);
}

TEST_CASE("ScheduledSound composite forwards scheduled timestamps to children",
          "[ScheduledSound]")
{
    auto first = std::make_shared<RecordingSound>();
    auto second = std::make_shared<RecordingSound>();
    auto sound = sounds::MultiSound({ first, second });

    sound.playAt(123ms);
    sound.stopAt(456ms);

    for (const auto& child : { first, second }) {
        CHECK(child->playTimes ==
              std::vector<std::chrono::nanoseconds>{ 123ms });
        CHECK(child->stopTimes ==
              std::vector<std::chrono::nanoseconds>{ 456ms });
        CHECK(child->legacyPlayCount == 0);
        CHECK(child->legacyStopCount == 0);
    }
}

TEST_CASE("ScheduledSound composite preserves immediate child operations",
          "[ScheduledSound]")
{
    auto first = std::make_shared<RecordingSound>();
    auto second = std::make_shared<RecordingSound>();
    auto sound = sounds::MultiSound({ first, second });

    sound.play();
    sound.stop();

    for (const auto& child : { first, second }) {
        CHECK(child->playTimes.empty());
        CHECK(child->stopTimes.empty());
        CHECK(child->legacyPlayCount == 1);
        CHECK(child->legacyStopCount == 1);
    }
}

} // namespace
