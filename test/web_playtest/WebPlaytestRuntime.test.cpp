#include "web_playtest/InputEvent.h"
#include "web_playtest/WebPlaytestSnapshot.h"

#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace web_playtest;

TEST_CASE("browser input presets map exact physical codes")
{
    const auto nativeScratch =
      mapBrowserCode("ControlLeft", InputPreset::Native);
    REQUIRE(nativeScratch);
    CHECK(nativeScratch->key == input::BmsKey::Col1sDown);
    CHECK_FALSE(mapBrowserCode("ControlLeft", InputPreset::Lr2));

    const auto nativeFirst = mapBrowserCode("KeyA", InputPreset::Native);
    const auto lr2First = mapBrowserCode("KeyZ", InputPreset::Lr2);
    REQUIRE(nativeFirst);
    REQUIRE(lr2First);
    CHECK(nativeFirst->key == input::BmsKey::Col11);
    CHECK(lr2First->key == input::BmsKey::Col11);
    CHECK_FALSE(mapBrowserCode("ShiftRight", InputPreset::Native));
    CHECK(mapBrowserCode("Enter", InputPreset::Native)->control ==
          BrowserControl::Start);
    CHECK(mapBrowserCode("Escape", InputPreset::Lr2)->control ==
          BrowserControl::Abort);
}

TEST_CASE("browser key state deduplicates and synthesizes releases")
{
    BrowserInputDeduplicator state;
    const auto mapping = mapBrowserCode("KeyA", InputPreset::Native);
    REQUIRE(mapping);
    REQUIRE(state.apply(*mapping, true, false));
    CHECK_FALSE(state.apply(*mapping, true, false));
    CHECK_FALSE(state.apply(*mapping, true, true));
    CHECK(state.anyPressed());

    std::array<BrowserKeyTransition,
               BrowserInputDeduplicator::gameplayCodeCount>
      releases{};
    const auto count = state.synthesizeReleases(InputPreset::Native, releases);
    REQUIRE(count == 1);
    CHECK(releases[0].key == input::BmsKey::Col11);
    CHECK(releases[0].action == gameplay_logic::GameplayKeyAction::Release);
    CHECK_FALSE(state.anyPressed());
    CHECK(state.synthesizeReleases(InputPreset::Native, releases) == 0);
}

TEST_CASE("one FIFO preserves command order while timestamps clamp")
{
    RuntimeCommandQueue<4> queue;
    REQUIRE(queue.tryPush({ .type = RuntimeCommandType::Tick,
                            .sequenceId = 1,
                            .browserMonotonicUs = 200 }));
    REQUIRE(
      queue.tryPush({ .type = RuntimeCommandType::Input,
                      .input = { .sequenceId = 2, .browserMonotonicUs = 100 },
                      .sequenceId = 2,
                      .browserMonotonicUs = 100 }));

    RuntimeCommand first;
    RuntimeCommand second;
    REQUIRE(queue.tryPop(first));
    REQUIRE(queue.tryPop(second));
    CHECK(first.type == RuntimeCommandType::Tick);
    CHECK(second.type == RuntimeCommandType::Input);
    CHECK(second.browserMonotonicUs == 100);

    GameplayTimestampWatermark watermark;
    CHECK(watermark.clamp(200'000) == 200'000);
    CHECK(watermark.clamp(100'000) == 200'000);
    CHECK(watermark.lateInputClampNs() == 100'000);
    CHECK(watermark.clamp(250'000) == 250'000);
    CHECK(watermark.lateInputClampNs() == 100'000);
}

TEST_CASE("runtime queue reports bounded overflow")
{
    RuntimeCommandQueue<2> queue;
    REQUIRE(queue.tryPush({ .sequenceId = 1 }));
    REQUIRE(queue.tryPush({ .sequenceId = 2 }));
    CHECK_FALSE(queue.tryPush({ .sequenceId = 3 }));
    CHECK(queue.droppedCount() == 1);
}

TEST_CASE("snapshot mailbox never overwrites a reading slot")
{
    SnapshotMailbox mailbox;
    mailbox.reserveVisibleNotes(4);

    std::size_t firstSlot{};
    auto* first = mailbox.tryBeginWrite(firstSlot);
    REQUIRE(first != nullptr);
    first->gameplay.chartTimeNs = 10;
    mailbox.publishWrite(firstSlot, 1);
    const auto* reading = mailbox.tryAcquireLatest();
    REQUIRE(reading != nullptr);
    CHECK(reading->gameplay.chartTimeNs == 10);
    CHECK(mailbox.stateForTesting(firstSlot) == SnapshotSlotState::Reading);

    std::size_t secondSlot{};
    std::size_t thirdSlot{};
    REQUIRE(mailbox.tryBeginWrite(secondSlot) != nullptr);
    mailbox.publishWrite(secondSlot, 2);
    REQUIRE(mailbox.tryBeginWrite(thirdSlot) != nullptr);
    mailbox.publishWrite(thirdSlot, 3);
    std::size_t unavailable{};
    CHECK(mailbox.tryBeginWrite(unavailable) == nullptr);
    CHECK(mailbox.droppedSnapshots() == 1);
    CHECK(mailbox.stateForTesting(firstSlot) == SnapshotSlotState::Reading);

    mailbox.releaseReading();
    CHECK(mailbox.stateForTesting(firstSlot) == SnapshotSlotState::Free);
    const auto* newest = mailbox.tryAcquireLatest();
    REQUIRE(newest != nullptr);
    CHECK(newest->publicationSequence == 3);
    mailbox.releaseReading();
}

TEST_CASE("held long note body remains visible after its begin is removed")
{
    WebPlaytestNoteModel model;
    model.reserve(2);
    gameplay_logic::GameplaySnapshot snapshot{};
    snapshot.visibleNotes.push_back(
      { .stableId = 1,
        .column = 0,
        .type = charts::BmsNotesData::NoteType::LongNoteBegin,
        .chartTimeNs = 1,
        .beatPosition = 1.0,
        .scrollPosition = 1.0,
        .pairedScrollPosition = 4.0,
        .removed = true,
        .holding = true });
    model.apply(snapshot);
    CHECK(model.rowCount() == 1);

    snapshot.visibleNotes.front().holding = false;
    model.apply(snapshot);
    CHECK(model.rowCount() == 0);
}
