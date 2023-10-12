//
// Created by bobini on 22.08.23.
//

#include "Miss.h"

namespace gameplay_logic {
Miss::Miss(DeltaTime offsetFromStart,
           BmsPoints points,
           int column,
           int noteIndex)
  : offsetFromStart(offsetFromStart)
  , points(points)
  , column(column)
  , noteIndex(noteIndex)
{
}
auto
Miss::getOffsetFromStart() const -> DeltaTime
{
    return offsetFromStart;
}
auto
Miss::getColumn() const -> int
{
    return column;
}
auto
Miss::getNoteIndex() const -> int
{
    return noteIndex;
}
auto
Miss::getPoints() const -> BmsPoints
{
    return points;
}
auto
operator<<(QDataStream& stream, const Miss& miss) -> QDataStream&
{
    stream << static_cast<qint64>(miss.offsetFromStart) << miss.points
           << miss.column << miss.noteIndex;
    return stream;
}
auto
operator>>(QDataStream& stream, Miss& miss) -> QDataStream&
{
    qint64 offsetFromStart;
    stream >> offsetFromStart >> miss.points >> miss.column >> miss.noteIndex;
    miss.offsetFromStart = offsetFromStart;
    return stream;
}

} // namespace gameplay_logic