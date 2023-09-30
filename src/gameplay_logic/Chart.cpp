//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

namespace gameplay_logic {

Chart::Chart(QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
             ChartData* chartData,
             BmsNotes* notes,
             BmsScore* score,
             charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart,
             std::function<db::SqliteCppDb&()> scoreDb,
             QObject* parent)
  : QObject(parent)
  , bpmChanges(notes->getBpmChanges())
  , chartData(chartData)
  , notes(notes)
  , score(score)
  , refereeFuture(std::move(refereeFuture))
  , scoreDb(std::move(scoreDb))
  , elapsed(-timeBeforeChartStart.timestamp.count())
  , position(-timeBeforeChartStart.position)
{
    chartData->setParent(this);
    notes->setParent(this);
    score->setParent(this);
    connect(&refereeFutureWatcher,
            &QFutureWatcher<gameplay_logic::BmsGameReferee>::finished,
            this,
            &Chart::setReferee);
    refereeFutureWatcher.setFuture(this->refereeFuture);
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
    updateBpm();
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::steady_clock::now() - startTimepoint;
    if (auto nanos = offset.count(); nanos != elapsed) {
        auto delta = nanos - elapsed;
        elapsed = nanos;
        emit elapsedChanged(delta);
    }
    auto newPosition = gameReferee->update(offset);
    if (newPosition != position) {
        auto delta = newPosition - position;
        position = newPosition;
        emit positionChanged(delta);
    }
    if (gameReferee->isOver()) {
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
Chart::passKey(QKeyEvent* keyEvent)
{
    if (keyEvent->isAutoRepeat()) {
        keyEvent->ignore();
        return;
    }
    auto timestampQint = keyEvent->timestamp();
    auto timestamp = std::chrono::steady_clock::time_point{
        std::chrono::milliseconds{ timestampQint }
    };
    if (auto bmsKey =
          inputTranslator.translate(static_cast<Qt::Key>(keyEvent->key()));
        bmsKey.has_value()) {
        if (!gameReferee) {
            emit score->sendVisualOnlyTap(
              { static_cast<int>(bmsKey.value()),
                std::nullopt,
                (timestamp - startTimepoint).count(),
                std::nullopt });
        } else {
            gameReferee->passInput(timestamp - startTimepoint, *bmsKey);
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
Chart::setReferee()
{
    gameReferee = refereeFuture.takeResult();
    if (startRequested) {
        start();
    }
}
auto
Chart::finish() -> BmsScoreAftermath*
{
    startRequested = false;
    propertyUpdateTimer.stop();
    // update
    if (!gameReferee) {
        return nullptr;
    }

    auto chartLength = chartData->getLength();
    if (elapsed < chartLength) {
        gameReferee->update(std::chrono::nanoseconds(chartLength));
    }
    try {
        auto result = score->getResult();
        auto replayData = score->getReplayData();
        auto gaugeHistory = score->getGaugeHistory();
        auto& currentScoreDb = scoreDb();
        auto scoreId =
          result->save(currentScoreDb, chartData->getSha256().toStdString());
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
} // namespace gameplay_logic