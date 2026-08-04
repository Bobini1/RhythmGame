#ifndef QMLFOREIGNTYPES_H
#define QMLFOREIGNTYPES_H

#include "input/GamepadManager.h"
#include "input/InputTranslator.h"
#include "qml_components/ChartLoader.h"
#include "qml_components/FileQuery.h"
#include "qml_components/Logger.h"
#include "qml_components/OnlineScores.h"
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

#include <QQmlEngine>
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
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(SongAssetStoreForeign,
                                 resource_managers::SongAssetStore);

#undef RHYTHMGAME_QML_ANONYMOUS_FOREIGN

} // namespace rhythm_game_qml

#endif // QMLFOREIGNTYPES_H
