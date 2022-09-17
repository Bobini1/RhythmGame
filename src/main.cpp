#include "wiring/Injector.h"
#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <boost/di.hpp>
#include <future>
#include <thread>
#include "state_transitions/SceneStateMachineImpl.h"
#include "state_transitions/WindowStateMachineImpl.h"

#include "state_transitions/Game.h"

class LuaScriptFinderImpl
{
  public:
    auto findHandlerScript(const std::string& /*screen*/) -> std::string
    {
        return "    -- defines a factorial function\n"
               "    function fact (n)\n"
               "      if n == 0 then\n"
               "        return 1\n"
               "      else\n"
               "        return n * fact(n-1)\n"
               "      end\n"
               "    end\n"
               "    \n"
               "    print(\"enter a number:\")\n"
               "    a = 5\n"
               "    print(fact(a))";
    }
};

auto
main() -> int
{
    auto luaScriptFinder = LuaScriptFinderImpl{};
    auto startingScene =
      std::make_shared<drawing::SplashScene<decltype(luaScriptFinder)>>(
        luaScriptFinder);
    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
