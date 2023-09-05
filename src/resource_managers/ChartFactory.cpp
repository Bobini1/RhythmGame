//
// Created by bobini on 24.08.23.
//

#include "ChartFactory.h"

namespace resource_managers {
auto
ChartFactory::createChart(const QString& filename) -> gameplay_logic::Chart*
{
    auto [chartData, notesData, wavs] =
      chartDataFactory->loadChartData(filename);
    auto path = chartData->getDirectory().toString().toStdString();
    auto sounds =
      charts::helper_functions::loadBmsSounds(wavs, std::move(path));
    auto* score = chartData->createEmptyScore();
    auto referee = gameplay_logic::BmsGameReferee(
      notesData, score, sounds, gameplay_logic::BmsRules{});
    return new gameplay_logic::Chart(
      std::move(referee), chartData, score, std::move(sounds));
}
ChartFactory::ChartFactory(ChartDataFactory* chartDataFactory)
  : chartDataFactory(chartDataFactory)
{
}
} // namespace resource_managers