//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

namespace gameplay_logic {
Chart::Chart(gameplay_logic::BmsGameReferee gameReferee,
             ChartData* chartData,
             BmsScore* score,
             std::unordered_map<std::string, sounds::OpenALSound> sounds,
             QObject* parent)
  : QObject(parent)
  , gameReferee(std::move(gameReferee))
  , sounds(std::move(sounds))
  , chartData(chartData)
  , score(score)
  , bpmChanges(chartData->getNoteData()->getBpmChanges())
{
    chartData->setParent(this);
    score->setParent(this);
}

void
Chart::start()
{
    propertyUpdateTimer.start(0);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimepoint = std::chrono::steady_clock::now();
    updateBpm();
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - startTimepoint);
    if (auto millis = offset.count(); millis != elapsed) {
        auto delta = millis - elapsed;
        elapsed = millis;
        emit elapsedChanged(delta);
    }
    auto newPosition = gameReferee.update(offset);
    if (newPosition != position) {
        auto delta = newPosition - position;
        position = newPosition;
        emit positionChanged(delta);
    }
    if (gameReferee.isOver()) {
        propertyUpdateTimer.stop();
        over = true;
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
        gameReferee.passInput(timestamp - startTimepoint, *bmsKey);
    }
}

auto
Chart::isOver() const -> bool
{
    return over;
}
auto
Chart::getChartData() const -> ChartData*
{
    return chartData;
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
} // namespace gameplay_logic