//
// Created by bobini on 18.08.23.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <QObject>
#include "BmsGameReferee.h"
#include "ChartData.h"
#include "BmsScore.h"
#include "BmsLiveScore.h"
#include "resource_managers/Profile.h"
#include "qml_components/Bga.h"
#include "NoteState.h"

#include <QTimer>
#include <qfuture.h>
#include <qfuturewatcher.h>
namespace gameplay_logic {
class GameplayState;
class Player;

/**
 * @brief The main class responsible for running a chart during gameplay.
 * @details Once start() is called and the BGA and sounds are loaded, the
 * ChartRunner starts its update timer. It updates the player objects on each
 * timer tick. It also handles key events and passes them to the appropriate
 * player object.
 */
class ChartRunner final : public QObject
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

    /** @brief The chart data of the chart being played. */
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    /** @brief The BGA object. Initially null. */
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY bgaLoaded)
    /** @brief The current status of the chart runner. */
    Q_PROPERTY(Status status READ getStatus NOTIFY statusChanged)
    /**
     * @brief The keymode of the chart being played.
     * @details Can be different from the keymode of chartData when
     * resource_managers::dp_options::DpOptions is battle.
     * @note This is the property used to determine which gampley screen to
     * load.
     */
    Q_PROPERTY(
      gameplay_logic::ChartData::Keymode keymode READ getKeymode CONSTANT)
    /**
     * @brief The player 1 object.
     * @details When playing single player, this is the only player object
     * (even when the player plays on the right side).
     */
    Q_PROPERTY(Player* player1 READ getPlayer1 CONSTANT)
    /**
     * @brief The player 2 object.
     * @details Only set in battle mode, null otherwise.
     */
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
    ChartData::Keymode keymode;

    void updateElapsed();
    int numberOfSetupCalls = 0;
    void setStatus(Status ready);
    void setup();

  public:
    explicit ChartRunner(
      ChartData* chartData,
      QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
      ChartData::Keymode keymode,
      Player* player1,
      Player* player2,
      QObject* parent = nullptr);

    Q_INVOKABLE void start();

    void passKey(input::BmsKey key, EventType eventType, int64_t time);

    Q_INVOKABLE QList<BmsScore*> finish();

    auto getChartData() const -> ChartData*;
    auto getKeymode() const -> ChartData::Keymode;

    auto getStatus() const -> Status;

    auto getBga() const -> qml_components::BgaContainer*;

    auto getPlayer1() const -> Player*;
    auto getPlayer2() const -> Player*;

  signals:
    void statusChanged();
    void bgaLoaded();
};

/**
 * @brief A player of a chart.
 * @details The Player class represents a player of a chart. It plays sounds,
 * gets its score updated, etc.
 */
class Player : public QObject
{
    Q_OBJECT
    /**
     * @brief The notes of the chart being played.
     */
    Q_PROPERTY(BmsNotes* notes READ getNotes CONSTANT)
    /**
     * @brief The live score of the player, which gets updated during
     * gameplay.
     */
    Q_PROPERTY(BmsLiveScore* score READ getScore CONSTANT)
    /**
     * @brief The gameplay state of the player, containing column states and
     * barlines.
     */
    Q_PROPERTY(GameplayState* state READ getState CONSTANT)
    /**
     * @brief The profile of the player.
     */
    Q_PROPERTY(resource_managers::Profile* profile READ getProfile CONSTANT)
    /**
     * @brief The positition in the chart, expressed in beats.
     */
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    /**
     * @brief The elapsed time since the start of the chart, in nanoseconds.
     */
    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    /**
     * @brief The current status of the player.
     */
    Q_PROPERTY(ChartRunner::Status status READ getStatus NOTIFY statusChanged)
    /**
     * @brief The length of the chart in nanoseconds.
     */
    Q_PROPERTY(int64_t chartLength READ getChartLength CONSTANT)
    BmsNotes* notes;
    GameplayState* state;
    QPointer<resource_managers::Profile> profile;
    BmsGameReferee::Position position{};
    ChartRunner::Status status{ ChartRunner::Status::Loading };
    int64_t elapsed{};
    std::chrono::nanoseconds chartLength;

    void setElapsed(int64_t newElapsed);
    void setPosition(BmsGameReferee::Position position);

    QFutureWatcher<BmsGameReferee> refereeWatcher;
    QFuture<BmsGameReferee> refereeFuture;
    BmsLiveScore* score;

  protected:
    std::optional<BmsGameReferee> referee;

  public:
    explicit Player(BmsNotes* notes,
                    BmsLiveScore* score,
                    GameplayState* state,
                    resource_managers::Profile* profile,
                    QFuture<BmsGameReferee> referee,
                    std::chrono::nanoseconds chartLength,
                    QObject* parent = nullptr);
    virtual void update(std::chrono::nanoseconds offsetFromStart,
                        bool lastUpdate);
    virtual void passKey(input::BmsKey key,
                         ChartRunner::EventType eventType,
                         std::chrono::nanoseconds offset);
    void setup();
    auto getNotes() const -> BmsNotes*;
    auto getScore() const -> BmsLiveScore*;
    auto getState() const -> GameplayState*;
    auto getProfile() const -> resource_managers::Profile*;
    auto getPosition() const -> double;
    auto getElapsed() const -> int64_t;
    auto getStatus() const -> ChartRunner::Status;
    void setStatus(ChartRunner::Status status);
    auto getChartLength() const -> int64_t;
    auto finish() -> BmsScore*;

  signals:
    void positionChanged(double delta);
    void elapsedChanged(int64_t delta);
    void statusChanged();
};

/**
 * @brief A player that replays a recorded score.
 * @details The RePlayer class replays a recorded score. It does not accept
 * any input from the user.
 */
class RePlayer final : public Player
{
    Q_OBJECT
    /**
     * @brief The score being replayed.
     */
    Q_PROPERTY(BmsScore* replayedScore READ getReplayedScore CONSTANT)

    BmsScore* replayedScore;
    std::span<const HitEvent> events;

  public:
    explicit RePlayer(BmsNotes* notes,
                      BmsLiveScore* score,
                      GameplayState* state,
                      resource_managers::Profile* profile,
                      QFuture<BmsGameReferee> referee,
                      std::chrono::nanoseconds chartLength,
                      BmsScore* replayedScore,
                      QObject* parent = nullptr);
    void passKey(input::BmsKey key,
                 ChartRunner::EventType eventType,
                 std::chrono::nanoseconds offset) override;
    void update(std::chrono::nanoseconds offsetFromStart,
                bool lastUpdate) override;
    auto getReplayedScore() const -> BmsScore*;
};

/**
 * @brief A player that plays automatically, hitting all notes perfectly.
 * @details The AutoPlayer class plays automatically, hitting all notes
 * perfectly. Its score is not saved.
 */
class AutoPlayer final : public Player
{
    Q_OBJECT
    std::vector<HitEvent> eventsVec;
    std::span<const HitEvent> events;

  public:
    explicit AutoPlayer(BmsNotes* notes,
                        BmsLiveScore* score,
                        GameplayState* state,
                        resource_managers::Profile* profile,
                        QFuture<BmsGameReferee> referee,
                        std::chrono::nanoseconds chartLength,
                        std::vector<HitEvent> events,
                        QObject* parent = nullptr);
    void passKey(input::BmsKey key,
                 ChartRunner::EventType eventType,
                 std::chrono::nanoseconds offset) override;
    void update(std::chrono::nanoseconds offsetFromStart,
                bool lastUpdate) override;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
