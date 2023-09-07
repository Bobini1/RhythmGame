//
// Created by bobini on 22.08.23.
//

#ifndef RHYTHMGAME_MISS_H
#define RHYTHMGAME_MISS_H

#include <QObject>
namespace gameplay_logic {

class Miss
{
    using DeltaTime = int64_t;
    Q_GADGET
    Q_PROPERTY(DeltaTime offsetFromStart READ getOffsetFromStart CONSTANT)
    Q_PROPERTY(int column READ getColumn CONSTANT)
    Q_PROPERTY(int noteIndex READ getNoteIndex CONSTANT)

    DeltaTime offsetFromStart;
    int column;
    int noteIndex;

  public:
    Miss(DeltaTime offsetFromStart, int column, int noteIndex);

    auto getOffsetFromStart() const -> DeltaTime;
    auto getColumn() const -> int;
    auto getNoteIndex() const -> int;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_MISS_H
