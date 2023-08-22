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
} // namespace gameplay_logic