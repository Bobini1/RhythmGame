//
// Created by bobini on 11.08.23.
//

#include "SceneUrls.h"
#include <spdlog/spdlog.h>
#include <QJSEngine>
#include <utility>
#include <qqmlcontext.h>

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
    if (newThemeConfig.resultScene != themeConfig.resultScene) {
        spdlog::info("Result scene changed from {} to {}",
                     themeConfig.resultScene.toStdString(),
                     newThemeConfig.resultScene.toStdString());
        emit resultSceneUrlChanged();
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
auto
qml_components::SceneUrls::resultSceneUrl() const -> QUrl
{
    return themeConfig.resultScene;
}