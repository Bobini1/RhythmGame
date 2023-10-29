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
#include "qml_components/Bga.h"
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
#ifdef _WIN32
    clock_t startTimepointClk;
#endif
    std::chrono::steady_clock::time_point startTimepoint;
    std::optional<gameplay_logic::BmsGameReferee> gameReferee;
    input::KeyboardInputTranslatorToBms inputTranslator;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    BmsNotes* notes;
    BmsScore* score;
    QFuture<gameplay_logic::BmsGameReferee> refereeFuture;
    QFutureWatcher<gameplay_logic::BmsGameReferee> refereeFutureWatcher;
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
      QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
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
    void passKey(QKeyEvent* keyEvent, EventType eventType);

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
