//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/Chart.h"
#include "input/InputTranslator.h"
namespace resource_managers {

class ChartFactory
{
    std::function<db::SqliteCppDb&()> scoreDb;
    input::InputTranslator* inputTranslator;

  public:
    explicit ChartFactory(std::function<db::SqliteCppDb&()> scoreDb,
                          input::InputTranslator* inputTranslator);

    auto createChart(
      ChartDataFactory::ChartComponents chartComponents,
      std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules,
      QList<gameplay_logic::rules::BmsGauge*> gauges,
      double maxHitValue) -> gameplay_logic::Chart*;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
