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
    // sort by position
    std::ranges::stable_sort(notes, [](const auto& a, const auto& b) {
        return a.note.time.position < b.note.time.position;
    });
    timeToPositionIndexMapping.resize(notes.size());
    for (int i = 0; i < notes.size(); ++i) {
        timeToPositionIndexMapping[notes[i].index] = i;
    }
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
        emit dataChanged(index(hit.getNoteIndex()),
                         index(hit.getNoteIndex() + 1));
    } else if (note.note.type == Note::Type::LongNoteEnd) {
        auto& prevNote = notes[hit.getNoteIndex() - 1];
        prevNote.otherEndHitData = note.hitData;
        emit dataChanged(index(hit.getNoteIndex() - 1),
                         index(hit.getNoteIndex()));
    } else {
        emit dataChanged(index(hit.getNoteIndex()), index(hit.getNoteIndex()));
    }
}
auto
ColumnState::isPressed() const -> bool
{
    return pressed;
}
auto
ColumnState::mapTimeIndexToPositionIndex(int64_t timeIndex) const -> int
{
    return timeToPositionIndexMapping[timeIndex];
}
BarLinesState::BarLinesState(QList<BarLineState> barLines, QObject* parent)
  : barLines(std::move(barLines))
{
    std::ranges::sort(barLines, [](const auto& a, const auto& b) {
        return a.time.position < b.time.position;
    });
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
Filter::setPressed(bool pressed)
{
    if (this->pressed == pressed) {
        return;
    }
    this->pressed = pressed;
    emit pressedChanged();
}
void
Filter::adjustLnEndVisibility(int notesFrom, int notesTo, bool shown)
{
    for (int i = notesFrom; i < notesTo; i++) {
        auto note = columnState->getNotes()[i];
        auto otherEndIndex = 0;
        if (note.note.type == Note::Type::LongNoteBegin) {
            otherEndIndex =
              columnState->mapTimeIndexToPositionIndex(note.index + 1);
        } else if (note.note.type == Note::Type::LongNoteEnd) {
            otherEndIndex =
              columnState->mapTimeIndexToPositionIndex(note.index - 1);
        }
        auto& endNote = columnState->getNotes()[otherEndIndex];
        if (endNote.belowBottom == shown) {
            endNote.belowBottom = !shown;
            emit columnState->dataChanged(
              columnState->index(otherEndIndex, 0, QModelIndex()),
              columnState->index(otherEndIndex, 0, QModelIndex()));
        }
    }
}
Filter::Filter(ColumnState* columnState, QObject* parent)
  : QAbstractProxyModel(parent)
  , columnState(columnState)
{
    setSourceModel(columnState);
    connect(columnState,
            &ColumnState::pressedChanged,
            this,
            [this, columnState]() { setPressed(columnState->isPressed()); });
    connect(columnState,
            &ColumnState::dataChanged,
            this,
            [this](const QModelIndex& topLeft,
                   const QModelIndex& bottomRight,
                   const QVector<int>& roles) {
                emit dataChanged(index(topLeft.row() - bottomRow),
                                 index(bottomRight.row() - bottomRow),
                                 roles);
            });
}
BarlineFilter::BarlineFilter(BarLinesState* barLinesState, QObject* parent)
{
    setSourceModel(barLinesState);
}
void
BarlineFilter::setTopPosition(double value)
{
    if (topPosition != value) {
        beginFilterChange();
        topPosition = value;
        emit topPositionChanged();
        endFilterChange();
    }
}
void
BarlineFilter::setBottomPosition(double value)
{
    if (bottomPosition != value) {
        beginFilterChange();
        bottomPosition = value;
        emit bottomPositionChanged();
        endFilterChange();
    }
}
bool
BarlineFilter::filterAcceptsRow(int sourceRow,
                                const QModelIndex& sourceParent) const
{
    if (sourceRow < 0 || sourceRow >= sourceModel()->rowCount(sourceParent)) {
        return false;
    }
    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto barLineState =
      sourceModel()->data(index, Qt::DisplayRole).value<BarLineState>();
    const auto show = barLineState.time.position <= topPosition &&
                      barLineState.time.position >= bottomPosition;
    return show;
}
void
Filter::setTopPosition(double value)
{
    if (topPosition == value) {
        return;
    }
    if (value < bottomPosition) {
        setBottomPosition(value);
    }
    // Find the first note strictly above the new top position, excluding
    // LongNoteEnd
    const auto upper =
      std::ranges::find_if(columnState->getNotes(), [value](const auto& note) {
          return note.note.type != Note::Type::LongNoteEnd &&
                 note.note.time.position > value;
      });
    auto newTopRow =
      static_cast<int>(std::distance(columnState->getNotes().begin(), upper));
    if (newTopRow > topRow) {
        beginInsertRows(
          QModelIndex(), topRow - bottomRow, newTopRow - bottomRow - 1);
        topPosition = value;
        topRow = newTopRow;
        endInsertRows();
    } else if (newTopRow < topRow) {
        beginRemoveRows(
          QModelIndex(), newTopRow - bottomRow, topRow - bottomRow - 1);
        topPosition = value;
        topRow = newTopRow;
        endRemoveRows();
    }
    if (topRow != newTopRow) {
        auto notesFrom = std::min(topRow, newTopRow);
        auto notesTo = std::max(topRow, newTopRow);
        auto shown = notesFrom <= notesTo;
        adjustLnEndVisibility(notesFrom, notesTo, shown);
    }

    emit topPositionChanged();
}
void
Filter::setBottomPosition(double value)
{
    if (bottomPosition == value) {
        return;
    }

    auto lower = std::lower_bound(columnState->getNotes().begin(),
                                  columnState->getNotes().end(),
                                  value,
                                  [](const auto& note, double value) {
                                      return note.note.time.position < value;
                                  });

    const auto newBottomRow =
      static_cast<int>(std::distance(columnState->getNotes().begin(), lower));
    if (newBottomRow > bottomRow) [[likely]] {
        beginRemoveRows(QModelIndex(), 0, newBottomRow - bottomRow - 1);
        auto oldBottomRow = bottomRow;
        bottomPosition = value;
        bottomRow = newBottomRow;
        endRemoveRows();
        for (int i = oldBottomRow; i < newBottomRow; ++i) {
            auto& note = columnState->getNotes()[i];
            note.belowBottom = true;
        }
        emit columnState->dataChanged(
          columnState->index(oldBottomRow, 0, QModelIndex()),
          columnState->index(newBottomRow - 1, 0, QModelIndex()));
    } else if (newBottomRow < bottomRow) [[unlikely]] {
        beginInsertRows(QModelIndex(), 0, bottomRow - newBottomRow - 1);
        auto oldBottomRow = bottomRow;
        bottomPosition = value;
        bottomRow = newBottomRow;
        endInsertRows();
        for (int i = newBottomRow; i < oldBottomRow; ++i) {
            auto& note = columnState->getNotes()[i];
            note.belowBottom = false;
        }
        emit columnState->dataChanged(
          columnState->index(newBottomRow, 0, QModelIndex()),
          columnState->index(oldBottomRow - 1, 0, QModelIndex()));
    }
    if (bottomRow != newBottomRow) {
        auto notesFrom = std::min(bottomRow, newBottomRow);
        auto notesTo = std::max(bottomRow, newBottomRow);
        auto shown = newBottomRow < bottomRow;
        adjustLnEndVisibility(notesFrom, notesTo, shown);
    }
    emit bottomPositionChanged();
}
QModelIndex
Filter::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= topRow - bottomRow || column != 0) {
        return QModelIndex();
    }
    return createIndex(
      row, column, &columnState->getNotes().at(row + bottomRow));
}
QModelIndex
Filter::parent(const QModelIndex& child) const
{
    return QModelIndex();
}
int
Filter::rowCount(const QModelIndex& parent) const
{
    return topRow - bottomRow;
}
int
Filter::columnCount(const QModelIndex& parent) const
{
    return 1;
}
QModelIndex
Filter::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.row() < 0 ||
        proxyIndex.row() >= rowCount({})) {
        return QModelIndex();
    }
    return columnState->index(
      proxyIndex.row() + bottomRow, proxyIndex.column(), QModelIndex());
}
QModelIndex
Filter::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != columnState) {
        return QModelIndex();
    }
    return index(
      sourceIndex.row() - bottomRow, sourceIndex.column(), QModelIndex());
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
