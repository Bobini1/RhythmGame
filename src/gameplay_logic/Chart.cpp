//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

#include "resource_managers/Profile.h"
#include "support/GeneratePermutation.h"

namespace gameplay_logic {

Chart::Chart(QFuture<std::vector<BmsGameReferee>> refereesFuture,
             QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
             ChartData* chartData,
             PlayerSpecificComponents player1,
             std::optional<PlayerSpecificComponents> player2,
             QObject* parent)
  : QObject(parent)
  , bpmChanges(player1.notes->getBpmChanges())
  , chartData(chartData)
  , refereesFuture(std::move(refereesFuture))
  , bgaFuture(std::move(bgaFuture))
  , player1(std::move(player1))
  , player2(std::move(player2))
{
    connectPlayer(this->player1);
    if (this->player2.has_value()) {
        connectPlayer(*this->player2);
    }
    chartData->setParent(this);
    connect(&refereesFutureWatcher,
            &QFutureWatcher<BmsGameReferee>::finished,
            this,
            &Chart::setup);
    refereesFutureWatcher.setFuture(this->refereesFuture);
    connect(&bgaFutureWatcher,
            &QFutureWatcher<qml_components::Bga*>::finished,
            this,
            &Chart::setup);
    bgaFutureWatcher.setFuture(this->bgaFuture);

    setTimeBeforeChartStart(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds{ 1 })
        .count());
    setTimeAfterChartEnd(std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::seconds{ 3 })
                           .count());
}

void
Chart::start()
{
    if (refereesFuture.isRunning() || bgaFuture.isRunning()) {
        startRequested = true;
        return;
    }
    emit started();
    propertyUpdateTimer.start(1);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimepoint = std::chrono::system_clock::now();
    updateBpm();
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::system_clock::now() - startTimepoint;
    offset -= std::chrono::duration_cast<decltype(offset)>(
      std::chrono::nanoseconds(timeBeforeChartStart));
    setElapsed(std::chrono::nanoseconds(offset).count());
    setPosition(player1.referee->update(offset));
    if (player2) {
        setPosition(player2->referee->update(offset));
    }
    bga->update(offset);
    if (chartData->getLength() + timeAfterChartEnd <= elapsed) {
        propertyUpdateTimer.stop();
        emit over();
    }
}

auto
Chart::getElapsed() const -> int64_t
{
    return elapsed;
}

void
Chart::passKey(input::BmsKey key, const EventType eventType, const int64_t time)
{
    if (key == input::BmsKey::Start1 || key == input::BmsKey::Select1 ||
        key == input::BmsKey::Start2 || key == input::BmsKey::Select2) {
        return;
    }
    const auto index = playerIndexFromKey(key);
    // key pressed for a player side that is not present
    if (!isDp(chartData->getKeymode()) && index == 1 && !player2) {
        return;
    }
    const auto offset =
      std::chrono::milliseconds{ time } - startTimepoint.time_since_epoch();
    auto& player =
      isDp(chartData->getKeymode()) || index == 0 ? player1 : *player2;
    key = isDp(chartData->getKeymode()) ? key : convertToP1Key(key);
    if (!isDp(chartData->getKeymode())) {
        key = convertToP1Key(key);
    }
    if (!player.referee) {
        if (eventType == EventType::KeyPress) {
            if (index == 0 || index == 1 && player2 ||
                isDp(chartData->getKeymode())) {
                player.playerData.score->sendVisualOnlyTap(
                  { static_cast<int>(key),
                    std::nullopt,
                    offset.count(),
                    std::nullopt,
                    HitEvent::Action::Press,
                    /*noteRemoved=*/false });
            }
        } else {
            if (index == 0 || index == 1 && player2) {
                player.playerData.score->sendVisualOnlyRelease(
                  HitEvent{ static_cast<int>(key),
                            std::nullopt,
                            offset.count(),
                            std::nullopt,
                            HitEvent::Action::Release,
                            /*noteRemoved=*/false });
            }
        }
    } else {
        if (eventType == EventType::KeyPress) {
            player.referee->passPressed(
              offset - std::chrono::nanoseconds(timeBeforeChartStart), key);
        } else {
            player.referee->passReleased(
              offset - std::chrono::nanoseconds(timeBeforeChartStart), key);
        }
    }
}

auto
Chart::getChartData() const -> ChartData*
{
    return chartData;
}

auto
Chart::getNotes1() const -> BmsNotes*
{
    return player1.playerData.notes;
}
auto
Chart::getNotes2() const -> BmsNotes*
{
    if (!player2) {
        return nullptr;
    }
    return player2->playerData.notes;
}
auto
Chart::getScore1() const -> BmsScore*
{
    return player1.playerData.score;
}
auto
Chart::getScore2() const -> BmsScore*
{
    if (!player2) {
        return nullptr;
    }
    return player2->playerData.score;
}
auto
Chart::getState1() -> GameplayState*
{
    return player1.playerData.state;
}
auto
Chart::getState2() -> GameplayState*
{
    if (!player2) {
        return nullptr;
    }
    return player2->playerData.state;
}
void
Chart::updateBpm()
{
    auto bpmChange = bpmChanges.begin();
    while (bpmChange != bpmChanges.end() &&
           bpmChange->time.timestamp <= elapsed) {
        emit bpmChanged(*bpmChange);
        ++bpmChange;
    }
    if (bpmChange != bpmChanges.end()) {
        auto nextTimestamp = bpmChange->time.timestamp - elapsed;
        QTimer::singleShot(nextTimestamp, this, &Chart::updateBpm);
    }
    bpmChanges = std::span(bpmChange, bpmChanges.end());
}
auto
Chart::getProfile1() const -> resource_managers::Profile*
{
    return player1.playerData.profile;
}
auto
Chart::getProfile2() const -> resource_managers::Profile*
{
    if (!player2.has_value()) {
        return nullptr;
    }
    return player2->playerData.profile;
}
auto
Chart::getPosition() const -> double
{
    return position;
}
void
Chart::setup()
{
    if (++numberOfSetupCalls < 2) {
        return;
    }
    auto gameReferees = refereesFuture.takeResult();
    player1.referee = std::move(gameReferees[0]);
    if (player2) {
        player2->referee = std::move(gameReferees[1]);
    }
    bga = bgaFuture.takeResult().release();
    bga->setParent(this);
    emit loaded();
    if (startRequested) {
        start();
    }
}
auto
Chart::finish() -> QList<BmsScoreAftermath*>
{
    startRequested = false;
    propertyUpdateTimer.stop();
    if (!(player1.referee)) {
        return {};
    }
    // if we didn't get bga yet, cancel
    if (bga == nullptr) {
        bgaFutureWatcher.cancel();
        bgaFuture.cancel();
    }

    const auto chartLength = chartData->getLength();
    if (elapsed < chartLength) {
        player1.referee->update(std::chrono::nanoseconds(chartLength),
                                /*lastUpdate=*/true);
        if (player2) {
            player2->referee->update(std::chrono::nanoseconds(chartLength),
                                     /*lastUpdate=*/true);
        }
    }
    auto ret = QList<BmsScoreAftermath*>{};
    auto i = -1;
    auto players = std::vector<PlayerSpecificComponents*>{};
    players.push_back(&player1.playerData);
    if (player2) {
        players.push_back(&player2->playerData);
    }
    for (const auto& player : players) {
        const auto* score = player->score;
        i++;
        auto result = score->getResult();
        auto replayData = score->getReplayData();
        auto gaugeHistory = score->getGaugeHistory();
        if (auto* profilePtr = player->profile.get()) {
            try {
                const auto scoreId = result->save(profilePtr->getDb());
                result->setId(scoreId);
                replayData->save(profilePtr->getDb(), scoreId);
                gaugeHistory->save(profilePtr->getDb(), scoreId);
            } catch (const std::exception& e) {
                spdlog::error("Failed to save score: {}", e.what());
            }
        } else {
            spdlog::warn("Profile was deleted before saving score");
        }
        ret.append(new BmsScoreAftermath{ player->profile.get(),
                                          std::move(result),
                                          std::move(replayData),
                                          std::move(gaugeHistory) });
    }
    return ret;
}
auto
Chart::getTimeBeforeChartStart() const -> int64_t
{
    return timeBeforeChartStart;
}
auto
Chart::getTimeAfterChartEnd() const -> int64_t
{
    return timeAfterChartEnd;
}
void
Chart::setTimeBeforeChartStart(int64_t timeBeforeChartStart)
{
    if (timeBeforeChartStart < 0) {
        spdlog::warn("Time before chart start cannot be negative");
        return;
    }
    if (startRequested) {
        spdlog::warn("Cannot change time before chart start after starting");
        return;
    }
    if (timeBeforeChartStart == this->timeBeforeChartStart) {
        return;
    }
    this->timeBeforeChartStart = timeBeforeChartStart;
    auto beatsBeforeChartStart =
      std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::nanoseconds(timeBeforeChartStart)).count() *
      player1.playerData.notes->getBpmChanges().first().bpm / 60;
    setElapsed(-timeBeforeChartStart);
    setPosition(-beatsBeforeChartStart);
}
void
Chart::setTimeAfterChartEnd(int64_t newTimeAfterChartEnd)
{
    if (newTimeAfterChartEnd < 0) {
        spdlog::warn("Time after chart end cannot be negative");
        return;
    }
    if (newTimeAfterChartEnd == timeAfterChartEnd) {
        return;
    }
    timeAfterChartEnd = newTimeAfterChartEnd;
}
void
Chart::setElapsed(int64_t newElapsed)
{
    if (newElapsed != elapsed) {
        const auto delta = newElapsed - elapsed;
        elapsed = newElapsed;
        emit elapsedChanged(delta);
    }
}
void
Chart::setPosition(double newPosition)
{
    if (newPosition != position) {
        const auto delta = newPosition - position;
        position = newPosition;
        emit positionChanged(delta);
    }
}
void
Chart::connectPlayer(LoadedPlayerSpecificComponents& player)
{
    player.playerData.notes->setParent(this);
    player.playerData.score->setParent(this);
    player.playerData.state->setParent(this);
    for (auto [index, column] : std::ranges::views::enumerate(
           player.playerData.state->getColumnStates())) {
        connect(player.playerData.score,
                &BmsScore::hit,
                column,
                [column, index](const HitEvent& event) {
                    if (index == event.getColumn()) {
                        column->onHitEvent(event);
                    }
                });
        connect(
          this, &Chart::elapsedChanged, column, [column, this](int64_t delta) {
              column->setElapsed(this->elapsed);
          });
    }
    connect(this,
            &Chart::elapsedChanged,
            player.playerData.state->getBarLinesState(),
            [barLines = player.playerData.state->getBarLinesState(),
             this](int64_t delta) { barLines->setElapsed(this->elapsed); });
}
auto
Chart::getBga() const -> qml_components::BgaContainer*
{
    return bga;
}
} // namespace gameplay_logic