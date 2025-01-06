//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/Chart.h"
#include "input/InputTranslator.h"
namespace resource_managers {
class InputTranslators;
}
namespace qml_components {
class ProfileList;
} // namespace qml_components
namespace resource_managers {

class ChartFactory
{
  public:
    auto createChart(
      ChartDataFactory::ChartComponents chartComponents,
      std::vector<std::unique_ptr<gameplay_logic::rules::BmsHitRules>> hitRules,
      std::vector<QList<gameplay_logic::rules::BmsGauge*>> gauges,
      const QList<Profile*>& profiles,
      const QList<input::InputTranslator*>& inputTranslators,
      double maxHitValue) -> gameplay_logic::Chart*;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
