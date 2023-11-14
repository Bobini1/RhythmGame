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
  , deviation(hitOffset)
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
gameplay_logic::MineHit::getDeviation() const -> int64_t
{
    return deviation;
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
auto
gameplay_logic::operator<<(QDataStream& stream,
                           const gameplay_logic::MineHit& hit) -> QDataStream&
{
    return stream << static_cast<qint64>(hit.offsetFromStart)
                  << static_cast<qint64>(hit.deviation) << hit.penalty
                  << hit.column << hit.noteIndex;
}
auto
gameplay_logic::operator>>(QDataStream& stream, gameplay_logic::MineHit& hit)
  -> QDataStream&
{
    qint64 offsetFromStart;
    qint64 deviation;
    stream >> offsetFromStart >> deviation >> hit.penalty >> hit.column >>
      hit.noteIndex;
    hit.offsetFromStart = offsetFromStart;
    hit.deviation = deviation;
    return stream;
}
auto
gameplay_logic::MineHit::getHitOffset() const -> int64_t
{
    return offsetFromStart + deviation;
}
