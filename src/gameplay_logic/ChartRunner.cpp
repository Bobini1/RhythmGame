//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "ChartRunner.h"

#include "resource_managers/Profile.h"
#include "support/GeneratePermutation.h"

using namespace std::chrono_literals;
namespace gameplay_logic {

ChartRunner::ChartRunner(
  ChartData* chartData,
  QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
  ChartData::Keymode keymode,
  Player* player1,
  Player* player2,
  QObject* parent)
  : QObject(parent)
  , player1(player1)
  , player2(player2)
  , chartData(chartData)
  , bgaFuture(std::move(bgaFuture))
  , keymode(keymode)
{
    player1->setParent(this);
    if (player2 != nullptr) {
        player2->setParent(this);
    }
    chartData->setParent(this);
    connect(&bgaFutureWatcher,
            &QFutureWatcher<qml_components::Bga*>::finished,
            this,
            &ChartRunner::setup);
    bgaFutureWatcher.setFuture(this->bgaFuture);
    connect(player1, &Player::statusChanged, this, [player1, this] {
        if (player1->getStatus() == Ready) {
            setup();
        }
    });
    if (player2) {
        connect(player2, &Player::statusChanged, this, [player2, this] {
            if (player2->getStatus() == Ready) {
                setup();
            }
        });
    }
}

void
ChartRunner::start()
{
    if (player1->getStatus() == Loading ||
        (player2 != nullptr && player2->getStatus() == Loading) ||
        bgaFuture.isRunning()) {
        startRequested = true;
        return;
    }
    setStatus(Running);
    propertyUpdateTimer.start(1);
    connect(&propertyUpdateTimer,
            &QTimer::timeout,
            this,
            &ChartRunner::updateElapsed);
    startTimepoint = std::chrono::steady_clock::now();
}

void
ChartRunner::updateElapsed()
{
    const auto offset = std::chrono::steady_clock::now() - startTimepoint;
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
ChartRunner::passKey(input::BmsKey key,
                     const EventType eventType,
                     const int64_t time)
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
    auto offset =
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
ChartRunner::getChartData() const -> ChartData*
{
    return chartData;
}
auto
ChartRunner::getKeymode() const -> ChartData::Keymode
{
    return keymode;
}
auto
ChartRunner::getStatus() const -> Status
{
    return status;
}

void
ChartRunner::setStatus(const Status status)
{
    if (this->status != status) {
        this->status = status;
        player1->setStatus(status);
        if (player2 != nullptr) {
            player2->setStatus(status);
        }
        emit statusChanged();
    }
}
void
ChartRunner::setup()
{
    if (++numberOfSetupCalls != (player2 != nullptr ? 3 : 2) ||
        status != Loading) {
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
ChartRunner::finish() -> QList<BmsScore*>
{
    startRequested = false;
    propertyUpdateTimer.stop();

    // if we didn't get bga yet, cancel
    if (bga == nullptr) {
        bgaFutureWatcher.cancel();
        bgaFuture.cancel();
    }

    auto ret = QList<BmsScore*>{};
    if (player1->getStatus() == Running) {
        player1->update(std::chrono::nanoseconds{ player1->getChartLength() } +
                          10s,
                        /*lastUpdate=*/true);
    }
    if (player2 != nullptr && player2->getStatus() == Running) {
        player2->update(std::chrono::nanoseconds{ player2->getChartLength() } +
                          10s,
                        /*lastUpdate=*/true);
    }
    ret.append(player1->finish());
    if (player2 != nullptr) {
        ret.push_back(player2->finish());
    }
    setStatus(Finished);
    return ret;
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
ChartRunner::getBga() const -> qml_components::BgaContainer*
{
    return bga;
}
auto
ChartRunner::getPlayer1() const -> Player*
{
    return player1;
}
auto
ChartRunner::getPlayer2() const -> Player*
{
    return player2;
}
Player::Player(BmsNotes* notes,
               BmsLiveScore* score,
               GameplayState* state,
               resource_managers::Profile* profile,
               QFuture<BmsGameReferee> referee,
               std::chrono::nanoseconds chartLength,
               double initialBpm,
               QObject* parent)
  : QObject(parent)
  , notes(notes)
  , state(state)
  , profile(profile)
  , chartLength(chartLength)
  , refereeFuture(std::move(referee))
  , score(score)
  , bpm(initialBpm)
{
    notes->setParent(this);
    score->setParent(this);
    state->setParent(this);
    connect(&refereeWatcher,
            &QFutureWatcher<BmsGameReferee>::finished,
            this,
            &Player::setup);
    refereeWatcher.setFuture(refereeFuture);
    for (auto [index, column] :
         std::ranges::views::enumerate(state->getColumnStates())) {
        connect(score,
                &BmsLiveScore::hit,
                column,
                [column, index](const HitEvent& event) {
                    if (index == event.getColumn()) {
                        column->onHitEvent(event);
                    }
                });
    }
}
void
Player::setBpm(double newBpm)
{
    if (newBpm != bpm) {
        bpm = newBpm;
        emit bpmChanged();
    }
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
    setElapsed(offsetFromStart.count());
    if (referee) {
        referee->update(offsetFromStart, lastUpdate);
        const auto visualOffset =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double, std::milli>(
              profile ? profile->getVars()->getGeneralVars()->getOffset()
                      : 0.0));
        auto bpmChange = referee->getBpm(offsetFromStart + visualOffset);
        setBpm(bpmChange.second);
        setPosition(
          referee->getPosition(bpmChange, offsetFromStart + visualOffset));
        if (lastUpdate) {
            for (auto i = 0; i < charts::BmsNotesData::columnNumber; ++i) {
                referee->passReleased(
                  std::chrono::nanoseconds{ getChartLength() } + 10s,
                  static_cast<input::BmsKey>(i));
            }
        }
        if (offsetFromStart >= chartLength + 5s) {
            setStatus(ChartRunner::Finished);
        }
    }
}
void
Player::passKey(input::BmsKey key,
                ChartRunner::EventType eventType,
                std::chrono::nanoseconds offset)
{
    if (!referee || status == ChartRunner::Status::Finished) {
        if (eventType == ChartRunner::EventType::KeyPress) {
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
        if (eventType == ChartRunner::EventType::KeyPress) {
            referee->passPressed(offset, key);
        } else {
            referee->passReleased(offset, key);
        }
    }
}
void
Player::setup()
{
    referee = refereeFuture.takeResult();
    setStatus(ChartRunner::Status::Ready);
}
auto
Player::getNotes() const -> BmsNotes*
{
    return notes;
}
auto
Player::getScore() const -> BmsLiveScore*
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
Player::getStatus() const -> ChartRunner::Status
{
    return status;
}
void
Player::setStatus(const ChartRunner::Status status)
{
    if (this->status != status) {
        this->status = status;
        emit statusChanged();
    }
}
auto
Player::getChartLength() const -> int64_t
{
    return chartLength.count();
}
double
Player::getBpm() const
{
    return bpm;
}
auto
Player::finish() -> BmsScore*
{
    if (refereeFuture.isRunning()) {
        refereeFuture.cancel();
    }
    if (status == ChartRunner::Status::Loading) {
        return nullptr;
    }
    auto result = score->getResult();
    auto replayData = score->getReplayData();
    auto gaugeHistory = score->getGaugeHistory();
    auto score = std::make_unique<BmsScore>(
      std::move(result), std::move(replayData), std::move(gaugeHistory));
    if (auto* profilePtr = profile.get()) {
        try {
            score->save(profilePtr->getDb());
        } catch (const std::exception& e) {
            spdlog::error("Failed to save score: {}", e.what());
        }
    } else {
        spdlog::warn("Profile was deleted before saving score");
    }
    return score.release();
}
RePlayer::RePlayer(BmsNotes* notes,
                   BmsLiveScore* score,
                   GameplayState* state,
                   resource_managers::Profile* profile,
                   QFuture<BmsGameReferee> referee,
                   std::chrono::nanoseconds chartLength,
                   double initialBpm,
                   BmsScore* replayedScore,
                   QObject* parent)
  : Player(notes,
           score,
           state,
           profile,
           std::move(referee),
           chartLength,
           initialBpm,
           parent)
  , replayedScore(replayedScore)
  , events(replayedScore->getReplayData()->getHitEvents())
{
    replayedScore->setParent(this);
}
void
RePlayer::passKey(input::BmsKey key,
                  ChartRunner::EventType eventType,
                  std::chrono::nanoseconds offset)
{
}
void
RePlayer::update(const std::chrono::nanoseconds offsetFromStart,
                 const bool lastUpdate)
{
    while (!events.empty() &&
           events.front().getOffsetFromStart() <= offsetFromStart.count()) {
        const auto event = events.front();
        events = events.subspan(1);
        const auto hitOffset =
          std::chrono::nanoseconds{ event.getOffsetFromStart() };
        if (event.getAction() == HitEvent::Action::Press) {
            referee->update(hitOffset, lastUpdate);
            referee->passPressed(hitOffset,
                                 static_cast<input::BmsKey>(event.getColumn()));
        } else if (event.getAction() == HitEvent::Action::Release) {
            referee->update(hitOffset, lastUpdate);
            referee->passReleased(
              hitOffset, static_cast<input::BmsKey>(event.getColumn()));
        }
    }
    Player::update(offsetFromStart, lastUpdate);
}
auto
RePlayer::getReplayedScore() const -> BmsScore*
{
    return replayedScore;
}
AutoPlayer::AutoPlayer(BmsNotes* notes,
                       BmsLiveScore* score,
                       GameplayState* state,
                       resource_managers::Profile* profile,
                       QFuture<BmsGameReferee> referee,
                       std::chrono::nanoseconds chartLength,
                       double initialBpm,
                       std::vector<HitEvent> events,
                       QObject* parent)
  : Player(notes,
           score,
           state,
           profile,
           std::move(referee),
           chartLength,
           initialBpm,
           parent)
  , eventsVec(std::move(events))
  , events(eventsVec)
{
}
void
AutoPlayer::passKey(input::BmsKey key,
                    ChartRunner::EventType eventType,
                    std::chrono::nanoseconds offset)
{
}
void
AutoPlayer::update(std::chrono::nanoseconds offsetFromStart, bool lastUpdate)
{
    while (!events.empty() &&
           events.front().getOffsetFromStart() <= offsetFromStart.count()) {
        const auto event = events.front();
        events = events.subspan(1);
        const auto hitOffset =
          std::chrono::nanoseconds{ event.getOffsetFromStart() };
        if (event.getAction() == HitEvent::Action::Press) {
            referee->update(hitOffset, lastUpdate);
            referee->passPressed(hitOffset,
                                 static_cast<input::BmsKey>(event.getColumn()));
        } else if (event.getAction() == HitEvent::Action::Release) {
            referee->update(hitOffset, lastUpdate);
            referee->passReleased(
              hitOffset, static_cast<input::BmsKey>(event.getColumn()));
        }
    }
    Player::update(offsetFromStart, lastUpdate);
}
} // namespace gameplay_logic