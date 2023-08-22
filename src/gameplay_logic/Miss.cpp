//
// Created by bobini on 22.08.23.
//

#include "Miss.h"

namespace gameplay_logic {
Miss::Miss(DeltaTime offsetFromStart, int column)
  : offsetFromStart(offsetFromStart)
  , column(column)
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
} // gameplay_logic