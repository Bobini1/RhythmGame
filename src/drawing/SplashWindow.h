//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHWINDOW_H
#define RHYTHMGAME_SPLASHWINDOW_H

#include <SFML/Graphics/RenderWindow.hpp>
#include "drawing/actors/Actor.h"
#include "resource_managers/LuaScriptFinder.h"
#include "Scene.h"
#include "Window.h"
namespace drawing {
/**
 * @brief Window that displays the splash scene while the game is being loaded.
 */
class SplashWindow : public Window
{
    std::shared_ptr<drawing::Scene> splashScene;

  public:
    SplashWindow(std::shared_ptr<drawing::Scene> splashScene,
                 const sf::VideoMode& mode,
                 const std::string& title,
                 const sf::ContextSettings& settings = sf::ContextSettings());
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto draw() -> void override;
};
} // namespace drawing
#endif // RHYTHMGAME_SPLASHWINDOW_H