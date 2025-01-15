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
    input::InputTranslator* inputTranslator;

  public:
    struct PlayerSpecificData
    {
        QPointer<Profile> profile;
        QList<gameplay_logic::rules::BmsGauge*> gauges;
        std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules{};
    };
    explicit ChartFactory(input::InputTranslator* inputTranslator);
    auto createChart(ChartDataFactory::ChartComponents chartComponents,
                     std::optional<PlayerSpecificData> player1,
                     std::optional<PlayerSpecificData> player2,
                     double maxHitValue) -> gameplay_logic::Chart*;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
