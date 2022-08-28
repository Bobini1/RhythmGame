//
// Created by bobini on 31.07.2022.
//

#include "SplashScene.h"
#include "Actor.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <execution>
void
drawing::SplashScene::update(std::chrono::nanoseconds delta)
{
}
void
drawing::SplashScene::draw(sf::RenderTarget& target,
                           sf::RenderStates states) const
{
    auto actorsFromLua = std::array<drawing::Actor*, 0>{};
    std::for_each(
#ifdef RHYTHMGAME_HAS_STD_EXECUTION
      std::execution::par,
#endif
      std::begin(actorsFromLua),
      std::end(actorsFromLua),
      [&target, &states](auto&& actor) { target.draw(*actor, states); });
}
drawing::SplashScene::SplashScene(
  std::unique_ptr<resource_locators::LuaScriptFinder> luaScriptFinder)
{
    lua.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    lua.script(luaScriptFinder->findHandlerScript("splash"));
}
