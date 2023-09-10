//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include "ChartFactory.h"

namespace resource_managers {
auto
ChartFactory::createChart(const QString& filename) -> gameplay_logic::Chart*
{
    auto [chartData, notesData, wavs] =
      chartDataFactory->loadChartData(filename);
    auto path = chartData->getDirectory().toString().toStdString();
    auto* score = chartData->createEmptyScore();
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
                 score] {
        auto sounds =
          charts::helper_functions::loadBmsSounds(wavs, std::move(path));
        return gameplay_logic::BmsGameReferee(notesData,
                                              score,
                                              std::move(sounds),
                                              gameplay_logic::BmsRules{},
                                              combinedTimeBeforeChartStart);
    };
    auto referee = QtConcurrent::run(std::move(task));
    return new gameplay_logic::Chart(
      std::move(referee), chartData, score, combinedTimeBeforeChartStart);
}
ChartFactory::ChartFactory(ChartDataFactory* chartDataFactory,
                           std::chrono::nanoseconds timeBeforeChartStart)
  : chartDataFactory(chartDataFactory)
  , timeBeforeChartStart(timeBeforeChartStart)
{
}
} // namespace resource_managers