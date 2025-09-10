//
// Created by PC on 08/05/2025.
//

#include "Rg.h"

Rg::Rg(qml_components::ProgramSettings* programSettings,
       input::InputTranslator* inputTranslator,
       qml_components::ChartLoader* chartLoader,
       qml_components::RootSongFoldersConfig* rootSongFoldersConfig,
       qml_components::SongFolderFactory* songFolderFactory,
       qml_components::PreviewFilePathFetcher* previewFilePathFetcher,
       qml_components::FileQuery* fileQuery,
       qml_components::Themes* themes,
       input::GamepadManager* gamepadManager,
       qml_components::ProfileList* profileList,
       resource_managers::Tables* tables,
       resource_managers::Languages* languages,
       sounds::AudioEngine* audioEngine,
       QObject* parent)
  : QObject(parent)
  , programSettings(programSettings)
  , inputTranslator(inputTranslator)
  , chartLoader(chartLoader)
  , rootSongFoldersConfig(rootSongFoldersConfig)
  , songFolderFactory(songFolderFactory)
  , previewFilePathFetcher(previewFilePathFetcher)
  , fileQuery(fileQuery)
  , themes(themes)
  , gamepadManager(gamepadManager)
  , profileList(profileList)
  , tables(tables)
  , languages(languages)
  , audioEngine(audioEngine)
{
}
Rg*
Rg::create(QQmlEngine* qmlEngine, QJSEngine*)
{
    QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}