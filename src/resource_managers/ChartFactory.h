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

class SoundTask : public QObject
{
    Q_OBJECT
    std::filesystem::path path;
    std::unordered_map<uint16_t, std::filesystem::path> wavs;

  public:
    SoundTask(std::filesystem::path path,
              std::unordered_map<uint16_t, std::filesystem::path> wavs);
    void run();
  signals:
    void soundsLoaded(std::unordered_map<uint16_t, sounds::OpenALSound> sounds);
};

class ChartFactory
{
    input::InputTranslator* inputTranslator;

  public:
    struct PlayerSpecificData
    {
        QPointer<Profile> profile;
        QList<gameplay_logic::rules::BmsGauge*> gauges;
        gameplay_logic::rules::StandardBmsHitRules hitRules;
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
