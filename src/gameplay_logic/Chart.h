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
#include "NoteState.h"
#include "support/GeneratePermutation.h"

#include <QTimer>
#include <qfuture.h>
#include <qfuturewatcher.h>
namespace resource_managers {
class Profile;
} // namespace resource_managers
namespace gameplay_logic {
class GameplayState;
class Player;

class Chart final : public QObject
{
    Q_OBJECT

  public:
    enum class EventType
    {
        KeyPress,
        KeyRelease
    };
    Q_ENUM(EventType)
    enum Status
    {
        Loading,
        Ready,
        Running,
        Finished
    };
    Q_ENUM(Status)

  private:
    Player* player1;
    Player* player2;

    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY bgaLoaded)
    Q_PROPERTY(Status status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(Player* player1 READ getPlayer1 CONSTANT)
    Q_PROPERTY(Player* player2 READ getPlayer2 CONSTANT)

    QTimer propertyUpdateTimer;
    std::chrono::system_clock::time_point startTimepoint;
    ChartData* chartData;
    qml_components::BgaContainer* bga{};
    QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture;
    QFutureWatcher<std::unique_ptr<qml_components::BgaContainer>>
      bgaFutureWatcher;
    Status status{ Loading };
    bool startRequested = false;

    void updateElapsed();
    int numberOfSetupCalls = 0;
    void setStatus(Status ready);
    void setup();

  public:
    explicit Chart(
      ChartData* chartData,
      QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
      Player* player1,
      Player* player2,
      QObject* parent = nullptr);

    Q_INVOKABLE void start();

    void passKey(input::BmsKey key, EventType eventType, int64_t time);

    Q_INVOKABLE QList<BmsScoreAftermath*> finish();

    auto getChartData() const -> ChartData*;

    auto getStatus() const -> Status;

    auto getBga() const -> qml_components::BgaContainer*;

    auto getPlayer1() const -> Player*;
    auto getPlayer2() const -> Player*;

  signals:
    void statusChanged();
    void bgaLoaded();
};

class Player final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(BmsNotes* notes READ getNotes CONSTANT)
    Q_PROPERTY(BmsScore* score READ getScore CONSTANT)
    Q_PROPERTY(GameplayState* state READ getState CONSTANT)
    Q_PROPERTY(resource_managers::Profile* profile READ getProfile CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(
      double positionBeforeChartStart READ getPositionBeforeChartStart CONSTANT)
    Q_PROPERTY(
      int64_t timeBeforeChartStart READ getTimeBeforeChartStart CONSTANT)
    Q_PROPERTY(Chart::Status status READ getStatus NOTIFY statusChanged)
    BmsNotes* notes;
    BmsScore* score;
    GameplayState* state;
    QPointer<resource_managers::Profile> profile;
    std::optional<BmsGameReferee> referee;
    QFutureWatcher<BmsGameReferee> refereeWatcher;
    QFuture<BmsGameReferee> refereeFuture;
    BmsGameReferee::Position positionBeforeChartStart{};
    BmsGameReferee::Position position{};
    Chart::Status status{ Chart::Status::Loading };
    int64_t elapsed{};
    int64_t timeBeforeChartStart{};

    void setTimeBeforeChartStart(int64_t timeBeforeChartStart);
    void setElapsed(int64_t newElapsed);
    void setPosition(BmsGameReferee::Position position);

  public:
    explicit Player(BmsNotes* notes,
                    BmsScore* score,
                    GameplayState* state,
                    resource_managers::Profile* profile,
                    QFuture<BmsGameReferee> referee,
                    QObject* parent = nullptr);
    void update(std::chrono::nanoseconds offsetFromStart, bool lastUpdate);
    void passKey(input::BmsKey key,
                 Chart::EventType eventType,
                 std::chrono::nanoseconds offset);
    void setup();
    auto getNotes() const -> BmsNotes*;
    auto getScore() const -> BmsScore*;
    auto getState() const -> GameplayState*;
    auto getProfile() const -> resource_managers::Profile*;
    auto getPosition() const -> double;
    auto getElapsed() const -> int64_t;
    auto getPositionBeforeChartStart() const -> double;
    auto getTimeBeforeChartStart() const -> int64_t;
    auto getStatus() const -> Chart::Status;
    void setStatus(Chart::Status status);
    auto finish() const -> BmsScoreAftermath*;

  signals:
    void positionChanged(double delta);
    void elapsedChanged(int64_t delta);
    void statusChanged();
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
