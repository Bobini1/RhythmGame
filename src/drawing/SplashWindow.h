//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHWINDOW_H
#define RHYTHMGAME_SPLASHWINDOW_H

#include <SFML/Graphics/RenderWindow.hpp>
#include "drawing/Actor.h"
#include "resource_locators/LuaScriptFinder.h"
#include "Scene.h"
#include "Window.h"
namespace drawing {
class SplashWindow : public Window
{
    std::unique_ptr<Scene> splashScene;

  public:
    SplashWindow(std::unique_ptr<Scene> splashScene,
                 const sf::VideoMode& mode,
                 const std::string& title,
                 const sf::ContextSettings& settings = sf::ContextSettings());
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto draw() -> void override;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SPLASHWINDOW_H
