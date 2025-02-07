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
    auto& note = notes[(hit.getNoteIndex())];
    note.hitData = QVariant::fromValue(hit);
    switch (hit.getType()) {
        case HitEvent::HitType::NothingHit:
        case HitEvent::HitType::Hit:
            setPressed(/*pressed=*/true);
            break;
        case HitEvent::HitType::NothingRelease:
        case HitEvent::HitType::LnEndHit:
            setPressed(/*pressed=*/false);
            break;
        default:
            break;
    }
    emit dataChanged(index(hit.getNoteIndex()), index(hit.getNoteIndex()));
}
void
ColumnState::modifyVisibility(decltype(notes)::size_type bottomIndex)
{
    auto top = elapsed + noteScreenTime;
    auto topIndex = bottomIndex;
    for (auto i = currentNote + 1; i < notes.size(); i++) {
        auto& note = notes[i];
        if (note.note.time.timestamp > top) {
            if (note.visible) {
                note.visible = false;
                if (bottomIndex == -1) {
                    bottomIndex = i;
                }
                topIndex = i;
            } else {
                break;
            }
        } else if (!note.visible) {
            note.visible = true;
            if (bottomIndex == -1) {
                bottomIndex = i;
            }
            topIndex = i;
        }
    }
    if (bottomIndex != -1) {
        emit dataChanged(index(bottomIndex), index(topIndex));
    }
}
void
ColumnState::setNoteScreenTime(int64_t nanos)
{
    noteScreenTime = nanos;
    modifyVisibility(-1);
}
void
ColumnState::setElapsed(int64_t nanos)
{
    elapsed = nanos;
    if (notes.empty()) {
        return;
    }
    auto bottomIndex = -1;
    for (auto i = currentNote + 1; i < notes.size(); i++) {
        if (notes[i].note.time.timestamp >= elapsed) {
            break;
        }
        notes[i].visible = false;
        currentNote = i;
        bottomIndex = i;
    }
    modifyVisibility(bottomIndex);
}
auto
ColumnState::isPressed() -> bool
{
    return pressed;
}
void
BarLinesState::modifyVisibility(decltype(barLines)::size_type bottomIndex)
{
    auto top = elapsed + noteScreenTime;
    auto topIndex = bottomIndex;
    for (auto i = currentLine + 1; i < barLines.size(); i++) {
        auto& barLine = barLines[i];
        if (barLine.time.timestamp > top) {
            if (barLine.visible) {
                barLine.visible = false;
                if (bottomIndex == -1) {
                    bottomIndex = i;
                }
                topIndex = i;
            } else {
                break;
            }
        } else if (!barLine.visible) {
            barLine.visible = true;
            if (bottomIndex == -1) {
                bottomIndex = i;
            }
            topIndex = i;
        }
    }
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
BarLinesState::onNoteScreenTimeChanged(int64_t nanos)
{
    noteScreenTime = nanos;
    modifyVisibility(-1);
}
void
BarLinesState::onElapsedChanged(int64_t nanos)
{
    elapsed = nanos;
    if (barLines.empty()) {
        return;
    }
    auto bottomIndex = -1;
    for (auto i = currentLine + 1; i < barLines.size(); i++) {
        if (barLines[i].time.timestamp >= elapsed) {
            break;
        }
        barLines[i].visible = false;
        currentLine = i;
        bottomIndex = i;
    }
    modifyVisibility(bottomIndex);
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
void
GameplayState::onHitEvent(HitEvent hit)
{
    columnStates.at(hit.getColumn())->onHitEvent(hit);
}
} // namespace gameplay_logic