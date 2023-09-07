//
// Created by bobini on 22.08.23.
//

#include "Miss.h"

namespace gameplay_logic {
Miss::Miss(DeltaTime offsetFromStart, int column, int noteIndex)
  : offsetFromStart(offsetFromStart)
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
} // namespace gameplay_logic