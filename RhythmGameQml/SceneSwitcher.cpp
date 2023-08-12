//
// Created by bobini on 11.08.23.
//

#include "SceneSwitcher.h"
#include "ChartList.h"
#include <spdlog/spdlog.h>
#include <QJSEngine>
#include <QQmlEngine>
#include <qqmlcontext.h>

void
qml_components::SceneSwitcher::switchToMain()
{
    auto mainPath = SceneSwitcher::instance->qmlScriptFinder("Main");
    view->setView(mainPath.string().c_str());
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
    auto songWheelPath = SceneSwitcher::instance->qmlScriptFinder("SongWheel");
    auto* chartList = new ChartList{};
    auto* context = new QQmlContext{ qmlEngine(this) };
    view->setView(songWheelPath.string().c_str(), context);

    spdlog::info("Switched to SongWheel scene");
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
  ViewManager* view,
  std::function<std::filesystem::path(std::string)> qmlScriptFinder)
  : qmlScriptFinder(std::move(qmlScriptFinder))
  , view(view)
{
}
void
qml_components::SceneSwitcher::setInstance(
  qml_components::SceneSwitcher* newInstance)
{
    SceneSwitcher::instance = newInstance;
}
