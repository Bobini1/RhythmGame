//
// Created by PC on 02/12/2025.
//

#ifndef RHYTHMGAME_SCOREREPLAYER_H
#define RHYTHMGAME_SCOREREPLAYER_H
#include "gameplay_logic/HitEvent.h"

#include <qqmlintegration.h>
#include <QHash>
#include <QObject>

namespace qml_components {

class ScoreReplayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<gameplay_logic::HitEvent> hitEvents READ getHitEvents WRITE
                 setHitEvents NOTIFY hitEventsChanged RESET resetHitEvents)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged RESET resetPoints)

    QList<gameplay_logic::HitEvent> hitEvents;

    // Saved score: points earned per note, keyed by (column << 32 | noteIndex).
    // Built from noteRemoved=true events only — one definitive entry per note.
    QHash<qint64, double> m_savedPointsByNote;

    double m_accumulatedPoints = 0.0;

  public:
    explicit ScoreReplayer(QObject* parent = nullptr);
    auto getHitEvents() const -> const QList<gameplay_logic::HitEvent>&;
    void setHitEvents(const QList<gameplay_logic::HitEvent>& hitEvents);
    void resetHitEvents();

    auto getPoints() const -> double;

    // Call this from QML's onHit for every hit event on the current player.
    // The ghost score advances when noteRemoved == true (note definitively
    // consumed), adding the saved score's points for that same note.
    Q_INVOKABLE void notifyHit(const gameplay_logic::HitEvent& currentHit);

    // Reset accumulated points for the current game (call at game start).
    Q_INVOKABLE void resetPoints();

  signals:
    void hitEventsChanged();
    void pointsChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCOREREPLAYER_H
