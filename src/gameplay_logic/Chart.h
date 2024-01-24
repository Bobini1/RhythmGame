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

#include <QTimer>
#include <qfuture.h>
#include <qfuturewatcher.h>
namespace gameplay_logic {

class Chart : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(BmsNotes* notes READ getNotes CONSTANT)
    Q_PROPERTY(BmsScore* score READ getScore CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(
      int64_t timeBeforeChartStart READ getTimeBeforeChartStart CONSTANT)
    Q_PROPERTY(int64_t timeAfterChartEnd READ getTimeAfterChartEnd CONSTANT)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY loaded)

    std::array<bool, charts::gameplay_models::BmsNotesData::columnNumber>
      keyStates;
    QTimer propertyUpdateTimer;
    std::chrono::system_clock::time_point startTimepoint;
    std::optional<BmsGameReferee> gameReferee;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    BmsNotes* notes;
    BmsScore* score;
    QFuture<BmsGameReferee> refereeFuture;
    QFutureWatcher<BmsGameReferee> refereeFutureWatcher;
    qml_components::BgaContainer* bga{};
    QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture;
    QFutureWatcher<std::unique_ptr<qml_components::BgaContainer>>
      bgaFutureWatcher;
    std::function<db::SqliteCppDb&()> scoreDb;
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
      QFuture<BmsGameReferee> refereeFuture,
      QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
      ChartData* chartData,
      BmsNotes* notes,
      BmsScore* score,
      std::function<db::SqliteCppDb&()> scoreDb,
      QObject* parent = nullptr);

    Q_INVOKABLE void start();

    enum class EventType
    {
        KeyPress,
        KeyRelease
    };
    Q_ENUM(EventType);

    void passKey(input::BmsKey key, EventType eventType, int64_t time);

    Q_INVOKABLE BmsScoreAftermath* finish();

    [[nodiscard]] auto getElapsed() const -> int64_t;

    [[nodiscard]] auto getChartData() const -> ChartData*;

    [[nodiscard]] auto getNotes() const -> BmsNotes*;

    [[nodiscard]] auto getScore() const -> BmsScore*;

    [[nodiscard]] auto getPosition() const -> double;

    [[nodiscard]] auto getTimeBeforeChartStart() const -> int64_t;

    [[nodiscard]] auto getTimeAfterChartEnd() const -> int64_t;

    [[nodiscard]] auto getBga() const -> qml_components::BgaContainer*;

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
