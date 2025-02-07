//
// Created by bobini on 22.08.23.
//

#ifndef RHYTHMGAME_HITEVENT_H
#define RHYTHMGAME_HITEVENT_H
#include <QObject>
#include <QVariant>
#include "BmsPoints.h"
namespace gameplay_logic {
class HitEvent
{
    Q_GADGET
  public:
    enum class HitType
    {
        None,
        Hit,
        Miss,
        // When the beginning is missed
        LnEndSkip,
        Mine
    };
    Q_ENUM(HitType)
    enum class Action
    {
        None,
        Press,
        Release
    };
    Q_ENUM(Action)
  private:
    // nanoseconds
    using DeltaTime = int64_t;
    Q_PROPERTY(DeltaTime offsetFromStart READ getOffsetFromStart CONSTANT)
    Q_PROPERTY(DeltaTime hitOffset READ getHitOffset CONSTANT)
    Q_PROPERTY(QVariant points READ getPoints CONSTANT)
    Q_PROPERTY(int column READ getColumn CONSTANT)
    Q_PROPERTY(int noteIndex READ getNoteIndex CONSTANT)
    Q_PROPERTY(HitType type READ getType CONSTANT)
    Q_PROPERTY(Action action READ getAction CONSTANT)

    DeltaTime offsetFromStart;
    std::optional<BmsPoints> points;
    std::optional<int> noteIndex;
    int column;
    HitType type;
    Action action;

  public:
    HitEvent(int column,
             std::optional<int> noteIndex,
             DeltaTime offsetFromStart,
             std::optional<BmsPoints> points,
             HitType type,
             Action action);
    HitEvent() = default;

    auto getOffsetFromStart() const -> DeltaTime;
    // if the tap did not hit a note, this returns null
    auto getHitOffset() const -> DeltaTime;
    auto getPoints() const -> QVariant;
    auto getPointsOptional() const -> std::optional<BmsPoints>;
    auto getColumn() const -> int;
    // if the tap did not hit a note, this returns -1
    auto getNoteIndex() const -> int;
    auto getType() const -> HitType;
    auto getAction() const -> Action;

    auto operator>(const HitEvent& other) const
    {
        auto hitOffset = points.has_value() ? points->getDeviation() : 0;
        auto otherHitOffset =
          other.points.has_value() ? other.points->getDeviation() : 0;
        return offsetFromStart + hitOffset >
               other.offsetFromStart + otherHitOffset;
    }

    friend auto operator<<(QDataStream& stream, const HitEvent& tap)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, HitEvent& tap) -> QDataStream&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_HITEVENT_H
