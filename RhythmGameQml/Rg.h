//
// Created by PC on 08/05/2025.
//

#ifndef RG_H
#define RG_H
#include "qml_components/ChartLoader.h"
#include "qml_components/FileQuery.h"
#include "qml_components/PreviewFilePathFetcher.h"
#include "qml_components/ProgramSettings.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/SongFolderFactory.h"
#include "qml_components/Themes.h"
#include "input/InputTranslator.h"
#include "sounds/AudioEngine.h"
#include "resource_managers/Languages.h"
#include "resource_managers/Tables.h"

#include <QObject>
#include <qqmlintegration.h>

/**
 * @brief The main singleton class that provides access to various components of
 * the application.
 * @details This class is exposed to QML as a singleton. Access it by importing
 * RhythmGameQml 1.0 and using the Rg type.
 */
class Rg final : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(qml_components::ProgramSettings* programSettings MEMBER
                 programSettings CONSTANT)
    Q_PROPERTY(
      input::InputTranslator* inputTranslator MEMBER inputTranslator CONSTANT)
    Q_PROPERTY(
      qml_components::ChartLoader* chartLoader MEMBER chartLoader CONSTANT)
    Q_PROPERTY(qml_components::RootSongFoldersConfig* rootSongFoldersConfig
                 MEMBER rootSongFoldersConfig CONSTANT)
    Q_PROPERTY(qml_components::SongFolderFactory* songFolderFactory MEMBER
                 songFolderFactory CONSTANT)
    Q_PROPERTY(qml_components::PreviewFilePathFetcher* previewFilePathFetcher
                 MEMBER previewFilePathFetcher CONSTANT)
    Q_PROPERTY(qml_components::FileQuery* fileQuery MEMBER fileQuery CONSTANT)
    Q_PROPERTY(qml_components::Themes* themes MEMBER themes CONSTANT)
    Q_PROPERTY(
      input::GamepadManager* gamepadManager MEMBER gamepadManager CONSTANT)
    Q_PROPERTY(
      qml_components::ProfileList* profileList MEMBER profileList CONSTANT)
    Q_PROPERTY(resource_managers::Tables* tables MEMBER tables CONSTANT)
    Q_PROPERTY(
      resource_managers::Languages* languages MEMBER languages CONSTANT)
    Q_PROPERTY(sounds::AudioEngine* audioEngine MEMBER audioEngine CONSTANT)

    qml_components::ProgramSettings* programSettings;
    input::InputTranslator* inputTranslator;
    qml_components::ChartLoader* chartLoader;
    qml_components::RootSongFoldersConfig* rootSongFoldersConfig;
    qml_components::SongFolderFactory* songFolderFactory;
    qml_components::PreviewFilePathFetcher* previewFilePathFetcher;
    qml_components::FileQuery* fileQuery;
    qml_components::Themes* themes;
    input::GamepadManager* gamepadManager;
    qml_components::ProfileList* profileList;
    resource_managers::Tables* tables;
    resource_managers::Languages* languages;
    sounds::AudioEngine* audioEngine;

  public:
    Rg(qml_components::ProgramSettings* programSettings,
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
       QObject* parent = nullptr);
    inline static Rg* instance = nullptr;
    static Rg* create(QQmlEngine* qmlEngine, QJSEngine*);
};

#endif // RG_H
