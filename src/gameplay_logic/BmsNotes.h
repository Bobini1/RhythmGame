//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_BMSNOTES_H
#define RHYTHMGAME_BMSNOTES_H
#include <QObject>

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

class BmsNotes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QList<Note>> visibleNotes READ getVisibleNotes CONSTANT)
    Q_PROPERTY(
      QList<QList<Note>> invisibleNotes READ getInvisibleNotes CONSTANT)
    Q_PROPERTY(QList<BpmChange> bpmChanges READ getBpmChanges CONSTANT)
    Q_PROPERTY(QList<Time> barLines READ getBarLines CONSTANT)

    QList<QList<Note>> visibleNotes;
    QList<QList<Note>> invisibleNotes;
    QList<BpmChange> bpmChanges;
    QList<Time> barLines;

  public:
    explicit BmsNotes(QList<QList<Note>> visibleNotes,
                      QList<QList<Note>> invisibleNotes,
                      QList<BpmChange> bpmChanges,
                      QList<Time> barLines,
                      QObject* parent = nullptr);

    [[nodiscard]] auto getVisibleNotes() const -> const QList<QList<Note>>&;

    [[nodiscard]] auto getInvisibleNotes() const -> const QList<QList<Note>>&;

    [[nodiscard]] auto getBarLines() const -> const QList<Time>&;

    [[nodiscard]] auto getBpmChanges() const -> const QList<BpmChange>&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSNOTES_H
