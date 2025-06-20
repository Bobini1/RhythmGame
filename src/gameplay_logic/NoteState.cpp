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
Filter::setPressed(bool pressed)
{
    if (this->pressed == pressed) {
        return;
    }
    this->pressed = pressed;
    emit pressedChanged();
}
Filter::Filter(ColumnState* columnState, QObject* parent)
  : QAbstractProxyModel(parent)
  , columnState(columnState)
{
    setSourceModel(columnState);
    connect(
      columnState, &ColumnState::pressedChanged, this, [this, columnState]() {
          setPressed(columnState->isPressed());
      });
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
    if (topPosition == value) {
        return;
    }
    const auto upper =
      std::upper_bound(columnState->getNotes().begin(),
                       columnState->getNotes().end(),
                       value,
                       [](double value, const auto& note) {
                           return note.note.time.position > value &&
                                  note.note.type != Note::Type::LongNoteEnd;
                       });
    auto newTopRow = std::distance(columnState->getNotes().begin(), upper);
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
    if (lower != columnState->getNotes().end() &&
        lower->note.type == Note::Type::LongNoteEnd) {
        --lower;
    }
    auto newBottomRow = std::distance(columnState->getNotes().begin(), lower);
    if (newBottomRow > bottomRow) {
        beginRemoveRows(QModelIndex(), 0, newBottomRow - bottomRow - 1);

        bottomPosition = value;
        bottomRow = newBottomRow;
        endRemoveRows();
    } else if (newBottomRow < bottomRow) {
        beginInsertRows(QModelIndex(), 0, bottomRow - newBottomRow - 1);

        bottomPosition = value;
        bottomRow = newBottomRow;
        endInsertRows();
    }
    emit bottomPositionChanged();
}
int
Filter::getRealIndex(int sourceRow) const
{
    return mapToSource(index(sourceRow, 0, QModelIndex())).row();
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