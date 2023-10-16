//
// Created by bobini on 15.10.23.
//

#ifndef RHYTHMGAME_MINEHIT_H
#define RHYTHMGAME_MINEHIT_H
#include <chrono>
#include <QObject>
namespace gameplay_logic {
class MineHit
{
    Q_GADGET
    Q_PROPERTY(int64_t offsetFromStart READ getOffsetFromStart CONSTANT)
    Q_PROPERTY(int64_t hitOffset READ getHitOffset CONSTANT)
    Q_PROPERTY(double penalty READ getPenalty CONSTANT)
    Q_PROPERTY(int column READ getColumn CONSTANT)
    Q_PROPERTY(int noteIndex READ getNoteIndex CONSTANT)
    int64_t offsetFromStart;
    int64_t hitOffset;
    double penalty;
    int column;
    int noteIndex;

  public:
    MineHit(int64_t offsetFromStart,
            int64_t hitOffset,
            double penalty,
            int column,
            int noteIndex);
    MineHit() = default;

    auto getOffsetFromStart() const -> int64_t;
    auto getHitOffset() const -> int64_t;
    auto getPenalty() const -> double;
    auto getColumn() const -> int;
    auto getNoteIndex() const -> int;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_MINEHIT_H
