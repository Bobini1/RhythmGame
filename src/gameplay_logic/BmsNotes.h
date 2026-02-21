//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_BMSNOTES_H
#define RHYTHMGAME_BMSNOTES_H
#include <QObject>
#include "support/Sha256.h"
#include "db/SqliteCppDb.h"

namespace gameplay_logic {

class Time
{
    Q_GADGET
    /** @brief Timestamp expressed in nanoseconds */
    Q_PROPERTY(int64_t timestamp MEMBER timestamp)
    /** @brief Position expressed in beats */
    Q_PROPERTY(double position MEMBER position)
  public:
    // Timestamp in milliseconds
    int64_t timestamp;
    // Position in beats
    double position;
    auto operator+(const Time& other) const -> Time
    {
        return { timestamp + other.timestamp, position + other.position };
    }
    auto operator<=>(const Time& other) const = default;
};

inline auto
operator<<(QDataStream& stream, const Time& time) -> QDataStream&
{
    return stream << static_cast<qint64>(time.timestamp) << time.position;
}

inline auto
operator>>(QDataStream& stream, Time& time) -> QDataStream&
{
    auto timestamp = qint64{};
    auto& ret = stream >> timestamp >> time.position;
    time.timestamp = timestamp;
    return ret;
}

class BpmChange
{
    Q_GADGET
    Q_PROPERTY(Time time MEMBER time)
    Q_PROPERTY(double bpm MEMBER bpm)
  public:
    Time time;
    double bpm;
    auto operator<=>(const BpmChange& other) const = default;
};

inline auto
operator<<(QDataStream& stream, const BpmChange& bpmChange) -> QDataStream&
{
    return stream << bpmChange.time << bpmChange.bpm;
}

inline auto
operator>>(QDataStream& stream, BpmChange& bpmChange) -> QDataStream&
{
    return stream >> bpmChange.time >> bpmChange.bpm;
}

/**
 * @brief Represents the position of the note in a measure.
 */
class Snap
{
    Q_GADGET
    /*
     * @brief The position of the note in a measure, expressed in whole notes.
     */
    Q_PROPERTY(double numerator MEMBER numerator)
    /**
     * @brief How many whole notes fit into a measure.
     */
    Q_PROPERTY(double denominator MEMBER denominator)
  public:
    double numerator;
    double denominator;
    auto operator<=>(const Snap& other) const = default;
};

inline auto
operator<<(QDataStream& stream, const Snap& snap) -> QDataStream&
{
    return stream << snap.numerator << snap.denominator;
}

inline auto
operator>>(QDataStream& stream, Snap& snap) -> QDataStream&
{
    return stream >> snap.numerator >> snap.denominator;
}

class Note
{
    Q_GADGET
  public:
    enum class Type
    {
        Normal,
        LongNoteBegin,
        LongNoteEnd,
        Landmine,
        Invisible
    };
    Q_ENUM(Type)
  private:
    Q_PROPERTY(Time time MEMBER time)
    Q_PROPERTY(Snap snap MEMBER snap)
    Q_PROPERTY(Type type MEMBER type)
  public:
    Time time;
    Snap snap;
    Type type;
    auto operator<=>(const Note& other) const = default;
};

inline auto
operator<<(QDataStream& stream, const Note& note) -> QDataStream&
{
    return stream << note.time << note.snap << note.type;
}

inline auto
operator>>(QDataStream& stream, Note& note) -> QDataStream&
{
    return stream >> note.time >> note.snap >> note.type;
}

class BmsNotes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QList<Note>> notes READ getNotes CONSTANT)
    Q_PROPERTY(QList<Time> barLines READ getBarLines CONSTANT)

    QList<QList<Note>> notes;
    QList<Time> barLines;

  public:
    BmsNotes() = default;
    explicit BmsNotes(QList<QList<Note>> visibleNotes,
                      QList<Time> barLines,
                      QObject* parent = nullptr);

    auto getNotes() -> QList<QList<Note>>&;

    auto getNotes() const -> const QList<QList<Note>>&;
    auto getBarLines() const -> const QList<Time>&;

    friend auto operator<<(QDataStream& stream, const BmsNotes& notes)
      -> QDataStream&
    {
        return stream << notes.notes << notes.barLines;
    }

    friend auto operator>>(QDataStream& stream, BmsNotes& notes) -> QDataStream&
    {
        return stream >> notes.notes >> notes.barLines;
    }

    static auto load(db::SqliteCppDb& db, const support::Sha256& sha256)
      -> std::unique_ptr<BmsNotes>;
    auto serialize() const -> QByteArray;
    auto save(db::SqliteCppDb& db, const support::Sha256& sha256) const -> void;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSNOTES_H
