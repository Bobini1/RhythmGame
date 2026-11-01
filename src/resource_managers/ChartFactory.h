//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "loadBmsSounds.h"
#include "gameplay_logic/ChartRunner.h"
#include "input/InputTranslator.h"
#include "charts/BmsNotesData.h"
#include <QThreadPool>
#include <stop_token>
namespace sounds {
class AudioEngine;
} // namespace sounds
namespace qml_components {
class ProfileList;
} // namespace qml_components
namespace resource_managers {

class SongAssetStore;

class SoundTask : public QObject
{
    Q_OBJECT
    std::filesystem::path path;
    std::unordered_map<uint64_t, std::filesystem::path> wavs;
    sounds::AudioEngine* engine;
    SongAssetStore* assetStore;
    std::stop_token stop;

    // bmson-specific (empty for BMS)
    std::vector<charts::BmsNotesData::BmsonSliceInfo> bmsonSlices;
    std::unordered_map<uint64_t, std::vector<uint64_t>> bmsonFusions;

  public:
    SoundTask(sounds::AudioEngine* engine,
              SongAssetStore* assetStore,
              std::filesystem::path path,
              std::unordered_map<uint64_t, std::filesystem::path> channelPaths,
              std::vector<charts::BmsNotesData::BmsonSliceInfo> slices,
              std::unordered_map<uint64_t, std::vector<uint64_t>> fusions,
              std::stop_token stop);
    void run();
  signals:
    void soundsLoaded(
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>> sounds);
};

class ChartFactory : public QObject
{
    sounds::AudioEngine* engine;
    input::InputTranslator* inputTranslator;
    SongAssetStore* assetStore;
    QThreadPool loadingPool;
    QList<std::weak_ptr<std::stop_source>> pendingLoads;

    void cancelLoading();

  public:
    struct PlayerSpecificData
    {
        QPointer<Profile> profile;
        QList<gameplay_logic::rules::BmsGauge*> gauges;
        gameplay_logic::rules::HitRules hitRules;
        gameplay_logic::BmsScore* replayedScore;
        NoteOrderAlgorithm noteOrderAlgorithm;
        NoteOrderAlgorithm noteOrderAlgorithmP2;
        DpOptions dpOptions = DpOptions::Off;
        uint64_t randomSeed;
        bool autoPlay = false;
        bool usePre130{};
    };
    ChartFactory(sounds::AudioEngine* engine,
                 input::InputTranslator* inputTranslator,
                 SongAssetStore* assetStore);
    ~ChartFactory() override;
    auto createChart(ChartDataFactory::ChartComponents chartComponents,
                     PlayerSpecificData player1,
                     std::optional<PlayerSpecificData> player2,
                     double maxHitValue)
      -> std::unique_ptr<gameplay_logic::ChartRunner>;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
