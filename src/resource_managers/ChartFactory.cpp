//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include "ChartFactory.h"

namespace resource_managers {
auto
ChartFactory::createChart(
  ChartDataFactory::ChartComponents chartComponents,
  std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules,
  QList<gameplay_logic::rules::BmsGauge*> gauges,
  double maxHitValue) -> gameplay_logic::Chart*
{
    auto& [chartData, notesData, wavs] = chartComponents;
    auto path =
      std::filesystem::path(chartData->getPath().toStdString()).parent_path();
    auto* score = new gameplay_logic::BmsScore(
      chartData->getNoteCount(), maxHitValue, std::move(gauges));
    auto beatsBeforeChartStart =
      std::chrono::duration<double>(timeBeforeChartStart).count() *
      chartData->getNoteData()->getBpmChanges().first().bpm / 60;
    auto combinedTimeBeforeChartStart =
      charts::gameplay_models::BmsNotesData::Time{ timeBeforeChartStart,
                                                   beatsBeforeChartStart };
    auto task = [path,
                 combinedTimeBeforeChartStart,
                 wavs = std::move(wavs),
                 notesData = std::move(notesData),
                 score,
                 hitRules = std::move(hitRules)]() mutable {
        auto sounds =
          charts::helper_functions::loadBmsSounds(wavs, std::move(path));
        return gameplay_logic::BmsGameReferee(notesData,
                                              score,
                                              std::move(sounds),
                                              std::move(hitRules),
                                              combinedTimeBeforeChartStart);
    };
    auto referee = QtConcurrent::run(std::move(task));
    return new gameplay_logic::Chart(
      std::move(referee), chartData, score, combinedTimeBeforeChartStart);
}
ChartFactory::ChartFactory(std::chrono::nanoseconds timeBeforeChartStart)
  : timeBeforeChartStart(timeBeforeChartStart)
{
}
} // namespace resource_managers