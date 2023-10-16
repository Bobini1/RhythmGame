//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

namespace gameplay_logic {

Chart::Chart(QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
             QFuture<std::unique_ptr<qml_components::BgaContainer>> bgaFuture,
             ChartData* chartData,
             BmsNotes* notes,
             BmsScore* score,
             std::function<db::SqliteCppDb&()> scoreDb,
             QObject* parent)
  : QObject(parent)
  , bpmChanges(notes->getBpmChanges())
  , chartData(chartData)
  , notes(notes)
  , score(score)
  , refereeFuture(std::move(refereeFuture))
  , bgaFuture(std::move(bgaFuture))
  , scoreDb(std::move(scoreDb))
{
    chartData->setParent(this);
    notes->setParent(this);
    score->setParent(this);
    connect(&refereeFutureWatcher,
            &QFutureWatcher<gameplay_logic::BmsGameReferee>::finished,
            this,
            &Chart::setup);
    refereeFutureWatcher.setFuture(this->refereeFuture);
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
    if (!gameReferee) {
        startRequested = true;
        return;
    }
    emit started();
    propertyUpdateTimer.start(1);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimepoint = std::chrono::steady_clock::now();
#ifdef _WIN32
    startTimepointClk = clock();
#endif
    updateBpm();
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::steady_clock::now() - startTimepoint;
    offset -= std::chrono::nanoseconds(timeBeforeChartStart);
    setElapsed(offset.count());
    auto newPosition = gameReferee->update(offset);
    setPosition(newPosition);
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
Chart::passKey(QKeyEvent* keyEvent, EventType eventType)
{
    if (keyEvent->isAutoRepeat()) {
        keyEvent->ignore();
        return;
    }
    auto timestampQint = keyEvent->timestamp();
#ifdef _WIN32
    auto offset = std::chrono::nanoseconds(
      std::chrono::milliseconds{ timestampQint - startTimepointClk });
#else
    auto offset = std::chrono::steady_clock::time_point(
                    std::chrono::milliseconds{ timestampQint }) -
                  startTimepoint;
#endif
    if (auto bmsKey =
          inputTranslator.translate(static_cast<Qt::Key>(keyEvent->key()));
        bmsKey.has_value()) {
        keyEvent->accept();
        if (!gameReferee) {
            if (eventType == EventType::KeyPress) {
                score->sendVisualOnlyTap({ static_cast<int>(bmsKey.value()),
                                           std::nullopt,
                                           offset.count(),
                                           std::nullopt });
            } else {
                score->sendVisualOnlyRelease({ static_cast<int>(bmsKey.value()),
                                               std::nullopt,
                                               offset.count(),
                                               std::nullopt });
            }
        } else {
            if (eventType == EventType::KeyPress) {
                gameReferee->passPressed(
                  offset - std::chrono::nanoseconds(timeBeforeChartStart),
                  *bmsKey);
            } else {
                gameReferee->passReleased(
                  offset - std::chrono::nanoseconds(timeBeforeChartStart),
                  *bmsKey);
            }
        }
        return;
    }
    keyEvent->ignore();
}

auto
Chart::getChartData() const -> ChartData*
{
    return chartData;
}

auto
Chart::getNotes() const -> BmsNotes*
{
    return notes;
}
auto
Chart::getScore() const -> BmsScore*
{
    return score;
}
void
Chart::updateBpm()
{
    auto bpmChange = bpmChanges.begin();
    while (bpmChange != bpmChanges.end() &&
           bpmChange->time.timestamp <= elapsed) {
        emit bpmChanged(*bpmChange);
        bpmChange++;
    }
    if (bpmChange != bpmChanges.end()) {
        auto nextTimestamp = bpmChange->time.timestamp - elapsed;
        QTimer::singleShot(nextTimestamp, this, &Chart::updateBpm);
    }
    bpmChanges = std::span(bpmChange, bpmChanges.end());
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
    gameReferee = refereeFuture.takeResult();
    bga = bgaFuture.takeResult().release();
    bga->setParent(this);
    emit loaded();
    if (startRequested) {
        start();
    }
}
auto
Chart::finish() -> BmsScoreAftermath*
{
    startRequested = false;
    propertyUpdateTimer.stop();
    if (!gameReferee) {
        return nullptr;
    }
    // if we didn't get bga yet, cancel
    if (bga == nullptr) {
        bgaFutureWatcher.cancel();
        bgaFuture.cancel();
    }

    auto chartLength = chartData->getLength();
    if (elapsed < chartLength) {
        gameReferee->update(std::chrono::nanoseconds(chartLength),
                            /*lastUpdate=*/true);
    }
    try {
        auto result = score->getResult();
        auto replayData = score->getReplayData();
        auto gaugeHistory = score->getGaugeHistory();
        auto& currentScoreDb = scoreDb();
        auto scoreId =
          result->save(currentScoreDb, chartData->getSha256().toStdString());
        result->setId(scoreId);
        replayData->save(currentScoreDb, scoreId);
        gaugeHistory->save(currentScoreDb, scoreId);
        return new BmsScoreAftermath{ std::move(result),
                                      std::move(replayData),
                                      std::move(gaugeHistory) };
    } catch (const std::exception& e) {
        spdlog::error("Failed to save score: {}", e.what());
        return nullptr;
    }
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
        auto delta = newElapsed - elapsed;
        elapsed = newElapsed;
        emit elapsedChanged(delta);
    }
}
void
Chart::setPosition(double newPosition)
{
    if (newPosition != position) {
        auto delta = newPosition - position;
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