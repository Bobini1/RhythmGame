//
// Created by bobini on 11.08.23.
//

#include "SceneUrls.h"
#include "ChartList.h"
#include <spdlog/spdlog.h>
#include <QJSEngine>
#include <QQmlEngine>
#include <utility>
#include <qqmlcontext.h>

auto
qml_components::SceneUrls::create(QQmlEngine* /*engine*/, QJSEngine* engine)
  -> qml_components::SceneUrls*
{
    Q_ASSERT(instance);
    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(engine->thread() == instance->thread());
    return instance;
}
void
qml_components::SceneUrls::setInstance(qml_components::SceneUrls* newInstance)
{
    QJSEngine::setObjectOwnership(newInstance, QQmlEngine::CppOwnership);
    SceneUrls::instance = newInstance;
}
auto
qml_components::SceneUrls::mainSceneUrl() const -> QUrl
{
    return themeConfig.mainScene;
}
auto
qml_components::SceneUrls::gameplaySceneUrl() const -> QUrl
{
    return themeConfig.gameplayScene;
}
auto
qml_components::SceneUrls::songWheelSceneUrl() const -> QUrl
{
    return themeConfig.songWheelScene;
}
void
qml_components::SceneUrls::refreshThemeConfig()
{
    auto newThemeConfig = themeConfigFactory();
    if (newThemeConfig.gameplayScene != themeConfig.gameplayScene) {
        spdlog::info("Gameplay scene changed from {} to {}",
                     themeConfig.gameplayScene.toStdString(),
                     newThemeConfig.gameplayScene.toStdString());
        emit gameplaySceneUrlChanged();
    }
    if (newThemeConfig.mainScene != themeConfig.mainScene) {
        spdlog::info("Main scene changed from {} to {}",
                     themeConfig.mainScene.toStdString(),
                     newThemeConfig.mainScene.toStdString());
        emit mainSceneUrlChanged();
    }
    if (newThemeConfig.songWheelScene != themeConfig.songWheelScene) {
        spdlog::info("Song wheel scene changed from {} to {}",
                     themeConfig.songWheelScene.toStdString(),
                     newThemeConfig.songWheelScene.toStdString());
        emit songWheelSceneUrlChanged();
    }
    if (newThemeConfig.SettingScene != themeConfig.SettingScene) {
        spdlog::info("Settings scene changed from {} to {}",
                     themeConfig.SettingScene.toStdString(),
                     newThemeConfig.SettingScene.toStdString());
        emit settingsSceneUrlChanged();
    }
    themeConfig = std::move(newThemeConfig);
    spdlog::info("Refreshed theme config");
}
qml_components::SceneUrls::SceneUrls(
  std::function<resource_managers::models::ThemeConfig()> themeConfigFactory)
  : themeConfigFactory{ std::move(themeConfigFactory) }
  , themeConfig{ this->themeConfigFactory() }
{
}
auto
qml_components::SceneUrls::settingsSceneUrl() const -> QUrl
{
    return themeConfig.SettingScene;
}
