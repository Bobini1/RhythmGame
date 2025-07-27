//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/ChartRunner.h"
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
        std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules;
        gameplay_logic::BmsScore* replayedScore;
        bool autoPlay = false;
    };
    explicit ChartFactory(input::InputTranslator* inputTranslator);
    auto createChart(ChartDataFactory::ChartComponents chartComponents,
                     PlayerSpecificData player1,
                     std::optional<PlayerSpecificData> player2,
                     double maxHitValue)
      -> std::unique_ptr<gameplay_logic::ChartRunner>;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
