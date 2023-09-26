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
    Q_PROPERTY(int64_t timestamp MEMBER timestamp)
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

class Snap
{
    Q_GADGET
    Q_PROPERTY(double numerator MEMBER numerator)
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
    Q_PROPERTY(Time time MEMBER time)
    Q_PROPERTY(Snap snap MEMBER snap)
  public:
    Time time;
    Snap snap;
    auto operator<=>(const Note& other) const = default;
};

inline auto
operator<<(QDataStream& stream, const Note& note) -> QDataStream&
{
    return stream << note.time << note.snap;
}

inline auto
operator>>(QDataStream& stream, Note& note) -> QDataStream&
{
    return stream >> note.time >> note.snap;
}

class BmsNotes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      QVector<QVector<Note>> visibleNotes READ getVisibleNotes CONSTANT)
    Q_PROPERTY(
      QVector<QVector<Note>> invisibleNotes READ getInvisibleNotes CONSTANT)
    Q_PROPERTY(QVector<BpmChange> bpmChanges READ getBpmChanges CONSTANT)
    Q_PROPERTY(QVector<Time> barLines READ getBarLines CONSTANT)

    QVector<QVector<Note>> visibleNotes;
    QVector<QVector<Note>> invisibleNotes;
    QVector<BpmChange> bpmChanges;
    QVector<Time> barLines;

  public:
    BmsNotes() = default;
    explicit BmsNotes(QVector<QVector<Note>> visibleNotes,
                      QVector<QVector<Note>> invisibleNotes,
                      QVector<BpmChange> bpmChanges,
                      QVector<Time> barLines,
                      QObject* parent = nullptr);

    [[nodiscard]] auto getVisibleNotes() const -> const QVector<QVector<Note>>&;

    [[nodiscard]] auto getInvisibleNotes() const
      -> const QVector<QVector<Note>>&;

    [[nodiscard]] auto getBarLines() const -> const QVector<Time>&;

    [[nodiscard]] auto getBpmChanges() const -> const QVector<BpmChange>&;

    friend auto operator<<(QDataStream& stream, const BmsNotes& notes)
      -> QDataStream&
    {
        return stream << notes.visibleNotes << notes.invisibleNotes
                      << notes.bpmChanges << notes.barLines;
    }

    friend auto operator>>(QDataStream& stream, BmsNotes& notes) -> QDataStream&
    {
        return stream >> notes.visibleNotes >> notes.invisibleNotes >>
               notes.bpmChanges >> notes.barLines;
    }

    static auto load(const std::string& serializedData)
      -> std::unique_ptr<BmsNotes>;
    auto serialize() const -> std::string;
    auto save(db::SqliteCppDb& db, const support::Sha256& sha256) const -> void;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSNOTES_H
