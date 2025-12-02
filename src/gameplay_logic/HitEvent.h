//
// Created by bobini on 22.08.23.
//

#ifndef RHYTHMGAME_HITEVENT_H
#define RHYTHMGAME_HITEVENT_H
#include <QObject>
#include <QVariant>
#include "BmsPoints.h"
namespace gameplay_logic {

/**
 * @brief A very generic event that happened during gameplay.
 * @details It represents hits but also misses and note releases.
 * Hits that hit nothing as well.
 */
class HitEvent
{
    Q_GADGET
  public:
    enum class Action
    {
        None,   /** The player did not press or release a key in this event. */
        Press,  /** The player pressed a key. */
        Release /** The player released a key. */
    };
    Q_ENUM(Action)
  private:
    // nanoseconds
    using DeltaTime = int64_t;
    /**
     * @brief The offset from the start of the chart when this event happened.
     */
    Q_PROPERTY(DeltaTime offsetFromStart READ getOffsetFromStart CONSTANT)
    /**
     * @brief The offset from the note's perfect hit time when this event
     * happened.
     * @details Will be 0 for hits that hit nothing.
     * @note This simply forwards the value from BmsPoints::deviation.
     */
    Q_PROPERTY(DeltaTime hitOffset READ getHitOffset CONSTANT)
    /**
     * @brief The points awarded for this hit.
     * @details Will be null for hits that hit nothing.
     * @see BmsPoints
     */
    Q_PROPERTY(QVariant points READ getPoints CONSTANT)
    /**
     * @brief The column where this event happened.
     */
    Q_PROPERTY(int column READ getColumn CONSTANT)
    /**
     * @brief The index of the note that was hit or -1 if no note was hit.
     * @details This index corresponds to the index in BmsNotes::notes.
     * It will be -1 for hits that hit nothing.
     */
    Q_PROPERTY(int noteIndex READ getNoteIndex CONSTANT)
    /**
     * @brief What the user did to cause this event to happen.
     */
    Q_PROPERTY(Action action READ getAction CONSTANT)
    /**
     * @brief Whether this event removed a note.
     * @details False for empty poors. The note can still be hit again.
     */
    Q_PROPERTY(bool noteRemoved READ getNoteRemoved CONSTANT)

    DeltaTime offsetFromStart;
    std::optional<BmsPoints> points;
    std::optional<int> noteIndex;
    int column;
    Action action;
    bool noteRemoved;

  public:
    HitEvent(int column,
             std::optional<int> noteIndex,
             DeltaTime offsetFromStart,
             std::optional<BmsPoints> points,
             Action action,
             bool noteRemoved);
    HitEvent() = default;

    auto getOffsetFromStart() const -> DeltaTime;
    // if the tap did not hit a note, this returns null
    auto getHitOffset() const -> DeltaTime;
    auto getPoints() const -> QVariant;
    auto getPointsOptional() const -> std::optional<BmsPoints>;
    auto getColumn() const -> int;
    // if the tap did not hit a note, this returns -1
    auto getNoteIndex() const -> int;
    auto getAction() const -> Action;
    auto getNoteRemoved() const -> bool;
    void setAction(Action newAction);
    void setPoints(std::optional<BmsPoints> newPoints);
    void setNoteRemoved(bool newNoteRemoved);

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
