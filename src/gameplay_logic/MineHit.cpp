//
// Created by bobini on 15.10.23.
//

#include "MineHit.h"
gameplay_logic::MineHit::MineHit(int64_t offsetFromStart,
                                 int64_t hitOffset,
                                 double penalty,
                                 int column,
                                 int noteIndex)
  : offsetFromStart(offsetFromStart)
  , hitOffset(hitOffset)
  , penalty(penalty)
  , column(column)
  , noteIndex(noteIndex)
{
}
auto
gameplay_logic::MineHit::getOffsetFromStart() const -> int64_t
{
    return offsetFromStart;
}
auto
gameplay_logic::MineHit::getHitOffset() const -> int64_t
{
    return hitOffset;
}
auto
gameplay_logic::MineHit::getPenalty() const -> double
{
    return penalty;
}
auto
gameplay_logic::MineHit::getColumn() const -> int
{
    return column;
}
auto
gameplay_logic::MineHit::getNoteIndex() const -> int
{
    return noteIndex;
}
