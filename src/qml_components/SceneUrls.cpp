//
// Created by bobini on 11.08.23.
//

#include "SceneUrls.h"
#include <spdlog/spdlog.h>
#include <QJSEngine>
#include <utility>
#include <qqmlcontext.h>

auto
qml_components::SceneUrls::mainScene() const -> QUrl
{
    return QUrl::fromLocalFile(themeConfig.mainScene);
}
auto
qml_components::SceneUrls::gameplayScene() const -> QUrl
{
    return QUrl::fromLocalFile(themeConfig.gameplayScene);
}
auto
qml_components::SceneUrls::songWheelScene() const -> QUrl
{
    return QUrl::fromLocalFile(themeConfig.songWheelScene);
}
void
qml_components::SceneUrls::refreshThemeConfig()
{
    auto newThemeConfig = themeConfigFactory();
    if (newThemeConfig.gameplayScene != themeConfig.gameplayScene) {
        spdlog::info("Gameplay scene changed from {} to {}",
                     themeConfig.gameplayScene.toStdString(),
                     newThemeConfig.gameplayScene.toStdString());
        emit gameplaySceneChanged();
    }
    if (newThemeConfig.mainScene != themeConfig.mainScene) {
        spdlog::info("Main scene changed from {} to {}",
                     themeConfig.mainScene.toStdString(),
                     newThemeConfig.mainScene.toStdString());
        emit mainSceneChanged();
    }
    if (newThemeConfig.songWheelScene != themeConfig.songWheelScene) {
        spdlog::info("Song wheel scene changed from {} to {}",
                     themeConfig.songWheelScene.toStdString(),
                     newThemeConfig.songWheelScene.toStdString());
        emit songWheelSceneChanged();
    }
    if (newThemeConfig.SettingScene != themeConfig.SettingScene) {
        spdlog::info("Settings scene changed from {} to {}",
                     themeConfig.SettingScene.toStdString(),
                     newThemeConfig.SettingScene.toStdString());
        emit settingsSceneChanged();
    }
    if (newThemeConfig.resultScene != themeConfig.resultScene) {
        spdlog::info("Result scene changed from {} to {}",
                     themeConfig.resultScene.toStdString(),
                     newThemeConfig.resultScene.toStdString());
        emit resultSceneChanged();
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
qml_components::SceneUrls::settingsScene() const -> QUrl
{
    return QUrl::fromLocalFile(themeConfig.SettingScene);
}
auto
qml_components::SceneUrls::resultScene() const -> QUrl
{
    return QUrl::fromLocalFile(themeConfig.resultScene);
}
