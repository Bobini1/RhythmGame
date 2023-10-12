//
// Created by bobini on 22.08.23.
//

#include "Tap.h"
namespace gameplay_logic {
auto
Tap::getOffsetFromStart() const -> DeltaTime
{
    return offsetFromStart;
}
auto
Tap::getPoints() const -> QVariant
{
    return points.has_value() ? QVariant::fromValue(points.value())
                              : QVariant();
}
auto
Tap::getColumn() const -> int
{
    return column;
}
Tap::Tap(int column,
         std::optional<int> noteIndex,
         DeltaTime offsetFromStart,
         std::optional<BmsPoints> points)
  : offsetFromStart(offsetFromStart)
  , points(points)
  , noteIndex(noteIndex)
  , column(column)
{
}
auto
Tap::getNoteIndex() const -> int
{
    return noteIndex.value_or(-1);
}
auto
Tap::getPointsOptional() const -> std::optional<BmsPoints>
{
    return points;
}
auto
operator<<(QDataStream& stream, const Tap& tap) -> QDataStream&
{
    QVariant points = tap.getPoints();
    stream << static_cast<qint64>(tap.offsetFromStart) << points << tap.column
           << tap.getNoteIndex();
    return stream;
}
auto
operator>>(QDataStream& stream, Tap& tap) -> QDataStream&
{
    qint64 offsetFromStart;
    QVariant points;
    int column;
    int noteIndex;
    stream >> offsetFromStart >> points >> column >> noteIndex;
    tap.offsetFromStart = offsetFromStart;
    tap.points =
      points.isNull() ? std::nullopt : std::optional(points.value<BmsPoints>());
    tap.column = column;
    tap.noteIndex = noteIndex == -1 ? std::nullopt : std::optional(noteIndex);
    return stream;
}
} // namespace gameplay_logic