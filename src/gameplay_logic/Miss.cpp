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
} // namespace gameplay_logic