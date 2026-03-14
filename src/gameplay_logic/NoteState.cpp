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
    std::ranges::stable_sort(this->notes, [](const auto& a, const auto& b) {
        return a.note.time.position < b.note.time.position;
    });
    timeToPositionIndexMapping.resize(this->notes.size());
    for (int i = 0; i < this->notes.size(); ++i) {
        timeToPositionIndexMapping[this->notes[i].index] = i;
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
    auto& note = notes[mapTimeIndexToPositionIndex(hit.getNoteIndex())];
    note.hitData = QVariant::fromValue(hit);
    const auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex());
    emit dataChanged(index(changedIndex), index(changedIndex));
    if (note.note.type == Note::Type::LongNoteBegin) {
        auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex() + 1);
        auto& nextNote = notes[changedIndex];
        nextNote.otherEndHitData = note.hitData;
        emit dataChanged(index(changedIndex), index(changedIndex));
    } else if (note.note.type == Note::Type::LongNoteEnd) {
        auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex() - 1);
        auto& prevNote = notes[changedIndex];
        prevNote.otherEndHitData = note.hitData;
        emit dataChanged(index(changedIndex), index(changedIndex));
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
int
Filter::getEffectiveBottomRow(int notesFrom, int notesTo)
{
    auto effectiveBottomRow = notesFrom;
    for (int i = notesFrom; i < notesTo; i++) {
        auto note = columnState->getNotes()[i];
        auto otherEndRow = 0;
        if (note.note.type == Note::Type::LongNoteBegin) {
            otherEndRow =
              columnState->mapTimeIndexToPositionIndex(note.index + 1);
        } else if (note.note.type == Note::Type::LongNoteEnd) {
            otherEndRow =
              columnState->mapTimeIndexToPositionIndex(note.index - 1);
        } else {
            continue;
        }
        auto& endNote = columnState->getNotes()[otherEndRow];
        effectiveBottomRow = std::min(effectiveBottomRow, otherEndRow);
    }
    return effectiveBottomRow;
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
    const auto upper =
      std::ranges::find_if(columnState->getNotes(), [value](const auto& note) {
          return note.note.time.position > value;
      });
    const auto newTopRow =
      static_cast<int>(std::distance(columnState->getNotes().begin(), upper));
    if (newTopRow > topRow) {
        beginInsertRows(QModelIndex(),
                        topRow - effectiveBottomRow,
                        newTopRow - effectiveBottomRow - 1);
        topRow = newTopRow;
        endInsertRows();
    } else if (newTopRow < topRow) {
        beginRemoveRows(QModelIndex(),
                        newTopRow - effectiveBottomRow,
                        topRow - effectiveBottomRow - 1);
        topRow = newTopRow;
        endRemoveRows();
    }
    if (topRow != newTopRow) {
        setEffectiveBottomRow(getEffectiveBottomRow(newTopRow, topRow));
    }

    topPosition = value;
    emit topPositionChanged();
}
void
Filter::setEffectiveBottomRow(const int newEffectiveBottomRow)
{
    if (newEffectiveBottomRow > effectiveBottomRow) {
        beginRemoveRows(
          QModelIndex(), 0, newEffectiveBottomRow - effectiveBottomRow - 1);
        effectiveBottomRow = newEffectiveBottomRow;
        endRemoveRows();
    } else if (newEffectiveBottomRow < effectiveBottomRow) {
        beginInsertRows(
          QModelIndex(), 0, effectiveBottomRow - newEffectiveBottomRow - 1);
        effectiveBottomRow = newEffectiveBottomRow;
        endInsertRows();
    }
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
    const auto oldBottomRow = bottomRow;
    if (newBottomRow > bottomRow) {
        for (int i = oldBottomRow; i < newBottomRow; ++i) {
            auto& note = columnState->getNotes()[i];
            note.belowBottom = true;
        }
        emit columnState->dataChanged(
          columnState->index(oldBottomRow, 0, QModelIndex()),
          columnState->index(newBottomRow - 1, 0, QModelIndex()));
    } else if (newBottomRow < bottomRow) {
        for (int i = newBottomRow; i < oldBottomRow; ++i) {
            auto& note = columnState->getNotes()[i];
            note.belowBottom = false;
        }
        emit columnState->dataChanged(
          columnState->index(newBottomRow, 0, QModelIndex()),
          columnState->index(oldBottomRow - 1, 0, QModelIndex()));
    }
    if (oldBottomRow != newBottomRow) {
        setEffectiveBottomRow(getEffectiveBottomRow(newBottomRow, topRow));
    }
    bottomPosition = value;
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
    return topRow - effectiveBottomRow;
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
    return columnState->index(proxyIndex.row() + effectiveBottomRow,
                              proxyIndex.column(),
                              QModelIndex());
}
QModelIndex
Filter::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != columnState) {
        return QModelIndex();
    }
    return index(sourceIndex.row() - effectiveBottomRow,
                 sourceIndex.column(),
                 QModelIndex());
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
