//
// Created by PC on 06/02/2025.
//

#include "NoteState.h"

#include <cmath>

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
void
ColumnState::setHoldingLongNote(bool holdingLongNote)
{
    if (this->holdingLongNote == holdingLongNote) {
        return;
    }
    this->holdingLongNote = holdingLongNote;
    emit holdingLongNoteChanged();
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
    for (int i = 0; i < this->notes.size(); ++i) {
        auto& noteState = this->notes[i];
        if (noteState.note.type != Note::Type::LongNoteBegin) {
            continue;
        }
        const auto otherEndIndex =
          mapTimeIndexToPositionIndex(noteState.index + 1);
        const auto& otherEnd = this->notes[otherEndIndex];
        noteState.longNoteEndPosition = otherEnd.note.time.position;
        auto lo =
          std::min(noteState.note.time.position, otherEnd.note.time.position);
        auto hi =
          std::max(noteState.note.time.position, otherEnd.note.time.position);
        auto set = std::set<qint64>();
        set.insert(noteState.index);
        lnBodies +=
          boost::icl::interval_map<double, std::set<qint64>>::value_type{
              boost::icl::interval<double>::closed(lo, hi),
              std::set{ static_cast<qint64>(
                timeToPositionIndexMapping[noteState.index]) }
          };
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
    const auto& note = notes.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
            return QVariant::fromValue(note);
        case NoteTypeRole:
            return QVariant::fromValue(note.note.type);
        case NotePositionRole:
            return note.note.time.position;
        case NoteHitRole:
            return note.hit;
        case NoteOtherEndHitRole:
            return note.otherEndHit;
        case BelowBottomRole:
            return note.belowBottom;
        case LongNoteEndPositionRole:
            return note.longNoteEndPosition;
        case VisibleNoteRole:
            return note.note.type == Note::Type::LongNoteBegin ||
                   note.note.type == Note::Type::LongNoteEnd || !note.hit;
        case HeldLongNoteRole:
            return note.note.type == Note::Type::LongNoteBegin && note.hit &&
                   !note.otherEndHit;
        case StaticLongNoteCandidateRole:
            return note.note.type == Note::Type::LongNoteBegin &&
                   ((note.hit && !note.otherEndHit) || note.belowBottom);
        default:
            return {};
    }
}

auto
ColumnState::roleNames() const -> QHash<int, QByteArray>
{
    return {
        { Qt::DisplayRole, "display" },
        { NoteTypeRole, "noteType" },
        { NotePositionRole, "notePosition" },
        { NoteHitRole, "noteHit" },
        { NoteOtherEndHitRole, "noteOtherEndHit" },
        { BelowBottomRole, "belowBottom" },
        { LongNoteEndPositionRole, "longNoteEndPosition" },
        { VisibleNoteRole, "visibleNote" },
        { HeldLongNoteRole, "heldLongNote" },
        { StaticLongNoteCandidateRole, "staticLongNoteCandidate" },
    };
}
void
ColumnState::onHitEvent(HitEvent hit)
{
    if (hit.getAction() == HitEvent::Action::Press) {
        setPressed(true);
    } else if (hit.getAction() == HitEvent::Action::Release) {
        setPressed(false);
        setHoldingLongNote(false);
    }
    if (hit.getNoteIndex() == -1 ||
        hit.getPointsOptional()->getJudgement() == Judgement::EmptyPoor) {
        return;
    }
    auto& note = notes[mapTimeIndexToPositionIndex(hit.getNoteIndex())];
    note.hitData = QVariant::fromValue(hit);
    note.hit = true;
    const auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex());
    emit dataChanged(index(changedIndex),
                     index(changedIndex),
                     { Qt::DisplayRole,
                       NoteHitRole,
                       VisibleNoteRole,
                       HeldLongNoteRole,
                       StaticLongNoteCandidateRole });
    if (note.note.type == Note::Type::LongNoteBegin) {
        if (hit.getAction() == HitEvent::Action::Press &&
            hit.getNoteRemoved()) {
            setHoldingLongNote(true);
        }
        auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex() + 1);
        auto& nextNote = notes[changedIndex];
        nextNote.otherEndHitData = note.hitData;
        nextNote.otherEndHit = true;
        emit dataChanged(index(changedIndex),
                         index(changedIndex),
                         { Qt::DisplayRole,
                           NoteOtherEndHitRole,
                           HeldLongNoteRole,
                           StaticLongNoteCandidateRole });
    } else if (note.note.type == Note::Type::LongNoteEnd) {
        setHoldingLongNote(false);
        auto changedIndex = mapTimeIndexToPositionIndex(hit.getNoteIndex() - 1);
        auto& prevNote = notes[changedIndex];
        prevNote.otherEndHitData = note.hitData;
        prevNote.otherEndHit = true;
        emit dataChanged(index(changedIndex),
                         index(changedIndex),
                         { Qt::DisplayRole,
                           NoteOtherEndHitRole,
                           HeldLongNoteRole,
                           StaticLongNoteCandidateRole });
    }
}
auto
ColumnState::isPressed() const -> bool
{
    return pressed;
}
auto
ColumnState::isHoldingLongNote() const -> bool
{
    return holdingLongNote;
}
auto
ColumnState::mapTimeIndexToPositionIndex(int64_t timeIndex) const -> int
{
    return timeToPositionIndexMapping[timeIndex];
}
BarLinesState::BarLinesState(QList<BarLineState> barLines, QObject* parent)
  : QAbstractListModel(parent)
  , barLines(std::move(barLines))
{
    std::ranges::sort(this->barLines, [](const auto& a, const auto& b) {
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
auto
BarLinesState::getBarlines() const -> const QList<BarLineState>&
{
    return barLines;
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
Filter::setHoldingLongNote(bool holdingLongNote)
{
    if (this->holdingLongNote == holdingLongNote) {
        return;
    }
    this->holdingLongNote = holdingLongNote;
    emit holdingLongNoteChanged();
}
auto
ColumnState::getLnBeginBounds(double position) const
  -> std::optional<std::pair<int, int>>
{
    auto it = lnBodies.find(position);
    if (it == lnBodies.end()) {
        return std::nullopt;
    }
    return std::pair{ static_cast<int>(*it->second.begin()),
                      static_cast<int>(*it->second.rbegin()) };
}
Filter::Filter(ColumnState* columnState, QObject* parent)
  : QAbstractProxyModel(parent)
  , columnState(columnState)
{
    setSourceModel(columnState);
    setPressed(columnState->isPressed());
    setHoldingLongNote(columnState->isHoldingLongNote());
    connect(columnState,
            &ColumnState::pressedChanged,
            this,
            [this, columnState]() { setPressed(columnState->isPressed()); });
    connect(columnState,
            &ColumnState::holdingLongNoteChanged,
            this,
            [this, columnState]() {
                setHoldingLongNote(columnState->isHoldingLongNote());
            });
    connect(columnState,
            &ColumnState::dataChanged,
            this,
            [this](const QModelIndex& topLeft,
                   const QModelIndex& bottomRight,
                   const QVector<int>& roles) {
                const auto first = std::max(topLeft.row(), firstRow);
                const auto last = std::min(bottomRight.row(), endRow - 1);
                if (first <= last) {
                    emit dataChanged(
                      index(first - firstRow), index(last - firstRow), roles);
                }
            });
    setVisibleRange(0, 0);
}
BarlineFilter::BarlineFilter(BarLinesState* barLinesState, QObject* parent)
  : QAbstractProxyModel(parent)
{
    setSourceModel(barLinesState);
    // forward dataChanged from source to proxy with row offset
    connect(barLinesState,
            &BarLinesState::dataChanged,
            this,
            [this](const QModelIndex& topLeft,
                   const QModelIndex& bottomRight,
                   const QVector<int>& roles) {
                emit dataChanged(index(topLeft.row() - bottomRow),
                                 index(bottomRight.row() - bottomRow),
                                 roles);
            });
}
void
BarlineFilter::setTopPosition(double value)
{
    if (topPosition == value) {
        return;
    }

    topPosition = value;
    emit topPositionChanged();
    if (value < bottomPosition) {
        setBottomPosition(value);
    }

    const auto& barlines =
      static_cast<BarLinesState*>(sourceModel())->getBarlines();
    const auto upper =
      std::upper_bound(barlines.begin(),
                       barlines.end(),
                       value,
                       [](const double value, const auto& barline) {
                           return value < barline.time.position;
                       });
    const auto newTopRow =
      static_cast<int>(std::distance(barlines.begin(), upper));
    const auto oldTopRow = topRow;
    if (newTopRow > topRow) {
        beginInsertRows(
          QModelIndex(), topRow - bottomRow, newTopRow - bottomRow - 1);
        topRow = newTopRow;
        endInsertRows();
    } else if (newTopRow < topRow) {
        beginRemoveRows(
          QModelIndex(), newTopRow - bottomRow, topRow - bottomRow - 1);
        topRow = newTopRow;
        endRemoveRows();
    }
}
void
BarlineFilter::setBottomPosition(double value)
{
    if (bottomPosition == value) {
        return;
    }

    bottomPosition = value;
    emit bottomPositionChanged();
    if (value > topPosition) {
        setTopPosition(value);
    }

    const auto& barlines =
      static_cast<BarLinesState*>(sourceModel())->getBarlines();
    auto lower = std::lower_bound(
      barlines.begin(),
      barlines.end(),
      value,
      [](const auto& bl, double value) { return bl.time.position < value; });

    const auto newBottomRow =
      static_cast<int>(std::distance(barlines.begin(), lower));
    const auto oldBottomRow = bottomRow;
    if (newBottomRow > bottomRow) {
        beginRemoveRows(QModelIndex(), 0, newBottomRow - bottomRow - 1);
        bottomRow = newBottomRow;
        endRemoveRows();
    } else if (newBottomRow < bottomRow) {
        beginInsertRows(QModelIndex(), 0, bottomRow - newBottomRow - 1);
        bottomRow = newBottomRow;
        endInsertRows();
    }
}
void
BarlineFilter::setVisibleRange(double bottom, double top)
{
    setTopPosition(top);
    setBottomPosition(bottom);
}

QModelIndex
BarlineFilter::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.row() < 0 ||
        proxyIndex.row() >= rowCount({})) {
        return QModelIndex();
    }
    return sourceModel()->index(
      proxyIndex.row() + bottomRow, proxyIndex.column(), QModelIndex());
}

QModelIndex
BarlineFilter::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != sourceModel()) {
        return QModelIndex();
    }
    return index(
      sourceIndex.row() - bottomRow, sourceIndex.column(), QModelIndex());
}

QModelIndex
BarlineFilter::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= topRow - bottomRow || column != 0) {
        return QModelIndex();
    }
    // store pointer to the BarLineState inside the model index
    const auto& bl = static_cast<BarLinesState*>(sourceModel())
                       ->getBarlines()
                       .at(row + bottomRow);
    return createIndex(row, column, const_cast<BarLineState*>(&bl));
}

QModelIndex
BarlineFilter::parent(const QModelIndex& child) const
{
    return QModelIndex();
}

int
BarlineFilter::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(topRow - bottomRow);
}

int
BarlineFilter::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant
BarlineFilter::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        const auto src =
          sourceModel()->index(index.row() + bottomRow, 0, QModelIndex());
        return sourceModel()->data(src, role);
    }
    return {};
}
void
Filter::setTopPosition(double value)
{
    setVisibleRange(std::min(bottomPosition, value), value);
}
void
Filter::setBottomPosition(double value)
{
    setVisibleRange(value, std::max(topPosition, value));
}
void
Filter::setRows(int first, int end)
{
    // Keep overlapping delegates, and remove the old range before a jump.
    if (first >= endRow || end <= firstRow) {
        if (endRow > firstRow) {
            beginRemoveRows({}, 0, endRow - firstRow - 1);
            endRow = firstRow;
            endRemoveRows();
        }
        firstRow = endRow = first;
    }
    if (end < endRow) {
        beginRemoveRows({}, end - firstRow, endRow - firstRow - 1);
        endRow = end;
        endRemoveRows();
    }
    if (first > firstRow) {
        beginRemoveRows({}, 0, first - firstRow - 1);
        firstRow = first;
        endRemoveRows();
    }
    if (first < firstRow) {
        beginInsertRows({}, 0, firstRow - first - 1);
        firstRow = first;
        endInsertRows();
    }
    if (end > endRow) {
        beginInsertRows({}, endRow - firstRow, end - firstRow - 1);
        endRow = end;
        endInsertRows();
    }
}
void
Filter::setVisibleRange(double bottom, double top)
{
    top = std::max(bottom, top);
    const auto bottomChanged = bottomPosition != bottom;
    const auto topChanged = topPosition != top;
    bottomPosition = bottom;
    topPosition = top;

    const auto& notes = columnState->getNotes();
    const auto lower = std::lower_bound(
      notes.begin(), notes.end(), bottom, [](const auto& note, double value) {
          return note.note.time.position < value;
      });
    const auto upper = std::upper_bound(
      notes.begin(), notes.end(), top, [](double value, const auto& note) {
          return value < note.note.time.position;
      });
    const auto newBottomRow = static_cast<int>(lower - notes.begin());
    auto first = newBottomRow;
    auto end = static_cast<int>(upper - notes.begin());
    // A body may enter the view from either edge, depending on scroll
    // direction.
    if (const auto bounds = columnState->getLnBeginBounds(bottom)) {
        first = std::min(first, bounds->first);
    }
    if (const auto bounds = columnState->getLnBeginBounds(top)) {
        end = std::max(end, bounds->second + 1);
    }

    if (newBottomRow != bottomRow) {
        const auto changedFirst = std::min(bottomRow, newBottomRow);
        const auto changedEnd = std::max(bottomRow, newBottomRow);
        for (int i = changedFirst; i < changedEnd; ++i) {
            columnState->getNotes()[i].belowBottom = i < newBottomRow;
        }
        bottomRow = newBottomRow;
        emit columnState->dataChanged(
          columnState->index(changedFirst),
          columnState->index(changedEnd - 1),
          { Qt::DisplayRole,
            ColumnState::BelowBottomRole,
            ColumnState::StaticLongNoteCandidateRole });
    }
    setRows(first, end);
    if (bottomChanged)
        emit bottomPositionChanged();
    if (topChanged)
        emit topPositionChanged();
}
QModelIndex
Filter::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || row >= endRow - firstRow ||
        column != 0) {
        return {};
    }
    return createIndex(
      row, column, &columnState->getNotes().at(row + firstRow));
}
QModelIndex
Filter::parent(const QModelIndex& child) const
{
    return {};
}
int
Filter::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : endRow - firstRow;
}
int
Filter::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}
QModelIndex
Filter::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.model() != this ||
        proxyIndex.row() < 0 || proxyIndex.row() >= rowCount({})) {
        return {};
    }
    return columnState->index(proxyIndex.row() + firstRow, proxyIndex.column());
}
QModelIndex
Filter::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != columnState) {
        return {};
    }
    return index(sourceIndex.row() - firstRow, sourceIndex.column());
}
GameplayState::GameplayState(QList<ColumnState*> columnStates,
                             BarLinesState* barLinesState,
                             QObject* parent)
  : QObject(parent)
  , barLinesState(barLinesState)
  , barLineFilter(new BarlineFilter(barLinesState, this))
  , columnStates(std::move(columnStates))
{
    for (auto* const columnState : this->columnStates) {
        auto* filter = new Filter(columnState, this);
        columnState->setParent(filter);
        columnFilters.append(filter);
    }

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
int
GameplayState::noteTypeAt(int column, int noteIndex) const
{
    if (column < 0 || column >= columnStates.size() || noteIndex < 0) {
        return -1;
    }
    const auto* columnState = columnStates.at(column);
    const auto& notes = columnState->getNotes();
    if (noteIndex >= notes.size()) {
        return -1;
    }
    const auto mappedIndex =
      columnState->mapTimeIndexToPositionIndex(noteIndex);
    return mappedIndex >= 0 && mappedIndex < notes.size()
             ? static_cast<int>(notes.at(mappedIndex).note.type)
             : -1;
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
void
GameplayState::setPosition(double value)
{
    if (position == value) {
        return;
    }
    position = value;
    updateVisibleRanges();
}
void
GameplayState::setVisiblePositionSpans(double noteSpan, double barLineSpan)
{
    if (!std::isfinite(noteSpan) || noteSpan < 0.0 ||
        !std::isfinite(barLineSpan) || barLineSpan < 0.0) {
        return;
    }
    if (noteVisiblePositionSpan == noteSpan &&
        barLineVisiblePositionSpan == barLineSpan) {
        return;
    }
    noteVisiblePositionSpan = noteSpan;
    barLineVisiblePositionSpan = barLineSpan;
    updateVisibleRanges();
}
void
GameplayState::updateVisibleRanges()
{
    if (!noteVisiblePositionSpan || !barLineVisiblePositionSpan) {
        return;
    }
    for (auto* const filter : columnFilters) {
        filter->setVisibleRange(position, position + *noteVisiblePositionSpan);
    }
    barLineFilter->setVisibleRange(position,
                                   position + *barLineVisiblePositionSpan);
}
} // namespace gameplay_logic
