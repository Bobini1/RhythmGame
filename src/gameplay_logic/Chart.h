//
// Created by bobini on 18.08.23.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <QObject>
#include <QtQmlIntegration>
#include "BmsGameReferee.h"
#include "input/KeyboardInputTranslatorToBms.h"
#include "ChartData.h"
namespace gameplay_logic {

class Chart : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(BmsScore* score READ getScore CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)

    QTimer propertyUpdateTimer;
    std::chrono::time_point<std::chrono::steady_clock> startTimepoint;
    std::optional<gameplay_logic::BmsGameReferee> gameReferee;
    input::KeyboardInputTranslatorToBms inputTranslator;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    QSharedPointer<BmsScore> score;
    QFuture<gameplay_logic::BmsGameReferee> refereeFuture;
    QFutureWatcher<gameplay_logic::BmsGameReferee> refereeFutureWatcher;
    int elapsed;
    double position;
    bool startRequested = false;

    void updateElapsed();
    void updateBpm();
    void setReferee();

  public:
    explicit Chart(
      QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
      ChartData* chartData,
      QSharedPointer<BmsScore> score,
      charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart,
      QObject* parent = nullptr);

    Q_INVOKABLE void start();

    Q_INVOKABLE void passKey(int key);

    [[nodiscard]] auto getElapsed() const -> int;

    [[nodiscard]] auto getChartData() const -> ChartData*;

    [[nodiscard]] auto getScore() const -> BmsScore*;

    [[nodiscard]] auto getPosition() const -> double;

  signals:
    void elapsedChanged(int delta);
    void positionChanged(double delta);
    void overChanged();
    void bpmChanged(BpmChange bpmChange);
    void started();
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
