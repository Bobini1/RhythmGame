//
// Created by PC on 06/02/2025.
//

#ifndef NOTESTATE_H
#define NOTESTATE_H

#include "BmsNotes.h"
#include "HitEvent.h"

#include <QSortFilterProxyModel>

namespace gameplay_logic {
class NoteState
{
    Q_GADGET
    Q_PROPERTY(Note note MEMBER note)
    Q_PROPERTY(bool belowJudgeline MEMBER belowJudgeline)
    Q_PROPERTY(QVariant hitData MEMBER hitData)
    Q_PROPERTY(QVariant otherEndHitData MEMBER otherEndHitData)
  public:
    Note note;
    QVariant hitData = QVariant::fromValue(nullptr);
    QVariant otherEndHitData = QVariant::fromValue(nullptr);
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

class Filter : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(double topPosition READ getTopPosition WRITE setTopPosition
                 NOTIFY topPositionChanged)
    Q_PROPERTY(double bottomPosition READ getBottomPosition WRITE
                 setBottomPosition NOTIFY bottomPositionChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

    double topPosition = 0.0;
    double bottomPosition = 0.0;
    bool pressed = false;
    void setPressed(bool pressed);

  public:
    explicit Filter(ColumnState* columnState, QObject* parent = nullptr);

    auto getTopPosition() const -> double { return topPosition; }
    void setTopPosition(double value);
    auto getBottomPosition() const -> double { return bottomPosition; }
    void setBottomPosition(double value);
    auto isPressed() const -> bool { return pressed; }
    Q_INVOKABLE int getRealIndex(int sourceRow) const;
  signals:
    void topPositionChanged();
    void bottomPositionChanged();
    void pressedChanged();

  protected:
    auto filterAcceptsRow(int source_row,
                          const QModelIndex& source_parent) const
      -> bool override;
};

class BarlineFilter : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(double topPosition READ getTopPosition WRITE setTopPosition
                 NOTIFY topPositionChanged)
    Q_PROPERTY(double bottomPosition READ getBottomPosition WRITE
                 setBottomPosition NOTIFY bottomPositionChanged)

    double topPosition = 0.0;
    double bottomPosition = 0.0;

  public:
    explicit BarlineFilter(BarLinesState* barLinesState, QObject* parent = nullptr);

    auto getTopPosition() const -> double { return topPosition; }
    void setTopPosition(double value);
    auto getBottomPosition() const -> double { return bottomPosition; }
    void setBottomPosition(double value);
  signals:
    void topPositionChanged();
    void bottomPositionChanged();

  protected:
    auto filterAcceptsRow(int source_row,
                          const QModelIndex& source_parent) const
      -> bool override;
};

class GameplayState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<Filter*> columnStates READ getColumnFilters CONSTANT)
    Q_PROPERTY(BarlineFilter* barLinesState READ getBarLineFilter CONSTANT)

    BarLinesState* barLinesState;
    BarlineFilter* barLineFilter = nullptr;
    QList<ColumnState*> columnStates;
    QList<Filter*> columnFilters;

  public:
    GameplayState(QList<ColumnState*> columnStates,
                  BarLinesState* barLinesState,
                  QObject* parent = nullptr);
    auto getColumnStates() -> QList<ColumnState*>;
    auto getColumnFilters() -> QList<Filter*>;
    auto getBarLinesState() const -> BarLinesState*;
    auto getBarLineFilter() const -> BarlineFilter*;
};

} // namespace gameplay_logic

#endif // NOTESTATE_H
