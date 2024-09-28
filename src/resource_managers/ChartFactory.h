//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/Chart.h"
#include "input/InputTranslator.h"
namespace qml_components {
class ProfileList;
} // namespace qml_components
namespace resource_managers {

class ChartFactory
{
    qml_components::ProfileList* profileList;

  public:
    explicit ChartFactory(qml_components::ProfileList* profile_list);

    auto createChart(
      ChartDataFactory::ChartComponents chartComponents,
      std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules,
      QList<gameplay_logic::rules::BmsGauge*> gauges,
      double maxHitValue) -> gameplay_logic::Chart*;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
