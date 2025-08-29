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
    Q_PROPERTY(qint64 index MEMBER index)
    /**
     * The latest hit data for this note.
     */
    Q_PROPERTY(QVariant hitData MEMBER hitData)
    /**
     * The latest hit data for the other end of this note, if it is an LN begin
     * or LN end.
     */
    Q_PROPERTY(QVariant otherEndHitData MEMBER otherEndHitData)
    /**
     * Whether this note is below the bottom of the visible area.
     */
    Q_PROPERTY(bool belowBottom MEMBER belowBottom)
  public:
    Note note;
    qint64 index;
    QVariant hitData = QVariant::fromValue(nullptr);
    QVariant otherEndHitData = QVariant::fromValue(nullptr);
    bool belowBottom;
};

/**
 * @brief The state of a single column, including its notes and whether it is
 * currently pressed.
 * @see NoteState
 */
class ColumnState final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

    QList<NoteState> notes;
    int64_t elapsed{};
    bool pressed = false;
    void setPressed(bool pressed);

  public:
    explicit ColumnState(QList<NoteState> notes, QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    void onHitEvent(HitEvent hit);
    auto isPressed() const -> bool;
    auto getNotes() const -> const QList<NoteState>& { return notes; }
    auto getNotes() -> QList<NoteState>& { return notes; }
  signals:
    void pressedChanged();
};

class BarLineState
{
    Q_GADGET
    Q_PROPERTY(Time time MEMBER time)
    Q_PROPERTY(qint64 index MEMBER index)

  public:
    Time time;
    qint64 index;
};

/**
 * @brief The state of the barlines in the chart.
 * @see BarLineState
 */
class BarLinesState final : public QAbstractListModel
{
    Q_OBJECT
    QList<BarLineState> barLines;

  public:
    explicit BarLinesState(QList<BarLineState> barLines,
                           QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
};

/**
 * @brief A filter model that only shows the notes that are in the visible
 * area.
 * @see ColumnState
 */
class Filter : public QAbstractProxyModel
{
    Q_OBJECT
    /**
     * The top position of the visible area expressed in beats.
     */
    Q_PROPERTY(double topPosition READ getTopPosition WRITE setTopPosition
                 NOTIFY topPositionChanged)
    /**
     * The bottom position of the visible area expressed in beats.
     */
    Q_PROPERTY(double bottomPosition READ getBottomPosition WRITE
                 setBottomPosition NOTIFY bottomPositionChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

    double topPosition = 0.0;
    double bottomPosition = 0.0;
    int bottomRow = 0;
    int topRow = 0;
    bool pressed = false;
    void setPressed(bool pressed);
    ColumnState* columnState;

  public:
    explicit Filter(ColumnState* columnState, QObject* parent = nullptr);

    auto getTopPosition() const -> double { return topPosition; }
    void setTopPosition(double value);
    auto getBottomPosition() const -> double { return bottomPosition; }
    void setBottomPosition(double value);
    auto isPressed() const -> bool { return pressed; }
    QModelIndex index(int row,
                      int column = 0,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
  signals:
    void topPositionChanged();
    void bottomPositionChanged();
    void pressedChanged();
};

/**
 * @brief A filter model that only shows the barlines that are in the visible
 * area.
 * @see BarLinesState
 */
class BarlineFilter : public QSortFilterProxyModel
{
    Q_OBJECT
    /**
     * The top position of the visible area expressed in beats.
     */
    Q_PROPERTY(double topPosition READ getTopPosition WRITE setTopPosition
                 NOTIFY topPositionChanged)
    /**
     * The bottom position of the visible area expressed in beats.
     */
    Q_PROPERTY(double bottomPosition READ getBottomPosition WRITE
                 setBottomPosition NOTIFY bottomPositionChanged)

    double topPosition = 0.0;
    double bottomPosition = 0.0;

  public:
    explicit BarlineFilter(BarLinesState* barLinesState,
                           QObject* parent = nullptr);

    auto getTopPosition() const -> double { return topPosition; }
    void setTopPosition(double value);
    auto getBottomPosition() const -> double { return bottomPosition; }
    void setBottomPosition(double value);
  signals:
    void topPositionChanged();
    void bottomPositionChanged();

  protected:
    auto filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
      -> bool override;
};

/**
 * @brief The state of gameplay columns and barlines.
 */
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
