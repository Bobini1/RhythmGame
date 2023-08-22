//
// Created by bobini on 22.08.23.
//

#ifndef RHYTHMGAME_TAP_H
#define RHYTHMGAME_TAP_H
#include <QObject>
#include <QVariant>
#include "BmsPoints.h"
namespace gameplay_logic {
class Tap
{
    // nanoseconds
    using DeltaTime = int64_t;
    Q_GADGET
    Q_PROPERTY(DeltaTime offsetFromStart READ getOffsetFromStart CONSTANT)
    Q_PROPERTY(QVariant points READ getPoints CONSTANT)
    Q_PROPERTY(int column READ getColumn CONSTANT)
    Q_PROPERTY(int noteIndex READ getNoteIndex CONSTANT)

    DeltaTime offsetFromStart;
    std::optional<BmsPoints> points;
    std::optional<int> noteIndex;
    int column;

  public:
    Tap(int column,
        std::optional<int> noteIndex,
        DeltaTime offsetFromStart,
        std::optional<BmsPoints> points);

    auto getOffsetFromStart() const -> DeltaTime;
    // if the tap did not hit a note, this returns null
    auto getPoints() const -> QVariant;
    auto getPointsOptional() const -> std::optional<BmsPoints>;
    auto getColumn() const -> int;
    // if the tap did not hit a note, this returns -1
    auto getNoteIndex() const -> int;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_TAP_H
