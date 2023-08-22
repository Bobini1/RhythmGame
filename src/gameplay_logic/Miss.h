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
    Q_PROPERTY(int Column READ getColumn CONSTANT)

    DeltaTime offsetFromStart;
    int column;

  public:
    Miss(DeltaTime offsetFromStart, int column);

    auto getOffsetFromStart() const -> DeltaTime;
    auto getColumn() const -> int;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_MISS_H
