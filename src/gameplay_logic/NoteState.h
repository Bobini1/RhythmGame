//
// Created by PC on 06/02/2025.
//

#ifndef NOTESTATE_H
#define NOTESTATE_H

#include "BmsNotes.h"
#include "HitEvent.h"

#include <QAbstractListModel>

namespace gameplay_logic {
class NoteState
{
    Q_GADGET
    Q_PROPERTY(Note note MEMBER note)
    Q_PROPERTY(bool belowJudgeline MEMBER belowJudgeline)
    Q_PROPERTY(QVariant hitData MEMBER hitData)
  public:
    Note note;
    QVariant hitData = QVariant::fromValue(nullptr);
    bool belowJudgeline = false;
};

class ColumnState final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

    QList<NoteState> notes;
    // the last invisible note
    decltype(notes)::size_type currentNote = -1;
    int64_t elapsed{};
    bool pressed = false;
    void setPressed(bool pressed);

  public:
    explicit ColumnState(QList<NoteState> notes, QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    void onHitEvent(HitEvent hit);
    // don't set it back in time!
    void setElapsed(int64_t nanos);
    auto isPressed() const -> bool;
  signals:
    void pressedChanged();
};

class BarLineState
{
    Q_GADGET
    Q_PROPERTY(Time time MEMBER time)
    Q_PROPERTY(bool belowJudgeline MEMBER belowJudgeline)

  public:
    Time time;
    bool belowJudgeline = false;
};

class BarLinesState final : public QAbstractListModel
{
    Q_OBJECT
    QList<BarLineState> barLines;
    // the last invisible barline
    decltype(barLines)::size_type currentLine = -1;
    int64_t elapsed{};

  public:
    explicit BarLinesState(QList<BarLineState> barLines,
                           QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    // don't set it back in time!
    void setElapsed(int64_t nanos);
};

class GameplayState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ColumnState*> columnStates READ getColumnStates CONSTANT)
    Q_PROPERTY(BarLinesState* barLinesState READ getBarLinesState CONSTANT)

    BarLinesState* barLinesState;
    QList<ColumnState*> columnStates;

  public:
    GameplayState(QList<ColumnState*> columnStates,
                  BarLinesState* barLinesState,
                  QObject* parent = nullptr);
    auto getColumnStates() -> QList<ColumnState*>;
    auto getBarLinesState() -> BarLinesState*;
};

} // namespace gameplay_logic

#endif // NOTESTATE_H
