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
#include "resource_managers/Languages.h"
#include "resource_managers/Profile.h"
#include "resource_managers/Tables.h"
#include "sounds/AudioEngine.h"

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
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(
  SongDirectoryFilePathFetcherForeign,
  qml_components::SongDirectoryFilePathFetcher);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(FileQueryForeign,
                                 qml_components::FileQuery);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(ThemesForeign, qml_components::Themes);
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(GamepadManagerForeign,
                                 input::GamepadManager);
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

#undef RHYTHMGAME_QML_ANONYMOUS_FOREIGN

} // namespace rhythm_game_qml

#endif // QMLFOREIGNTYPES_H
