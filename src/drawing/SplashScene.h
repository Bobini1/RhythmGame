//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SPLASHSCENE_H
#define RHYTHMGAME_SPLASHSCENE_H
#include "drawing/Scene.h"
#include "resource_locators/LuaScriptFinder.h"
#include "Actor.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <execution>
namespace drawing {
/**
 * @brief Scene that displays the splash screen while the game is being loaded.
 */
template<resource_locators::LuaScriptFinder LuaScriptFinderType>
class SplashScene : public Scene
{
    sol::state lua;

  public:
    explicit SplashScene(LuaScriptFinderType luaScriptFinder)
    {
        lua.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
        lua.script(luaScriptFinder.findHandlerScript("splash"));
    }
    void update(std::chrono::nanoseconds /* delta */) final {}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const final
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
};
} // namespace drawing

#endif // RHYTHMGAME_SPLASHSCENE_H
