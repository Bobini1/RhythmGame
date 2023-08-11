//
// Created by bobini on 11.08.23.
//

#include "SceneSwitcher.h"
#include <spdlog/spdlog.h>
#include <QJSEngine>

void
qml_components::SceneSwitcher::switchToMain()
{
    spdlog::info("Switched to Main scene");
}
void
qml_components::SceneSwitcher::switchToGameplay()
{
    spdlog::info("Switched to Gameplay scene");
}
void
qml_components::SceneSwitcher::switchToSongWheel()
{
    spdlog::info("Switched to Song Wheel scene");
}
auto
qml_components::SceneSwitcher::create(QQmlEngine* /*engine*/, QJSEngine* engine)
  -> qml_components::SceneSwitcher*
{
    Q_ASSERT(instance);
    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(engine->thread() == instance->thread());
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}
qml_components::SceneSwitcher::SceneSwitcher(
  std::function<std::filesystem::path(std::string)> qmlScriptFinder)
  : qmlScriptFinder(std::move(qmlScriptFinder))
{
}
void
qml_components::SceneSwitcher::setInstance(
  qml_components::SceneSwitcher* newInstance)
{
    SceneSwitcher::instance = newInstance;
}
