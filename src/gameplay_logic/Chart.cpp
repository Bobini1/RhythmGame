//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

namespace gameplay_logic {
Chart::Chart(QFuture<gameplay_logic::BmsGameReferee> refereeFuture,
             ChartData* chartData,
             QSharedPointer<BmsScore> score,
             charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart,
             QObject* parent)
  : QObject(parent)
  , bpmChanges(chartData->getNoteData()->getBpmChanges())
  , chartData(chartData)
  , score(std::move(score))
  , refereeFuture(std::move(refereeFuture))
  , elapsed(-std::chrono::duration_cast<std::chrono::milliseconds>(
               timeBeforeChartStart.timestamp)
               .count())
  , position(-timeBeforeChartStart.position)
{
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
    propertyUpdateTimer.start(0);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimepoint = std::chrono::steady_clock::now();
    updateBpm();
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::steady_clock::now() - startTimepoint;
    if (auto millis =
          std::chrono::duration_cast<std::chrono::milliseconds>(offset).count();
        millis != elapsed) {
        auto delta = millis - elapsed;
        elapsed = millis;
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
        emit overChanged();
    }
}

auto
Chart::getElapsed() const -> int
{
    return elapsed;
}

void
Chart::passKey(int key)
{
    auto timestamp = std::chrono::steady_clock::now();
    if (auto bmsKey = inputTranslator.translate(static_cast<Qt::Key>(key));
        bmsKey.has_value()) {
        if (!gameReferee) {
            emit score->sendVisualOnlyTap(
              { static_cast<int>(bmsKey.value()),
                std::nullopt,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                  timestamp - startTimepoint)
                  .count(),
                std::nullopt });
        } else {
            gameReferee->passInput(timestamp - startTimepoint, *bmsKey);
        }
    }
}

auto
Chart::getChartData() const -> ChartData*
{
    return chartData;
}
auto
Chart::getScore() const -> BmsScore*
{
    return score.get();
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
} // namespace gameplay_logic