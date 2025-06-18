//
// Created by PC on 06/02/2025.
//

#include "NoteState.h"

namespace gameplay_logic {
void
ColumnState::setPressed(bool pressed)
{
    if (this->pressed == pressed) {
        return;
    }
    this->pressed = pressed;
    emit pressedChanged();
}
ColumnState::ColumnState(QList<NoteState> notes, QObject* parent)
  : QAbstractListModel(parent)
  , notes(std::move(notes))
{
}
auto
ColumnState::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(notes.size());
}
auto
ColumnState::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(notes.at(index.row()));
    }
    return {};
}
void
ColumnState::onHitEvent(HitEvent hit)
{
    if (hit.getAction() == HitEvent::Action::Press) {
        setPressed(true);
    } else if (hit.getAction() == HitEvent::Action::Release) {
        setPressed(false);
    }
    if (hit.getNoteIndex() == -1 ||
        hit.getPointsOptional()->getJudgement() == Judgement::EmptyPoor) {
        return;
    }
    auto& note = notes[hit.getNoteIndex()];
    note.hitData = QVariant::fromValue(hit);
    if (note.note.type == Note::Type::LongNoteBegin) {
        auto& nextNote = notes[hit.getNoteIndex() + 1];
        nextNote.otherEndHitData = note.hitData;
        emit dataChanged(
          index(hit.getNoteIndex()), index(hit.getNoteIndex() + 1));
    } else if (note.note.type == Note::Type::LongNoteEnd) {
        auto& prevNote = notes[hit.getNoteIndex() - 1];
        prevNote.otherEndHitData = note.hitData;
        emit dataChanged(
          index(hit.getNoteIndex() - 1), index(hit.getNoteIndex()));
    } else {
        emit dataChanged(index(hit.getNoteIndex()), index(hit.getNoteIndex()));
    }
}
void
ColumnState::setElapsed(int64_t nanos)
{
    elapsed = nanos;
    if (notes.empty()) {
        return;
    }
    auto bottomIndex = -1;
    auto topIndex = bottomIndex;
    for (auto i = currentNote + 1; i < notes.size(); i++) {
        if (notes[i].note.time.timestamp >= elapsed) {
            break;
        }
        notes[i].belowJudgeline = true;
        currentNote = i;
        if (bottomIndex == -1) {
            bottomIndex = i;
        }
        topIndex = i;
    }
    if (bottomIndex != -1) {
        emit dataChanged(index(bottomIndex), index(topIndex));
    }
}
auto
ColumnState::isPressed() const -> bool
{
    return pressed;
}
BarLinesState::BarLinesState(QList<BarLineState> barLines, QObject* parent)
  : barLines(std::move(barLines))
{
}
auto
BarLinesState::rowCount(const QModelIndex& parent) const -> int
{
    return parent.isValid() ? 0 : static_cast<int>(barLines.size());
}
QVariant
BarLinesState::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(barLines.at(index.row()));
    }
    return {};
}
void
BarLinesState::setElapsed(int64_t nanos)
{
    elapsed = nanos;
    if (barLines.empty()) {
        return;
    }
    auto bottomIndex = -1;
    auto topIndex = bottomIndex;
    for (auto i = currentLine + 1; i < barLines.size(); i++) {
        if (barLines[i].time.timestamp >= elapsed) {
            break;
        }
        barLines[i].belowJudgeline = true;
        currentLine = i;
        if (bottomIndex == -1) {
            bottomIndex = i;
        }
        topIndex = i;
    }
    if (bottomIndex != -1) {
        emit dataChanged(index(bottomIndex), index(topIndex));
    }
}
void
Filter::setPressed(bool pressed)
{
    if (this->pressed == pressed) {
        return;
    }
    this->pressed = pressed;
    emit pressedChanged();
}
Filter::Filter(ColumnState* columnState, QObject* parent)
  : QSortFilterProxyModel(parent)
{
    setSourceModel(columnState);
    connect(
      columnState, &ColumnState::pressedChanged, this, &Filter::pressedChanged);
}
BarlineFilter::BarlineFilter(BarLinesState* barLinesState, QObject* parent)
{
    setSourceModel(barLinesState);
}
void
BarlineFilter::setTopPosition(double value)
{
    if (topPosition != value) {
        topPosition = value;
        emit topPositionChanged();
        invalidateFilter();
    }
}
void
BarlineFilter::setBottomPosition(double value)
{
    if (bottomPosition != value) {
        bottomPosition = value;
        emit bottomPositionChanged();
        invalidateFilter();
    }
}
bool
BarlineFilter::filterAcceptsRow(int source_row,
                                const QModelIndex& source_parent) const
{
    if (source_row < 0 ||
        source_row >= sourceModel()->rowCount(source_parent)) {
        return false;
    }
    const auto index = sourceModel()->index(source_row, 0, source_parent);
    const auto barLineState =
      sourceModel()->data(index, Qt::DisplayRole).value<BarLineState>();
    const auto show = barLineState.time.position <= topPosition &&
                      barLineState.time.position >= bottomPosition;
    return show;
}
void
Filter::setTopPosition(double value)
{
    if (topPosition != value) {
        topPosition = value;
        emit topPositionChanged();
        invalidateFilter();
    }
}
void
Filter::setBottomPosition(double value)
{
    if (bottomPosition != value) {
        bottomPosition = value;
        emit bottomPositionChanged();
        invalidateFilter();
    }
}
int
Filter::getRealIndex(int sourceRow) const
{
    return mapToSource(index(sourceRow, 0, QModelIndex())).row();
}
bool
Filter::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (source_row < 0 ||
        source_row >= sourceModel()->rowCount(source_parent)) {
        return false;
    }
    const auto index = sourceModel()->index(source_row, 0, source_parent);
    const auto noteState =
      sourceModel()->data(index, Qt::DisplayRole).value<NoteState>();
    const auto show = noteState.note.time.position <= topPosition &&
                      noteState.note.time.position >= bottomPosition;
    if (show) {
        return true;
    }
    if (noteState.note.type == Note::Type::LongNoteBegin) {
        if (const auto nextIndex =
              sourceModel()->index(source_row + 1, 0, source_parent);
            nextIndex.isValid()) {
            const auto nextNoteState = sourceModel()
                                         ->data(nextIndex, Qt::DisplayRole)
                                         .value<NoteState>();
            if (nextNoteState.note.time.position > bottomPosition &&
                noteState.note.time.position <= topPosition) {
                return true;
            }
        }
        return true;
    }
    if (noteState.note.type == Note::Type::LongNoteEnd) {
        // get previous note
        if (const auto prevIndex =
              sourceModel()->index(source_row - 1, 0, source_parent);
            prevIndex.isValid()) {
            const auto prevNoteState = sourceModel()
                                         ->data(prevIndex, Qt::DisplayRole)
                                         .value<NoteState>();
            if (prevNoteState.note.time.position < topPosition &&
                noteState.note.time.position >= bottomPosition) {
                return true;
            }
        }
    }
    return false;
}
GameplayState::GameplayState(QList<ColumnState*> columnStates,
                             BarLinesState* barLinesState,
                             QObject* parent)
  : QObject(parent)
  , barLinesState(barLinesState)
  , columnStates(std::move(columnStates))
{
    for (auto* const columnState : this->columnStates) {
        auto* filter = new Filter(columnState, this);
        columnState->setParent(filter);
        columnFilters.append(filter);
    }
    barLineFilter = new BarlineFilter(barLinesState, this);
    barLinesState->setParent(barLineFilter);
}
auto
GameplayState::getColumnStates() -> QList<ColumnState*>
{
    return columnStates;
}
auto
GameplayState::getColumnFilters() -> QList<Filter*>
{
    return columnFilters;
}
auto
GameplayState::getBarLinesState() const -> BarLinesState*
{
    return barLinesState;
}
auto
GameplayState::getBarLineFilter() const -> BarlineFilter*
{
    return barLineFilter;
}
} // namespace gameplay_logic