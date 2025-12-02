//
// Created by PC on 02/12/2025.
//

#ifndef RHYTHMGAME_SCOREREPLAYER_H
#define RHYTHMGAME_SCOREREPLAYER_H
#include "gameplay_logic/HitEvent.h"

#include <qqmlintegration.h>
#include <QObject>

namespace qml_components {

class ScoreReplayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<gameplay_logic::HitEvent> hitEvents READ getHitEvents WRITE
                 setHitEvents NOTIFY hitEventsChanged RESET resetHitEvents)
    Q_PROPERTY(qint64 elapsed READ getElapsed WRITE setElapsed NOTIFY
                 elapsedChanged RESET resetElapsed)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    QList<gameplay_logic::HitEvent> hitEvents;
    QList<std::pair<qint64, double>> pointElapseds;
    qint64 elapsed = 0;

  public:
    explicit ScoreReplayer(QObject* parent = nullptr);
    auto getHitEvents() const -> const QList<gameplay_logic::HitEvent>&;
    void setHitEvents(const QList<gameplay_logic::HitEvent>& hitEvents);
    void resetHitEvents();
    auto getElapsed() const -> qint64 { return elapsed; }
    void setElapsed(qint64 elapsed);
    void resetElapsed();
    auto getPoints() const -> double;
  signals:
    void hitEventsChanged();
    void elapsedChanged();
    void pointsChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCOREREPLAYER_H
