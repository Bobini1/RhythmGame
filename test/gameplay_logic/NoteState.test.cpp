#include "gameplay_logic/NoteState.h"

#include <catch2/catch_test_macros.hpp>
#include <QAbstractItemModelTester>
#include <QSignalSpy>

#include <initializer_list>
#include <memory>
#include <utility>

namespace {
// The model tester expects a text display role; QML consumes a NoteState value.
class TextDisplayFilter : public gameplay_logic::Filter
{
  public:
    using Filter::Filter;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override
    {
        return role == Qt::DisplayRole
                 ? QVariant(
                     Filter::data(index,
                                  gameplay_logic::ColumnState::NotePositionRole)
                       .toString())
                 : Filter::data(index, role);
    }
};

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

auto
makeLongNoteColumn(double begin = 1.0, double end = 3.0)
  -> gameplay_logic::ColumnState*
{
    auto notes = QList<gameplay_logic::NoteState>{
        { gameplay_logic::Note{ gameplay_logic::Time{ 0, begin, begin },
                                gameplay_logic::Snap{ 0.0, 1.0 },
                                gameplay_logic::Note::Type::LongNoteBegin },
          0 },
        { gameplay_logic::Note{ gameplay_logic::Time{ 0, end, end },
                                gameplay_logic::Snap{ 0.0, 1.0 },
                                gameplay_logic::Note::Type::LongNoteEnd },
          1 },
    };
    return new gameplay_logic::ColumnState(std::move(notes));
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

TEST_CASE("belowBottom follows the viewport through the proxy model")
{
    auto column =
      std::unique_ptr<gameplay_logic::ColumnState>(makeLongNoteColumn());
    auto filter = gameplay_logic::Filter(column.get());

    filter.setVisibleRange(2.0, 3.0);

    REQUIRE(filter.rowCount({}) == 2);
    const auto retainedBegin = filter.index(0);
    CHECK(
      filter.data(retainedBegin, gameplay_logic::ColumnState::NotePositionRole)
        .toDouble() == 1.0);
    CHECK(
      filter.data(retainedBegin, gameplay_logic::ColumnState::BelowBottomRole)
        .toBool());
    CHECK(
      column
        ->data(column->index(0), gameplay_logic::ColumnState::BelowBottomRole)
        .toBool());

    filter.setVisibleRange(0.5, 3.0);

    CHECK_FALSE(
      filter.data(filter.index(0), gameplay_logic::ColumnState::BelowBottomRole)
        .toBool());
    CHECK_FALSE(
      column
        ->data(column->index(0), gameplay_logic::ColumnState::BelowBottomRole)
        .toBool());
}

TEST_CASE("note changes notify both display and named role consumers",
          "[NoteState]")
{
    using namespace gameplay_logic;
    auto column = std::unique_ptr<ColumnState>(makeLongNoteColumn());
    Filter filter(column.get());
    filter.setVisibleRange(0, 4);
    QSignalSpy changed(&filter, &QAbstractItemModel::dataChanged);
    const auto checkDisplayNotification = [&] {
        REQUIRE_FALSE(changed.isEmpty());
        for (const auto& change : changed) {
            CHECK(change[0].value<QModelIndex>().isValid());
            CHECK(change[1].value<QModelIndex>().isValid());
            CHECK(change[2].value<QList<int>>().contains(Qt::DisplayRole));
        }
        changed.clear();
    };
    column->onHitEvent({ 0,
                         0,
                         0,
                         BmsPoints(2, Judgement::Perfect, 0),
                         HitEvent::Action::Press,
                         true });
    checkDisplayNotification();
    CHECK(filter.data(filter.index(0)).value<NoteState>().hit);
    CHECK(filter.data(filter.index(1)).value<NoteState>().otherEndHit);
    CHECK(filter.data(filter.index(0), ColumnState::HeldLongNoteRole).toBool());

    column->onHitEvent({ 0,
                         1,
                         0,
                         BmsPoints(0, Judgement::LnEndSkip, 0),
                         HitEvent::Action::Release,
                         true });
    checkDisplayNotification();
    CHECK(filter.data(filter.index(0)).value<NoteState>().otherEndHit);
    CHECK_FALSE(
      filter.data(filter.index(0), ColumnState::HeldLongNoteRole).toBool());

    filter.setVisibleRange(2, 4);
    checkDisplayNotification();
    CHECK(filter.data(filter.index(0)).value<NoteState>().belowBottom);
    filter.setVisibleRange(0, 4);
    checkDisplayNotification();
    CHECK_FALSE(filter.data(filter.index(0)).value<NoteState>().belowBottom);

    filter.setVisibleRange(20, 24);
    changed.clear();
    column->onHitEvent({ 0,
                         0,
                         0,
                         BmsPoints(2, Judgement::Perfect, 0),
                         HitEvent::Action::Press,
                         true });
    CHECK(changed.isEmpty());
}

TEST_CASE("long note bodies stay visible in either scroll direction",
          "[NoteState]")
{
    using namespace gameplay_logic;
    for (bool reverse : { false, true }) {
        CAPTURE(reverse);
        auto column = std::unique_ptr<ColumnState>(
          makeLongNoteColumn(reverse ? 10 : 5, reverse ? 5 : 10));
        TextDisplayFilter filter(column.get());
        QAbstractItemModelTester tester(
          &filter, QAbstractItemModelTester::FailureReportingMode::Fatal);
        for (const auto range : { std::pair{ 6.0, 8.0 },
                                  { 11.0, 13.0 },
                                  { 2.0, 4.0 },
                                  { 8.0, 9.0 },
                                  { 5.0, 10.0 },
                                  { 10.0, 10.0 },
                                  { 6.0, 8.0 } }) {
            CAPTURE(range.first, range.second);
            filter.setVisibleRange(range.first, range.second);
            const auto begin = filter.mapFromSource(
              column->index(column->mapTimeIndexToPositionIndex(0)));
            const auto intersects = range.first <= 10 && range.second >= 5;
            CHECK(begin.isValid() == intersects);
            if (intersects) {
                CHECK(filter.data(begin, ColumnState::NoteTypeRole).toInt() ==
                      static_cast<int>(Note::Type::LongNoteBegin));
                CHECK(filter.data(begin, ColumnState::LongNoteEndPositionRole)
                        .toDouble() == (reverse ? 5 : 10));
            }
        }
    }
}

TEST_CASE("note filter preserves model indexes across moves and jumps",
          "[NoteState]")
{
    auto column = std::unique_ptr<gameplay_logic::ColumnState>(
      makeColumn({ -4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 14 }));
    TextDisplayFilter filter(column.get());
    QAbstractItemModelTester tester(
      &filter, QAbstractItemModelTester::FailureReportingMode::Fatal);
    for (const auto bottom : { 1.0, 2.0, 2.2, -2.0, 20.0, -10.0, 6.0, 3.0 }) {
        for (const auto span : { 0.0, 1.0, 5.0, 15.0 }) {
            CAPTURE(bottom, span);
            const auto previous = filter.index(0);
            const auto source = filter.mapToSource(previous);
            QPersistentModelIndex retained(previous);
            filter.setVisibleRange(bottom, bottom + span);
            int expectedCount = 0;
            for (const auto& note : column->getNotes()) {
                if (note.note.time.position >= bottom &&
                    note.note.time.position <= bottom + span)
                    ++expectedCount;
            }
            REQUIRE(filter.rowCount({}) == expectedCount);
            if (filter.mapFromSource(source).isValid()) {
                CHECK(retained.isValid());
                CHECK(filter.mapToSource(retained) == source);
            }
            for (int row = 0; row < expectedCount; ++row) {
                const auto index = filter.index(row);
                CHECK(filter.mapFromSource(filter.mapToSource(index)) == index);
            }
        }
    }
}
