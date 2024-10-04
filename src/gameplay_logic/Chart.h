//
// Created by bobini on 18.08.23.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <QObject>
#include "BmsGameReferee.h"
#include "ChartData.h"
#include "BmsScoreAftermath.h"
#include "qml_components/Bga.h"
#include "resource_managers/Vars.h"

#include <QTimer>
#include <qfuture.h>
#include <qfuturewatcher.h>
namespace resource_managers {
class Profile;
} // namespace resource_managers
namespace gameplay_logic {

class Chart final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(QList<BmsNotes*> notes READ getNotes CONSTANT)
    Q_PROPERTY(QList<BmsScore*> score READ getScore CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(
      int64_t timeBeforeChartStart READ getTimeBeforeChartStart CONSTANT)
    Q_PROPERTY(int64_t timeAfterChartEnd READ getTimeAfterChartEnd CONSTANT)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY loaded)
    Q_PROPERTY(
      QList<resource_managers::Profile*> profiles READ getProfiles CONSTANT)
    Q_PROPERTY(QList<BmsScore*> scores READ getScores CONSTANT)

    QTimer propertyUpdateTimer;
    std::chrono::system_clock::time_point startTimepoint;
    std::vector<BmsGameReferee> gameReferees;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    BmsNotes* notes;
    QList<BmsScore*> scores;
    QFuture<std::vector<BmsGameReferee>> refereesFuture;
    QFutureWatcher<std::vector<BmsGameReferee>> refereesFutureWatcher;
    qml_components::BgaContainer* bga{};
    QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture;
    QFutureWatcher<std::unique_ptr<qml_components::BgaContainer>>
      bgaFutureWatcher;
    QList<QPointer<resource_managers::Profile>> profiles;
    int64_t elapsed;
    int64_t timeBeforeChartStart{};
    int64_t timeAfterChartEnd{};
    double position;
    bool startRequested = false;

    void updateElapsed();
    void updateBpm();
    int numberOfSetupCalls = 0;
    void setup();
    void setElapsed(int64_t elapsed);
    void setPosition(double position);

    void setTimeBeforeChartStart(int64_t timeBeforeChartStart);
    void setTimeAfterChartEnd(int64_t timeAfterChartEnd);

  public:
    explicit Chart(
      QFuture<std::vector<BmsGameReferee>> refereesFuture,
      QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
      ChartData* chartData,
      BmsNotes* notes,
      QList<BmsScore*> scores,
      QList<resource_managers::Profile*> profiles,
      QObject* parent = nullptr);

    Q_INVOKABLE void start();

    enum class EventType
    {
        KeyPress,
        KeyRelease
    };
    Q_ENUM(EventType)

    void passKey(input::BmsKey key, EventType eventType, int64_t time);

    Q_INVOKABLE QList<BmsScoreAftermath*> finish();

    auto getElapsed() const -> int64_t;

    auto getChartData() const -> ChartData*;

    auto getNotes() const -> const BmsNotes*;

    auto getScores() const -> const QList<BmsScore*>&;

    auto getProfiles() const -> QList<resource_managers::Profile*>;

    auto getPosition() const -> double;

    auto getTimeBeforeChartStart() const -> int64_t;

    auto getTimeAfterChartEnd() const -> int64_t;

    auto getBga() const -> qml_components::BgaContainer*;

  signals:
    void elapsedChanged(int64_t delta);
    void positionChanged(double delta);
    void over();
    void bpmChanged(BpmChange bpmChange);
    void started();
    void loaded();
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
