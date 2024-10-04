//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

#include "resource_managers/Profile.h"

namespace gameplay_logic {

Chart::
Chart(QFuture<std::vector<BmsGameReferee>> refereesFuture,
      QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
      ChartData* chartData,
      BmsNotes* notes,
      QList<BmsScore*> scores,
      QList<resource_managers::Profile*> profiles,
      QObject* parent)
  : QObject(parent)
  , bpmChanges(notes->getBpmChanges())
  , chartData(chartData)
  , notes(notes)
  , scores(std::move(scores))
  , refereesFuture(std::move(refereesFuture))
  , bgaFuture(std::move(bgaFuture))
{
    for (auto* profile : profiles) {
        this->profiles.append(profile);
    }
    chartData->setParent(this);
    notes->setParent(this);
    for (auto* score : this->scores) {
        score->setParent(this);
    }
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
    if (gameReferees.empty()) {
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
    for (auto& referee : gameReferees) {
        setPosition(referee.update(offset));
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
    auto offset =
      std::chrono::milliseconds{ time } - startTimepoint.time_since_epoch();
    if (gameReferees.empty()) {
        if (eventType == EventType::KeyPress) {
            score->sendVisualOnlyTap({ static_cast<int>(key),
                                       std::nullopt,
                                       offset.count(),
                                       std::nullopt });
        } else {
            score->sendVisualOnlyRelease({ static_cast<int>(key),
                                           std::nullopt,
                                           offset.count(),
                                           std::nullopt });
        }
    } else {
        if (eventType == EventType::KeyPress) {
            gameReferee->passPressed(
              offset - std::chrono::nanoseconds(timeBeforeChartStart), key);
        } else {
            gameReferee->passReleased(
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
Chart::getNotes() const -> const BmsNotes*
{
    return notes;
}
auto
Chart::getScores() const -> const QList<BmsScore*>&
{
    return scores;
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
Chart::getProfiles() const -> QList<resource_managers::Profile*>
{
    auto result = QList<resource_managers::Profile*>{};
    result.reserve(profiles.size());
    std::ranges::transform(profiles, std::back_inserter(result), [](auto& ptr) {
        return ptr.data();
    });
    return result;
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
    gameReferees = refereeFuture.takeResult();
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
    if (gameReferees.empty()) {
        return {};
    }
    // if we didn't get bga yet, cancel
    if (bga == nullptr) {
        bgaFutureWatcher.cancel();
        bgaFuture.cancel();
    }

    const auto chartLength = chartData->getLength();
    if (elapsed < chartLength) {
        for (auto& referee : gameReferees) {
            referee.update(std::chrono::nanoseconds(chartLength),
                           /*lastUpdate=*/true);
        }
    }
    auto ret = QList<BmsScoreAftermath*>{};
    auto i = -1;
    for (const auto* score : scores) {
        i++;
        auto result = score->getResult();
        auto replayData = score->getReplayData();
        auto gaugeHistory = score->getGaugeHistory();
        if (auto* profilePtr = profiles[i].get()) {
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
        ret.append(new BmsScoreAftermath{ profiles[i].get(),
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
      std::chrono::duration<double>(timeBeforeChartStart).count() *
      notes->getBpmChanges().first().bpm / 60;
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
auto
Chart::getBga() const -> qml_components::BgaContainer*
{
    return bga;
}
} // namespace gameplay_logic