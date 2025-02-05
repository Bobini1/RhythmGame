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

  public:
    struct PlayerSpecificComponents
    {
        BmsNotes* notes;
        BmsScore* score;
        QPointer<resource_managers::Profile> profile;
    };

  private:
    struct LoadedPlayerSpecificComponents
    {
        PlayerSpecificComponents playerData;
        std::optional<BmsGameReferee> referee;
    };

    Q_PROPERTY(int64_t elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(BmsNotes* notes1 READ getNotes1 CONSTANT)
    Q_PROPERTY(BmsNotes* notes2 READ getNotes2 CONSTANT)
    Q_PROPERTY(BmsScore* score1 READ getScore1 CONSTANT)
    Q_PROPERTY(BmsScore* score2 READ getScore2 CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)
    Q_PROPERTY(
      int64_t timeBeforeChartStart READ getTimeBeforeChartStart CONSTANT)
    Q_PROPERTY(int64_t timeAfterChartEnd READ getTimeAfterChartEnd CONSTANT)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY loaded)
    Q_PROPERTY(resource_managers::Profile* profile1 READ getProfile1 CONSTANT)
    Q_PROPERTY(resource_managers::Profile* profile2 READ getProfile2 CONSTANT)

    QTimer propertyUpdateTimer;
    std::chrono::system_clock::time_point startTimepoint;
    std::span<const BpmChange> bpmChanges;
    ChartData* chartData;
    QFuture<std::vector<BmsGameReferee>> refereesFuture;
    QFutureWatcher<std::vector<BmsGameReferee>> refereesFutureWatcher;
    qml_components::BgaContainer* bga{};
    QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture;
    QFutureWatcher<std::unique_ptr<qml_components::BgaContainer>>
      bgaFutureWatcher;
    std::optional<LoadedPlayerSpecificComponents> player1;
    std::optional<LoadedPlayerSpecificComponents> player2;
    std::vector<LoadedPlayerSpecificComponents*> players;
    int64_t elapsed{};
    int64_t timeBeforeChartStart{};
    int64_t timeAfterChartEnd{};
    double position{};
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
      std::optional<PlayerSpecificComponents> player1,
      std::optional<PlayerSpecificComponents> player2,
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

    auto getNotes1() const -> BmsNotes*;

    auto getNotes2() const -> BmsNotes*;

    auto getScore1() const -> BmsScore*;

    auto getScore2() const -> BmsScore*;

    auto getProfile1() const -> resource_managers::Profile*;

    auto getProfile2() const -> resource_managers::Profile*;

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
