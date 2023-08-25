//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/Chart.h"
#include "charts/helper_functions/loadBmsSounds.h"
namespace resource_managers {

class ChartFactory
{
    ChartDataFactory* chartDataFactory;

  public:
    explicit ChartFactory(ChartDataFactory* chartDataFactory)
      : chartDataFactory(chartDataFactory)
    {
    }

    auto createChart(const QString& filename) -> gameplay_logic::Chart*
    {
        auto [chartData, notesData, wavs] =
          chartDataFactory->loadChartData(filename);
        auto path = chartData->getDirectory().toString().toStdString();
        auto sounds =
          charts::helper_functions::loadBmsSounds(wavs, std::move(path));
        auto referee =
          gameplay_logic::BmsGameReferee(notesData,
                                         chartData->createEmptyScore(),
                                         sounds,
                                         gameplay_logic::BmsRules{});
        return new gameplay_logic::Chart(std::move(referee),
                                         chartData,
                                         chartData->createEmptyScore(),
                                         std::move(sounds));
    }
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
