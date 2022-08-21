//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHSCENE_H
#define RHYTHMGAME_SPLASHSCENE_H
#include "drawing/Scene.h"
#include "resource_locators/LuaScriptFinder.h"
namespace drawing {
class SplashScene : public Scene
{
    sol::state lua;

  public:
    explicit SplashScene(
      std::unique_ptr<resource_locators::LuaScriptFinder> luaScriptFinder);
    auto update(std::chrono::nanoseconds delta) -> void override;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};
} // namespace state_transitions

#endif // RHYTHMGAME_SPLASHSCENE_H
