//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

#include "resource_managers/Profile.h"
#include "support/GeneratePermutation.h"

namespace gameplay_logic {

Chart::Chart(ChartData* chartData,
             QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
             Player* player1,
             Player* player2,
             QObject* parent)
  : QObject(parent)
  , player1(player1)
  , player2(player2)
  , chartData(chartData)
  , bgaFuture(std::move(bgaFuture))
{
    player1->setParent(this);
    if (player2 != nullptr) {
        player2->setParent(this);
    }
    chartData->setParent(this);
    connect(&bgaFutureWatcher,
            &QFutureWatcher<qml_components::Bga*>::finished,
            this,
            &Chart::setup);
    bgaFutureWatcher.setFuture(this->bgaFuture);
    connect(player1, &Player::statusChanged, this, &Chart::setup);
    if (player2) {
        connect(player2, &Player::statusChanged, this, &Chart::setup);
    }
}

void
Chart::start()
{
    if (player1->getStatus() == Loading ||
        (player2 != nullptr && player2->getStatus() == Loading) ||
        bgaFuture.isRunning()) {
        startRequested = true;
        return;
    }
    setStatus(Running);
    propertyUpdateTimer.start(1);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimepoint = std::chrono::system_clock::now();
}

void
Chart::updateElapsed()
{
    const auto offset = std::chrono::system_clock::now() - startTimepoint;
    player1->update(offset,
                    /*lastUpdate=*/false);
    if (player2 != nullptr) {
        player2->update(offset,
                        /*lastUpdate=*/false);
    }
    bga->update(offset);
    if (player1->getStatus() == Finished &&
        (player2 == nullptr || player2->getStatus() == Finished)) {
        propertyUpdateTimer.stop();
        setStatus(Finished);
    }
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
    if (!isDp(chartData->getKeymode()) && index == 1 && player2 == nullptr) {
        return;
    }
    const auto offset =
      std::chrono::milliseconds{ time } - startTimepoint.time_since_epoch();
    auto* player =
      isDp(chartData->getKeymode()) || index == 0 ? player1 : player2;
    key = isDp(chartData->getKeymode()) ? key : convertToP1Key(key);
    if (!isDp(chartData->getKeymode())) {
        key = convertToP1Key(key);
    }
    player->passKey(key, eventType, offset);
}

auto
Chart::getChartData() const -> ChartData*
{
    return chartData;
}
auto
Chart::getStatus() const -> Status
{
    return status;
}

void
Chart::setStatus(const Status status)
{
    if (this->status != status) {
        this->status = status;
        emit statusChanged();
    }
}
void
Chart::setup()
{
    if (++numberOfSetupCalls < (player2 != nullptr ? 3 : 2)) {
        return;
    }
    bga = bgaFuture.takeResult().release();
    bga->setParent(this);
    emit bgaLoaded();
    setStatus(Ready);
    if (startRequested) {
        start();
    }
}
auto
Chart::finish() -> QList<BmsScoreAftermath*>
{
    startRequested = false;
    propertyUpdateTimer.stop();
    if (player1->getStatus() == Loading ||
        (player2 != nullptr && player2->getStatus() == Loading)) {
    }
    // if we didn't get bga yet, cancel
    if (bga == nullptr) {
        bgaFutureWatcher.cancel();
        bgaFuture.cancel();
    }
    using namespace std::chrono_literals;
    auto chartLength = std::chrono::nanoseconds(chartData->getLength());

    if (player1->getStatus() == Running) {
        player1->update(chartLength + 10s,
                        /*lastUpdate=*/true);
    }
    if (player2 != nullptr && player2->getStatus() == Running) {
        player2->update(std::chrono::nanoseconds(chartLength) + 10s,
                        /*lastUpdate=*/true);
    }
    auto ret = QList<BmsScoreAftermath*>{};
    ret.append(player1->finish());
    if (player2 != nullptr) {
        ret.push_back(player2->finish());
    }
    return ret;
}
void
Player::setTimeBeforeChartStart(int64_t timeBeforeChartStart)
{
    this->timeBeforeChartStart = timeBeforeChartStart;
    positionBeforeChartStart =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::nanoseconds(timeBeforeChartStart))
        .count() *
      notes->getBpmChanges().first().bpm / 60;
    setElapsed(-timeBeforeChartStart);
    setPosition(-positionBeforeChartStart);
}
void
Player::setElapsed(const int64_t newElapsed)
{
    if (newElapsed != elapsed) {
        const auto delta = newElapsed - elapsed;
        elapsed = newElapsed;
        emit elapsedChanged(delta);
    }
}
auto
Chart::getBga() const -> qml_components::BgaContainer*
{
    return bga;
}
auto
Chart::getPlayer1() const -> Player*
{
    return player1;
}
auto
Chart::getPlayer2() const -> Player*
{
    return player2;
}
Player::Player(BmsNotes* notes,
               BmsScore* score,
               GameplayState* state,
               resource_managers::Profile* profile,
               QFuture<BmsGameReferee> referee,
               QObject* parent)
  : QObject(parent)
  , notes(notes)
  , score(score)
  , state(state)
  , profile(profile)
  , refereeFuture(std::move(referee))
{
    notes->setParent(this);
    score->setParent(this);
    state->setParent(this);
    setTimeBeforeChartStart(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds{ 0 })
        .count());
    connect(&refereeWatcher,
            &QFutureWatcher<BmsGameReferee>::finished,
            this,
            &Player::setup);
    refereeWatcher.setFuture(refereeFuture);
    for (auto [index, column] :
         std::ranges::views::enumerate(state->getColumnStates())) {
        connect(score,
                &BmsScore::hit,
                column,
                [column, index](const HitEvent& event) {
                    if (index == event.getColumn()) {
                        column->onHitEvent(event);
                    }
                });
        connect(this, &Player::elapsedChanged, column, [column, this](int64_t) {
            column->setElapsed(this->elapsed);
        });
    }
    connect(this,
            &Player::elapsedChanged,
            state->getBarLinesState(),
            [barLines = state->getBarLinesState(), this](int64_t) {
                barLines->setElapsed(this->elapsed);
            });
}
void
Player::setPosition(BmsGameReferee::Position newPosition)
{
    if (newPosition != position) {
        const auto delta = newPosition - position;
        position = newPosition;
        emit positionChanged(delta);
    }
}
void
Player::update(std::chrono::nanoseconds offsetFromStart, bool lastUpdate)
{
    offsetFromStart =
      offsetFromStart - std::chrono::nanoseconds(timeBeforeChartStart);
    setElapsed(offsetFromStart.count());
    if (referee) {
        const auto position = referee->update(offsetFromStart, lastUpdate);
        setPosition(position);
    }
}
void
Player::passKey(input::BmsKey key,
                Chart::EventType eventType,
                std::chrono::nanoseconds offset)
{
    if (!referee) {
        if (eventType == Chart::EventType::KeyPress) {
            score->sendVisualOnlyTap({ static_cast<int>(key),
                                       std::nullopt,
                                       offset.count(),
                                       std::nullopt,
                                       HitEvent::Action::Press,
                                       /*noteRemoved=*/false });
        } else {
            score->sendVisualOnlyRelease(HitEvent{ static_cast<int>(key),
                                                   std::nullopt,
                                                   offset.count(),
                                                   std::nullopt,
                                                   HitEvent::Action::Release,
                                                   /*noteRemoved=*/false });
        }
    } else {
        if (eventType == Chart::EventType::KeyPress) {
            referee->passPressed(
              offset - std::chrono::nanoseconds(timeBeforeChartStart), key);
        } else {
            referee->passReleased(
              offset - std::chrono::nanoseconds(timeBeforeChartStart), key);
        }
    }
}
void
Player::setup()
{
    referee = refereeFuture.takeResult();
    setStatus(Chart::Status::Ready);
}
auto
Player::getNotes() const -> BmsNotes*
{
    return notes;
}
auto
Player::getScore() const -> BmsScore*
{
    return score;
}
auto
Player::getState() const -> GameplayState*
{
    return state;
}
auto
Player::getProfile() const -> resource_managers::Profile*
{
    return profile;
}
auto
Player::getPosition() const -> double
{
    return position;
}
auto
Player::getElapsed() const -> int64_t
{
    return elapsed;
}
auto
Player::getPositionBeforeChartStart() const -> double
{
    return positionBeforeChartStart;
}
auto
Player::getTimeBeforeChartStart() const -> int64_t
{
    return timeBeforeChartStart;
}
auto
Player::getStatus() const -> Chart::Status
{
    return status;
}
void
Player::setStatus(const Chart::Status status)
{
    if (this->status != status) {
        this->status = status;
        emit statusChanged();
    }
}
auto
Player::finish() const -> BmsScoreAftermath*
{
    auto result = score->getResult();
    auto replayData = score->getReplayData();
    auto gaugeHistory = score->getGaugeHistory();
    if (auto* profilePtr = profile.get()) {
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
    return new BmsScoreAftermath{ profile.get(),
                                  std::move(result),
                                  std::move(replayData),
                                  std::move(gaugeHistory) };
}
} // namespace gameplay_logic