//
// Created by bobini on 18.08.23.
//

#include <QKeyEvent>
#include <spdlog/spdlog.h>
#include "Chart.h"

namespace gameplay_logic {
Chart::Chart(gameplay_logic::BmsGameReferee gameReferee,
             ChartData* chartData,
             std::unordered_map<std::string, sounds::OpenALSound> sounds,
             QObject* parent)
  : QObject(parent)
  , gameReferee(std::move(gameReferee))
  , sounds(std::move(sounds))
  , chartData(chartData)
{
}

void
Chart::start()
{
    propertyUpdateTimer.start(0);
    connect(
      &propertyUpdateTimer, &QTimer::timeout, this, &Chart::updateElapsed);
    startTimestamp =
      QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier).timestamp();
    startTimepoint = std::chrono::steady_clock::now();
    spdlog::info("startTimestamp: {}", startTimestamp);
}

void
Chart::updateElapsed()
{
    auto offset = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - startTimepoint);
    if (auto millis = offset.count(); millis != elapsed) {
        elapsed = millis;
        emit elapsedChanged();
    }
    gameReferee.update(offset);
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
Chart::passKey(QKeyEvent* event)
{
    if (auto bmsKey =
          inputTranslator.translate(static_cast<Qt::Key>(event->key()));
        bmsKey.has_value()) {
        gameReferee.passInput(
          std::chrono::milliseconds(event->timestamp() - startTimestamp),
          *bmsKey);
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
    return nullptr;
}
} // namespace gameplay_logic