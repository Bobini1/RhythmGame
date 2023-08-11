//
// Created by bobini on 11.08.23.
//

#include "SceneSwitcher.h"
#include <spdlog/spdlog.h>

namespace QmlComponents {

} // namespace qml_components
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
qml_components::SceneSwitcher::SceneSwitcher(
  std::function<std::filesystem::path(std::string)> qmlScriptFinder)
  : qmlScriptFinder(std::move(qmlScriptFinder))
{
}
