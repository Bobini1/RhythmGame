//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include "ChartFactory.h"
#include "support/QStringToPath.h"

namespace resource_managers {
auto
ChartFactory::createChart(
  ChartDataFactory::ChartComponents chartComponents,
  std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules,
  QList<gameplay_logic::rules::BmsGauge*> gauges,
  double maxHitValue) -> gameplay_logic::Chart*
{
    auto& [chartData, notes, notesData, wavs] = chartComponents;
    auto path =
      support::qStringToPath(chartData->getPath()).parent_path();
    auto* score = new gameplay_logic::BmsScore(
      chartData->getNoteCount(), maxHitValue, std::move(gauges));
    auto task = [path,
                 wavs = std::move(wavs),
                 notesData = std::move(notesData),
                 score,
                 hitRules = std::move(hitRules)]() mutable {
        auto sounds =
          charts::helper_functions::loadBmsSounds(wavs, std::move(path));
        return gameplay_logic::BmsGameReferee(
          notesData, score, std::move(sounds), std::move(hitRules));
    };
    auto referee = QtConcurrent::run(std::move(task));
    return new gameplay_logic::Chart(
      std::move(referee), chartData.release(), notes.release(), score, scoreDb);
}
ChartFactory::ChartFactory(std::function<db::SqliteCppDb&()> scoreDb)
  : scoreDb(std::move(scoreDb))
{
}
} // namespace resource_managers