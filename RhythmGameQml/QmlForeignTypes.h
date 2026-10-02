#ifndef QMLFOREIGNTYPES_H
#define QMLFOREIGNTYPES_H

#include "input/GamepadManager.h"
#include "input/InputTranslator.h"
#include "qml_components/ChartLoader.h"
#include "qml_components/FileQuery.h"
#include "qml_components/Logger.h"
#include "qml_components/OnlineScores.h"
#include "qml_components/OnlineProfileInfo.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ProgramSettings.h"
#include "qml_components/ReplayImportOperation.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/ScoreDb.h"
#include "qml_components/ScoreSyncOperation.h"
#include "qml_components/SongDirectoryFilePathFetcher.h"
#include "qml_components/SongFolderFactory.h"
#include "qml_components/Themes.h"
#include "support/PendingReply.h"
#include "resource_managers/Languages.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/Profile.h"
#include "resource_managers/ChartFolderModel.h"
#include "resource_managers/Tables.h"
#include "sounds/AudioEngine.h"
#include "arena/ArenaChatModel.h"
#include "arena/ArenaAvailabilityIndex.h"
#include "arena/ArenaMemberListModel.h"
#include "arena/ArenaOpponentTarget.h"
#include "arena/ArenaResultModel.h"
#include "arena/ArenaRoomListModel.h"
#include "arena/ArenaSession.h"
#include "arena/ArenaStandingsModel.h"

#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/CourseRunner.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsPoints.h"
#include "gameplay_logic/HitEvent.h"
#include "qml_components/InputAttached.h"
#include "qml_components/QmlUtils.h"
#include "qml_components/ScoreReplayer.h"
#include "qml_components/Bga.h"
#include "sounds/AudioPlayer.h"
#include "sounds/ChartAudioPlayer.h"

#include <QQmlEngine>
#include <QStringListModel>
#include <QtCore/qassert.h>
#include <QtQml/qqmlregistration.h>

namespace rhythm_game_qml {

struct LoggerForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::Logger)
    QML_NAMED_ELEMENT(Logger)
    QML_SINGLETON

  public:
    inline static qml_components::Logger* instance = nullptr;

    static auto create(QQmlEngine*, QJSEngine*) -> qml_components::Logger*
    {
        Q_ASSERT(instance != nullptr);
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
        return instance;
    }
};

struct InputTranslatorForeign
{
    Q_GADGET
    QML_FOREIGN(input::InputTranslator)
    QML_NAMED_ELEMENT(InputTranslator)
    QML_UNCREATABLE("InputTranslator is provided by Rg.inputTranslator")
};

struct AnalogAxisConfigForeign
{
    Q_GADGET
    QML_FOREIGN(input::AnalogAxisConfig)
    QML_NAMED_ELEMENT(AnalogAxisConfig)
};

struct ProfileForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Profile)
    QML_NAMED_ELEMENT(Profile)
    QML_UNCREATABLE("Profile is managed by ProfileList")
};

struct ArenaSessionForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaSession)
    QML_NAMED_ELEMENT(ArenaSession)
    QML_UNCREATABLE("ArenaSession is provided by Rg.arenaSession")
};

struct ArenaAvailabilityIndexForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaAvailabilityIndex)
    QML_NAMED_ELEMENT(ArenaAvailabilityIndex)
    QML_UNCREATABLE("Arena availability is provided by ArenaSession")
};

struct ArenaStandingsModelForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaStandingsModel)
    QML_NAMED_ELEMENT(ArenaStandingsModel)
    QML_UNCREATABLE("ArenaStandingsModel is provided by ArenaSession")
};

struct ArenaResultModelForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaResultModel)
    QML_NAMED_ELEMENT(ArenaResultModel)
    QML_UNCREATABLE("ArenaResultModel is provided by ArenaSession")
};

struct ArenaOpponentTargetForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaOpponentTarget)
    QML_NAMED_ELEMENT(ArenaOpponentTarget)
    QML_UNCREATABLE("ArenaOpponentTarget is provided by ArenaSession")
};

struct ChartFolderModelForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::ChartFolderModel)
    QML_NAMED_ELEMENT(ChartFolderModel)
};

#define RHYTHMGAME_QML_ANONYMOUS_FOREIGN(WrapperName, ForeignType)             \
    struct WrapperName                                                         \
    {                                                                          \
        Q_GADGET                                                               \
        QML_FOREIGN(ForeignType)                                               \
        QML_ANONYMOUS                                                          \
        QML_UNCREATABLE("Provided by the game")                                \
    }

RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ProgramSettingsForeign,
                                 qml_components::ProgramSettings);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ChartLoaderForeign,
                                 qml_components::ChartLoader);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(RootSongFoldersConfigForeign,
                                 qml_components::RootSongFoldersConfig);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(SongFolderFactoryForeign,
                                 qml_components::SongFolderFactory);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(SongDirectoryFilePathFetcherForeign,
                                 qml_components::SongDirectoryFilePathFetcher);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(FileQueryForeign, qml_components::FileQuery);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ThemesForeign, qml_components::Themes);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(GamepadManagerForeign, input::GamepadManager);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ProfileListForeign,
                                 qml_components::ProfileList);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(BattleProfilesForeign,
                                 qml_components::BattleProfiles);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(VarsForeign, resource_managers::Vars);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(GeneralVarsForeign,
                                 resource_managers::GeneralVars);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ScoreDbForeign, qml_components::ScoreDb);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ScoreSyncOperationForeign,
                                 qml_components::ScoreSyncOperation);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ReplayImportOperationForeign,
                                 qml_components::ReplayImportOperation);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(TablesForeign, resource_managers::Tables);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(LanguagesForeign,
                                 resource_managers::Languages);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(AudioEngineForeign, sounds::AudioEngine);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(OnlineScoresForeign,
                                 qml_components::OnlineScores);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(PendingReplyForeign, support::PendingReply);
struct ArenaRoomListModelForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaRoomListModel)
    QML_ANONYMOUS
    QML_UNCREATABLE("Arena room models are provided by ArenaSession")
};

struct ArenaMemberListModelForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaMemberListModel)
    QML_ANONYMOUS
    QML_UNCREATABLE("Arena member models are provided by ArenaSession")
};

struct ArenaChatModelForeign
{
    Q_GADGET
    QML_FOREIGN(arena::ArenaChatModel)
    QML_ANONYMOUS
    QML_UNCREATABLE("Arena chat models are provided by ArenaSession")
};
struct SongAssetStoreForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::SongAssetStore)
    QML_ANONYMOUS
    QML_UNCREATABLE("Song assets are provided by Rg.songAssets")
};

struct LevelForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Level)
    QML_VALUE_TYPE(level)
};

struct CourseForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Course)
    QML_VALUE_TYPE(course)
};

struct TrophyForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Trophy)
    QML_VALUE_TYPE(trophy)
};

struct TableForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Table)
    QML_VALUE_TYPE(table)
};

struct EntryForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::Entry)
    QML_VALUE_TYPE(entry)
};

struct TableInfoForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::TableInfo)
    QML_VALUE_TYPE(tableInfo)
};

struct ChartRunnerForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::ChartRunner)
    QML_NAMED_ELEMENT(ChartRunner)
    QML_UNCREATABLE("Supplied by the game")
};

struct CourseRunnerForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::CourseRunner)
    QML_NAMED_ELEMENT(CourseRunner)
    QML_UNCREATABLE("Supplied by the game")
};

struct ChartDataForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::ChartData)
    QML_NAMED_ELEMENT(ChartData)
    QML_UNCREATABLE("Supplied by the game")
};

struct PlayerForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::Player)
    QML_NAMED_ELEMENT(Player)
    QML_UNCREATABLE("Supplied by the game")
};

struct AutoPlayerForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::AutoPlayer)
    QML_NAMED_ELEMENT(AutoPlayer)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsGaugeForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::rules::BmsGauge)
    QML_NAMED_ELEMENT(BmsGauge)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsLiveScoreForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsLiveScore)
    QML_NAMED_ELEMENT(BmsLiveScore)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsNotesForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsNotes)
    QML_NAMED_ELEMENT(BmsNotes)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsScoreForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsScore)
    QML_NAMED_ELEMENT(BmsScore)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsScoreCourseForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsScoreCourse)
    QML_NAMED_ELEMENT(BmsScoreCourse)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsResultCourseForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsResultCourse)
    QML_NAMED_ELEMENT(BmsResultCourse)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsPointsForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsPoints)
    QML_VALUE_TYPE(bmsPoints)
};

struct BmsResultForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsResult)
    QML_NAMED_ELEMENT(BmsResult)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsReplayDataForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsReplayData)
    QML_NAMED_ELEMENT(BmsReplayData)
    QML_UNCREATABLE("Supplied by the game")
};

struct BmsGaugeHistoryForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::BmsGaugeHistory)
    QML_NAMED_ELEMENT(BmsGaugeHistory)
    QML_UNCREATABLE("Supplied by the game")
};

struct HitEventForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::HitEvent)
    QML_VALUE_TYPE(hitEvent)
};

struct BgaForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::Bga)
    QML_NAMED_ELEMENT(Bga)
    QML_UNCREATABLE("Supplied by the game")
};

struct BgaContainerForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::BgaContainer)
    QML_NAMED_ELEMENT(BgaContainer)
    QML_UNCREATABLE("Supplied by the game")
};

struct KeyForeign
{
    Q_GADGET
    QML_FOREIGN(input::Key)
    QML_VALUE_TYPE(key)
};

struct GamepadForeign
{
    Q_GADGET
    QML_FOREIGN(input::Gamepad)
    QML_VALUE_TYPE(gamepad)
};

struct MidiDeviceForeign
{
    Q_GADGET
    QML_FOREIGN(input::MidiDevice)
    QML_VALUE_TYPE(midiDevice)
};

struct OnlineProfileInfoForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::OnlineProfileInfo)
    QML_VALUE_TYPE(onlineProfileInfo)
};

struct NoteForeign
{
    Q_GADGET
    QML_FOREIGN(gameplay_logic::Note)
    QML_VALUE_TYPE(note)
};

struct InputForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::InputAttached)
    QML_ATTACHED(qml_components::InputAttached)
    QML_NAMED_ELEMENT(Input)
    QML_UNCREATABLE("Supplied by the game")
};

struct AudioPlayerForeign
{
    Q_GADGET
    QML_FOREIGN(sounds::AudioPlayer)
    QML_NAMED_ELEMENT(AudioPlayer)
};

struct ChartAudioPlayerForeign
{
    Q_GADGET
    QML_FOREIGN(sounds::ChartAudioPlayer)
    QML_NAMED_ELEMENT(ChartAudioPlayer)
};

struct ScoreReplayerForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::ScoreReplayer)
    QML_NAMED_ELEMENT(ScoreReplayer)
};

struct OnlineRankingModelForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::OnlineRankingModel)
    QML_NAMED_ELEMENT(OnlineRankingModel)
};

struct RankingEntryForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::RankingEntry)
    QML_VALUE_TYPE(rankingEntry)
};

struct OnlineUserDataForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::OnlineUserData)
    QML_VALUE_TYPE(onlineUserData)
};

struct TachiDataForeign
{
    Q_GADGET
    QML_FOREIGN(resource_managers::TachiData)
    QML_VALUE_TYPE(tachiData)
};

struct QmlUtilsForeign
{
    Q_GADGET
    QML_FOREIGN(qml_components::QmlUtilsAttached)
    QML_ATTACHED(qml_components::QmlUtilsAttached)
    QML_NAMED_ELEMENT(QmlUtils)
    QML_UNCREATABLE("Supplied by the game")
};

namespace JudgementForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(gameplay_logic::judgement)
QML_NAMED_ELEMENT(Judgement)
}

namespace BmsKeyForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(input)
QML_NAMED_ELEMENT(BmsKey)
}

namespace DpOptionsForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::dp_options)
QML_NAMED_ELEMENT(DpOptions)
}

namespace HiSpeedFixForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::hi_speed_fix)
QML_NAMED_ELEMENT(HiSpeedFix)
}

namespace GaugeModeForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::gauge_mode)
QML_NAMED_ELEMENT(GaugeMode)
}

namespace ScoreTargetForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::score_target)
QML_NAMED_ELEMENT(ScoreTarget)
}

namespace SelectSortModeForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::select_sort_mode)
QML_NAMED_ELEMENT(SelectSortMode)
}

namespace SelectKeymodeFilterForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::select_keymode_filter)
QML_NAMED_ELEMENT(SelectKeymodeFilter)
}

namespace NoteOrderAlgorithmForeign {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(resource_managers::note_order_algorithm)
QML_NAMED_ELEMENT(NoteOrderAlgorithm)
}

RHYTHMGAME_QML_ANONYMOUS_FOREIGN(TimeForeign, gameplay_logic::Time);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(StringListModelForeign, QStringListModel);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(BpmChangeForeign, gameplay_logic::BpmChange);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(SnapForeign, gameplay_logic::Snap);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(CoursePlayerForeign,
                                 gameplay_logic::CoursePlayer);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(JudgementCountsForeign,
                                 gameplay_logic::JudgementCounts);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(BmsGaugeInfoForeign,
                                 gameplay_logic::BmsGaugeInfo);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(MappingForeign, input::Mapping);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(RootSongFolderForeign,
                                 qml_components::RootSongFolder);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(RootSongFoldersForeign,
                                 qml_components::RootSongFolders);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ScanningQueueForeign,
                                 qml_components::ScanningQueue);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ThemeFamilyForeign,
                                 qml_components::ThemeFamily);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ScreenForeign, qml_components::Screen);

#undef RHYTHMGAME_QML_ANONYMOUS_FOREIGN

} // namespace rhythm_game_qml

#endif // QMLFOREIGNTYPES_H
