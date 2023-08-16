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
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}
void
qml_components::SceneUrls::setInstance(qml_components::SceneUrls* newInstance)
{
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
                     themeConfig.gameplayScene.toString().toStdString(),
                     newThemeConfig.gameplayScene.toString().toStdString());
        emit gameplaySceneUrlChanged();
    }
    if (newThemeConfig.mainScene != themeConfig.mainScene) {
        spdlog::info("Main scene changed from {} to {}",
                     themeConfig.mainScene.toString().toStdString(),
                     newThemeConfig.mainScene.toString().toStdString());
        emit mainSceneUrlChanged();
    }
    if (newThemeConfig.songWheelScene != themeConfig.songWheelScene) {
        spdlog::info("Song wheel scene changed from {} to {}",
                     themeConfig.songWheelScene.toString().toStdString(),
                     newThemeConfig.songWheelScene.toString().toStdString());
        emit songWheelSceneUrlChanged();
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
