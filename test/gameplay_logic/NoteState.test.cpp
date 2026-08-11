#include "gameplay_logic/NoteState.h"

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <utility>

namespace {
auto
makeColumn(std::initializer_list<double> positions)
  -> gameplay_logic::ColumnState*
{
    auto notes = QList<gameplay_logic::NoteState>{};
    notes.reserve(static_cast<qsizetype>(positions.size()));
    auto index = qint64{ 0 };
    for (const auto position : positions) {
        notes.append(
          { gameplay_logic::Note{ gameplay_logic::Time{ 0, position, position },
                                  gameplay_logic::Snap{ 0.0, 1.0 },
                                  gameplay_logic::Note::Type::Normal },
            index++ });
    }
    return new gameplay_logic::ColumnState(std::move(notes));
}

auto
makeBarLines(std::initializer_list<double> positions)
  -> gameplay_logic::BarLinesState*
{
    auto barLines = QList<gameplay_logic::BarLineState>{};
    barLines.reserve(static_cast<qsizetype>(positions.size()));
    auto index = qint64{ 0 };
    for (const auto position : positions) {
        barLines.append(
          { gameplay_logic::Time{ 0, position, position }, index++ });
    }
    return new gameplay_logic::BarLinesState(std::move(barLines));
}
}

TEST_CASE("gameplay viewport updates all note and barline filters")
{
    auto state = gameplay_logic::GameplayState{
        { makeColumn({ 0.0, 1.0, 2.0, 3.0 }),
          makeColumn({ 0.0, 1.0, 2.0, 3.0 }) },
        makeBarLines({ 0.0, 1.0, 2.0, 3.0, 4.0 })
    };

    state.setVisiblePositionSpans(2.0, 1.0);
    state.setPosition(1.0);

    for (const auto* filter : state.getColumnFilters()) {
        CHECK(filter->getBottomPosition() == 1.0);
        CHECK(filter->getTopPosition() == 3.0);
        CHECK(filter->rowCount({}) == 3);
    }
    CHECK(state.getBarLineFilter()->getBottomPosition() == 1.0);
    CHECK(state.getBarLineFilter()->getTopPosition() == 2.0);
    CHECK(state.getBarLineFilter()->rowCount({}) == 2);

    state.setPosition(2.5);

    for (const auto* filter : state.getColumnFilters()) {
        CHECK(filter->getBottomPosition() == 2.5);
        CHECK(filter->getTopPosition() == 4.5);
        CHECK(filter->rowCount({}) == 1);
    }
    CHECK(state.getBarLineFilter()->getBottomPosition() == 2.5);
    CHECK(state.getBarLineFilter()->getTopPosition() == 3.5);
    CHECK(state.getBarLineFilter()->rowCount({}) == 1);
}

TEST_CASE("changing viewport spans immediately refilters the current position")
{
    auto state = gameplay_logic::GameplayState{
        { makeColumn({ 0.0, 1.0, 2.0, 3.0 }) },
        makeBarLines({ 0.0, 1.0, 2.0, 3.0, 4.0 })
    };
    state.setPosition(1.0);
    state.setVisiblePositionSpans(2.0, 1.0);

    state.setVisiblePositionSpans(1.0, 2.0);

    const auto* filter = state.getColumnFilters().front();
    CHECK(filter->getBottomPosition() == 1.0);
    CHECK(filter->getTopPosition() == 2.0);
    CHECK(filter->rowCount({}) == 2);
    CHECK(state.getBarLineFilter()->getBottomPosition() == 1.0);
    CHECK(state.getBarLineFilter()->getTopPosition() == 3.0);
    CHECK(state.getBarLineFilter()->rowCount({}) == 3);
}
