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
#include "BmsScoreAftermath.h"
namespace gameplay_logic {

class Chart : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(BmsNotes* notes READ getNotes CONSTANT)
    Q_PROPERTY(BmsScore* score READ getScore CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(int64_t timeBeforeChartStart READ getTimeBeforeChartStart WRITE
                 setTimeBeforeChartStart NOTIFY timeBeforeChartStartChanged)
    Q_PROPERTY(int64_t timeAfterChartEnd READ getTimeAfterChartEnd WRITE
                 setTimeAfterChartEnd NOTIFY timeAfterChartEndChanged)

    QTimer propertyUpdateTimer;
    std::chrono::steady_clock::time_point startTimepoint;
    std::optional<gameplay_logic::BmsGameReferee> gameReferee;
    input::KeyboardInputTranslatorToBms inputTranslator;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    BmsNotes* notes;
    BmsScore* score;
    QFuture<gameplay_logic::BmsGameReferee> refereeFuture;
    QFutureWatcher<gameplay_logic::BmsGameReferee> refereeFutureWatcher;
    std::function<db::SqliteCppDb&()> scoreDb;
    int64_t elapsed;
    int64_t timeBeforeChartStart{};
    int64_t timeAfterChartEnd{};
    double position;
    bool startRequested = false;

    void updateElapsed();
    void updateBpm();
    void setReferee();
    void setElapsed(int64_t elapsed);
    void setPosition(double position);

  public:
    explicit Chart(QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
                   ChartData* chartData,
                   BmsNotes* notes,
                   BmsScore* score,
                   std::function<db::SqliteCppDb&()> scoreDb,
                   QObject* parent = nullptr);

    Q_INVOKABLE void start();

    void passKey(QKeyEvent* keyEvent);

    Q_INVOKABLE BmsScoreAftermath* finish();

    [[nodiscard]] auto getElapsed() const -> int64_t;

    [[nodiscard]] auto getChartData() const -> ChartData*;

    [[nodiscard]] auto getNotes() const -> BmsNotes*;

    [[nodiscard]] auto getScore() const -> BmsScore*;

    [[nodiscard]] auto getPosition() const -> double;

    [[nodiscard]] auto getTimeBeforeChartStart() const -> int64_t;

    [[nodiscard]] auto getTimeAfterChartEnd() const -> int64_t;

    void setTimeBeforeChartStart(int64_t timeBeforeChartStart);

    void setTimeAfterChartEnd(int64_t timeAfterChartEnd);

  signals:
    void elapsedChanged(int64_t delta);
    void positionChanged(double delta);
    void over();
    void bpmChanged(BpmChange bpmChange);
    void started();
    void timeBeforeChartStartChanged();
    void timeAfterChartEndChanged();
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
