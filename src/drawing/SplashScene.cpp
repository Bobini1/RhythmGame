//
// Created by bobini on 31.07.2022.
//

#include "SplashScene.h"
void
drawing::SplashScene::update(std::chrono::nanoseconds delta)
{
}
void
drawing::SplashScene::draw(sf::RenderTarget& target,
                           sf::RenderStates states) const
{
}
drawing::SplashScene::SplashScene(
  std::unique_ptr<resource_locators::LuaScriptFinder> luaScriptFinder)
{
    lua.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    lua.script(luaScriptFinder->findHandlerScript("splash"));
}
