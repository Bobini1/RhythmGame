//
// Created by bobini on 18.08.23.
//

#include <algorithm>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include "ChartRunner.h"

using namespace std::chrono_literals;
namespace gameplay_logic {

namespace {

constexpr auto inputMappingSize = 16;

auto
mappingKeyFor(const input::BmsKey key) -> input::BmsKey
{
    switch (key) {
        case input::BmsKey::Col1sDown:
            return input::BmsKey::Col1sUp;
        case input::BmsKey::Col2sDown:
            return input::BmsKey::Col2sUp;
        default:
            return key;
    }
}

auto
restoreScratchDirection(input::BmsKey mapped, const input::BmsKey original)
  -> input::BmsKey
{
    if (original == input::BmsKey::Col1sDown &&
        mapped == input::BmsKey::Col1sUp) {
        return input::BmsKey::Col1sDown;
    }
    if (original == input::BmsKey::Col2sDown &&
        mapped == input::BmsKey::Col2sUp) {
        return input::BmsKey::Col2sDown;
    }
    return mapped;
}

} // namespace

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
    auto p1keymode = player1->getScore()->getKeymode();
    if (p1keymode == ChartData::Keymode::K10) {
        inputMapping = { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 8, 9, 10, 11, 12, 15 };
    } else {
        inputMapping = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
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
    const auto keyIndex = static_cast<int>(mappingKeyFor(key));
    if (keyIndex < 0 || keyIndex >= inputMapping.size()) {
        spdlog::error("Input key {} is outside input mapping bounds {}",
                      keyIndex,
                      inputMapping.size());
        return;
    }
    const auto mappedIndex = inputMapping[keyIndex];
    if (mappedIndex < 0 || mappedIndex >= inputMappingSize) {
        spdlog::error("Mapped input key {} is outside valid range",
                      mappedIndex);
        return;
    }
    auto mapped =
      restoreScratchDirection(static_cast<input::BmsKey>(mappedIndex), key);
    const auto index = playerIndexFromKey(mapped);
    // key pressed for a player side that is not present
    if (!isDp(chartData->getKeymode()) && index == 1 && player2 == nullptr) {
        return;
    }
    auto offset =
      std::chrono::milliseconds{ time } - startTimepoint.time_since_epoch();
    auto* player =
      isDp(chartData->getKeymode()) || index == 0 ? player1 : player2;
    mapped = isDp(chartData->getKeymode()) ? mapped : convertToP1Key(mapped);
    if (!isDp(chartData->getKeymode())) {
        mapped = convertToP1Key(mapped);
    }
    player->passKey(mapped, eventType, offset);
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
    ret.push_back(player1->finish(*chartData));
    if (player2 != nullptr) {
        ret.push_back(player2->finish(*chartData));
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
auto
ChartRunner::getInputMapping() const -> QList<int>
{
    return inputMapping;
}
void
ChartRunner::setInputMapping(QList<int> inputMapping)
{
    if (this->inputMapping == inputMapping) {
        return;
    }
    if (inputMapping.size() != inputMappingSize) {
        spdlog::error("Invalid input mapping size: {}", inputMapping.size());
        return;
    }
    // validate input mapping
    auto sortedInputMapping = inputMapping;
    std::ranges::sort(sortedInputMapping);
    auto expectedInputMapping = QList<int>{};
    expectedInputMapping.reserve(inputMappingSize);
    for (auto i = 0; i < inputMappingSize; ++i) {
        expectedInputMapping.append(i);
    }
    if (sortedInputMapping != expectedInputMapping) {
        spdlog::error("Invalid input mapping: {}",
                      fmt::join(inputMapping, ","));
        return;
    }
    this->inputMapping = inputMapping;
    emit inputMappingChanged();
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
Player::setScroll(double second)
{
    if (second != scroll) {
        scroll = second;
        emit scrollChanged();
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
Player::setBeatPosition(BmsGameReferee::Position beatPosition)
{
    if (beatPosition != this->beatPosition) {
        const auto delta = beatPosition - this->beatPosition;
        this->beatPosition = beatPosition;
        emit beatPositionChanged(delta);
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
        const auto bpmChange = referee->getBpm(offsetFromStart + visualOffset);
        setBpm(bpmChange.bpm);
        setScroll(bpmChange.scroll);
        auto position =
          referee->getPosition(bpmChange, offsetFromStart + visualOffset);
        setPosition(position.position);
        setBeatPosition(position.beatPosition);
        if (lastUpdate) {
            for (auto i = static_cast<int>(input::BmsKey::Col11);
                 i <= static_cast<int>(input::BmsKey::Col2sDown);
                 ++i) {
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
    const auto logicalColumn = [&key] {
        if (key == input::BmsKey::Col1sDown) {
            return static_cast<int>(input::BmsKey::Col1sUp);
        }
        if (key == input::BmsKey::Col2sDown) {
            return static_cast<int>(input::BmsKey::Col2sUp);
        }
        return static_cast<int>(key);
    }();
    if (!referee || status == ChartRunner::Status::Finished) {
        if (eventType == ChartRunner::EventType::KeyPress) {
            score->sendVisualOnlyTap({ logicalColumn,
                                       key,
                                       std::nullopt,
                                       offset.count(),
                                       std::nullopt,
                                       HitEvent::Action::Press,
                                       /*noteRemoved=*/false });
        } else {
            score->sendVisualOnlyRelease(HitEvent{ logicalColumn,
                                                   key,
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
Player::getBeatPosition() const -> double
{
    return beatPosition;
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
double
Player::getScroll() const
{
    return scroll;
}
auto
Player::finish(const ChartData& chartData) -> BmsScore*
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
        if (profilePtr->getLoginState() ==
              resource_managers::Profile::LoginState::LoggedIn &&
            !score->getResult()->getGuid().isEmpty() &&
            score->getSubmissionState() !=
              BmsScore::SubmissionState::Submitted) {
            score->setSubmissionState(BmsScore::SubmissionState::Submitting);
            auto* submission = profilePtr->submitScore(*score, chartData);
            connect(
              submission,
              &QNetworkReply::finished,
              score.get(),
              [score = score.get(), submission]() {
                  if (submission->error() != QNetworkReply::NoError) {
                      const auto qtError = submission->error();
                      const auto httpStatus =
                        submission
                          ->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                          .toInt();
                      const auto contentTypeHeader =
                        submission->header(QNetworkRequest::ContentTypeHeader)
                          .toString();
                      const QByteArray respBody = submission->readAll();
                      submission->deleteLater();
                      spdlog::error(
                        "Score upload failed: qtError={} httpStatus={} "
                        "qtErrorStr={} contentType={} respBody={}",
                        static_cast<int>(qtError),
                        httpStatus,
                        submission->errorString().toStdString(),
                        contentTypeHeader.toStdString(),
                        std::string(respBody.constData(), respBody.size()));
                      if (submission
                            ->attribute(
                              QNetworkRequest::HttpStatusCodeAttribute)
                            .toInt() == 409) {
                          score->setSubmissionState(
                            BmsScore::SubmissionState::Duplicate);
                      } else {
                          score->setSubmissionState(
                            BmsScore::SubmissionState::Failed);
                      }
                  } else {
                      score->setSubmissionState(
                        BmsScore::SubmissionState::Submitted);
                  }
                  submission->deleteLater();
              });
        } else if (profilePtr->getLoginState() !=
                     resource_managers::Profile::LoginState::LoggedIn ||
                   score->getResult()->getGuid().isEmpty()) {
            score->setSubmissionState(BmsScore::SubmissionState::NotSubmitting);
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
            referee->passPressed(
              hitOffset,
              event.getKeyOptional().value_or(
                static_cast<input::BmsKey>(event.getColumn())));
        } else if (event.getAction() == HitEvent::Action::Release) {
            referee->update(hitOffset, lastUpdate);
            referee->passReleased(
              hitOffset,
              event.getKeyOptional().value_or(
                static_cast<input::BmsKey>(event.getColumn())));
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
            referee->passPressed(
              hitOffset,
              event.getKeyOptional().value_or(
                static_cast<input::BmsKey>(event.getColumn())));
        } else if (event.getAction() == HitEvent::Action::Release) {
            referee->update(hitOffset, lastUpdate);
            referee->passReleased(
              hitOffset,
              event.getKeyOptional().value_or(
                static_cast<input::BmsKey>(event.getColumn())));
        }
    }
    Player::update(offsetFromStart, lastUpdate);
}
} // namespace gameplay_logic
