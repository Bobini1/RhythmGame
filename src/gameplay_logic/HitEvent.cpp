//
// Created by bobini on 22.08.23.
//

#include "HitEvent.h"
namespace gameplay_logic {
auto
HitEvent::getOffsetFromStart() const -> DeltaTime
{
    return offsetFromStart;
}
auto
HitEvent::getPoints() const -> QVariant
{
    return points.has_value() ? QVariant::fromValue(points.value())
                              : QVariant();
}
auto
HitEvent::getColumn() const -> int
{
    return column;
}
HitEvent::HitEvent(int column,
                   std::optional<int> noteIndex,
                   DeltaTime offsetFromStart,
                   std::optional<BmsPoints> points,
                   Action action,
                   bool noteRemoved)
  : offsetFromStart(offsetFromStart)
  , points(points)
  , noteIndex(noteIndex)
  , column(column)
  , action(action)
  , noteRemoved(noteRemoved)
{
}
auto
HitEvent::getNoteIndex() const -> int
{
    return noteIndex.value_or(-1);
}
auto
HitEvent::getAction() const -> Action
{
    return action;
}
auto
HitEvent::getNoteRemoved() const -> bool
{
    return noteRemoved;
}
void
HitEvent::setAction(Action newAction)
{
    action = newAction;
}
void
HitEvent::setPoints(std::optional<BmsPoints> newPoints)
{
    points = newPoints;
}
void
HitEvent::setNoteRemoved(bool newNoteRemoved)
{
    noteRemoved = newNoteRemoved;
}
auto
HitEvent::getPointsOptional() const -> std::optional<BmsPoints>
{
    return points;
}
auto
operator<<(QDataStream& stream, const HitEvent& tap) -> QDataStream&
{
    auto points = tap.getPoints();
    stream << static_cast<qint64>(tap.offsetFromStart) << points << tap.column
           << tap.getNoteIndex() << tap.action << tap.noteRemoved;
    return stream;
}
auto
operator>>(QDataStream& stream, HitEvent& tap) -> QDataStream&
{
    qint64 offsetFromStart;
    QVariant points;
    int column;
    int noteIndex;
    HitEvent::Action action;
    bool noteRemoved;
    stream >> offsetFromStart >> points >> column >> noteIndex >> action >>
      noteRemoved;
    tap.offsetFromStart = offsetFromStart;
    tap.points =
      points.isNull() ? std::nullopt : std::optional(points.value<BmsPoints>());
    tap.column = column;
    tap.noteIndex = noteIndex == -1 ? std::nullopt : std::optional(noteIndex);
    tap.action = action;
    tap.noteRemoved = noteRemoved;
    return stream;
}
auto
HitEvent::getHitOffset() const -> DeltaTime
{
    if (points.has_value()) {
        return offsetFromStart + points->getDeviation();
    }
    return offsetFromStart;
}
} // namespace gameplay_logic