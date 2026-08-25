#include <catch2/catch_test_macros.hpp>

#include "sounds/ChartAudioPlayer.h"

using namespace std::chrono_literals;

TEST_CASE("chart audio schedules only sounds produced by autoplay",
          "[sounds][ChartAudioPlayer]")
{
    auto notes = charts::BmsNotesData{};
    notes.bgmNotes.emplace_back(charts::BmsNotesData::Time{ 10ns, 0.0, 0.0 },
                                1);
    notes.notes[0] = {
        { { 20ns, 0.0, 0.0 }, {}, charts::BmsNotesData::NoteType::Normal, 2 },
        { { 30ns, 0.0, 0.0 },
          {},
          charts::BmsNotesData::NoteType::LongNoteBegin,
          3 },
        { { 40ns, 0.0, 0.0 },
          {},
          charts::BmsNotesData::NoteType::LongNoteEnd,
          4 },
        { { 50ns, 0.0, 0.0 },
          {},
          charts::BmsNotesData::NoteType::Invisible,
          5 },
        { { 60ns, 0.0, 0.0 }, {}, charts::BmsNotesData::NoteType::Landmine, 6 },
    };

    const auto events = sounds::createChartAudioEvents(notes);

    REQUIRE(events.size() == 3);
    CHECK(events[0] == sounds::ChartAudioEvent{ 10ns, 1 });
    CHECK(events[1] == sounds::ChartAudioEvent{ 20ns, 2 });
    CHECK(events[2] == sounds::ChartAudioEvent{ 30ns, 3 });
}

TEST_CASE("chart audio loop keeps the chart tail before restarting",
          "[sounds][ChartAudioPlayer]")
{
    const auto events = std::vector<sounds::ChartAudioEvent>{
        { 3s, 1 },
        { 12s, 2 },
    };

    CHECK(sounds::chartAudioLoopLength(events, 10s) == 17s);
    CHECK(sounds::chartAudioLoopLength(events, 20s) == 25s);
    CHECK(sounds::chartAudioLoopLength({}, 0ns) == 5s);
}

TEST_CASE("chart audio preview skips only the leading scheduled silence",
          "[sounds][ChartAudioPlayer]")
{
    auto events = std::vector<sounds::ChartAudioEvent>{
        { 5s, 1 },
        { 8s, 2 },
    };

    const auto skipped = sounds::skipChartAudioLeadingSilence(events);

    CHECK(skipped == 5s);
    CHECK(events[0] == sounds::ChartAudioEvent{ 0ns, 1 });
    CHECK(events[1] == sounds::ChartAudioEvent{ 3s, 2 });
    CHECK(sounds::chartAudioLoopLength(events, 12s - skipped) == 12s);

    auto emptyEvents = std::vector<sounds::ChartAudioEvent>{};
    CHECK(sounds::skipChartAudioLeadingSilence(emptyEvents) == 0ns);
}
