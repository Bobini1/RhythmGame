//
// Created by PC on 08/05/2025.
//

#include "Rg.h"

Rg::Rg(
  qml_components::ProgramSettings* programSettings,
  input::InputTranslator* inputTranslator,
  qml_components::ChartLoader* chartLoader,
  qml_components::RootSongFoldersConfig* rootSongFoldersConfig,
  qml_components::SongFolderFactory* songFolderFactory,
  qml_components::SongDirectoryFilePathFetcher* songDirectoryFilePathFetcher,
  qml_components::FileQuery* fileQuery,
  qml_components::Themes* themes,
  input::GamepadManager* gamepadManager,
  qml_components::ProfileList* profileList,
  arena::ArenaSession* arenaSession,
  arena::ArenaSession* arenaDirectorySession,
  resource_managers::Tables* tables,
  resource_managers::Languages* languages,
  sounds::AudioEngine* audioEngine,
  qml_components::OnlineScores* onlineScores,
  resource_managers::SongAssetStore* songAssets,
  QObject* parent)
  : QObject(parent)
  , programSettings(programSettings)
  , inputTranslator(inputTranslator)
  , chartLoader(chartLoader)
  , rootSongFoldersConfig(rootSongFoldersConfig)
  , songFolderFactory(songFolderFactory)
  , songDirectoryFilePathFetcher(songDirectoryFilePathFetcher)
  , fileQuery(fileQuery)
  , themes(themes)
  , gamepadManager(gamepadManager)
  , profileList(profileList)
  , arenaSession(arenaSession)
  , arenaDirectorySession(arenaDirectorySession)
  , tables(tables)
  , languages(languages)
  , audioEngine(audioEngine)
  , onlineScores(onlineScores)
  , songAssets(songAssets)
{
    for (auto* session : { arenaSession, arenaDirectorySession }) {
        if (!session)
            continue;
        connect(session,
                &arena::ArenaSession::activeChanged,
                this,
                &Rg::arenaActivityChanged);
        connect(session,
                &arena::ArenaSession::stateChanged,
                this,
                &Rg::arenaActivityChanged);
        connect(session,
                &arena::ArenaSession::directoryReadyChanged,
                this,
                &Rg::arenaActivityChanged);
        connect(session->getRooms(),
                &arena::ArenaRoomListModel::countChanged,
                this,
                &Rg::arenaActivityChanged);
    }
}

auto
Rg::arenaRoomCount() const -> int
{
    auto* session = arenaSession && arenaSession->getActive()
                      ? arenaSession
                      : arenaDirectorySession;
    if (!session || !session->getDirectoryReady() ||
        (session->getState() != arena::ArenaSession::State::Browsing &&
         session->getState() != arena::ArenaSession::State::InRoom))
        return -1;
    return session->getRooms()->rowCount();
}
Rg*
Rg::create(QQmlEngine* qmlEngine, QJSEngine*)
{
    QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}
