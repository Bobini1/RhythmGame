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
    if (hit.getNoteIndex() == -1 || hit.getPointsOptional()->getJudgement() == Judgement::EmptyPoor) {
        return;
    }
    auto& note = notes[hit.getNoteIndex()];
    note.hitData = QVariant::fromValue(hit);
    emit dataChanged(index(hit.getNoteIndex()), index(hit.getNoteIndex()));
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
GameplayState::GameplayState(QList<ColumnState*> columnStates,
                             BarLinesState* barLinesState,
                             QObject* parent)
  : QObject(parent)
  , barLinesState(barLinesState)
  , columnStates(std::move(columnStates))
{
    for (auto* const columnState : this->columnStates) {
        columnState->setParent(this);
    }
    barLinesState->setParent(this);
}
auto
GameplayState::getColumnStates() -> QList<ColumnState*>
{
    return columnStates;
}
auto
GameplayState::getBarLinesState() -> BarLinesState*
{
    return barLinesState;
}
} // namespace gameplay_logic