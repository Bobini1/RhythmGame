#include <co/co.h>
#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <boost/di.hpp>
#include "state_transitions/SceneStateMachineImpl.h"
#include "state_transitions/WindowStateMachineImpl.h"

class LuaLocatorImpl : public resource_locators::LuaScriptFinder
{
  public:
    auto findHandlerScript(const std::string& screen) -> std::string override
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
               "    a = 5 -- io.read(\"*number\")        -- read a number\n"
               "    print(fact(a))";
    }
};

auto
mainCo() -> int
{
    using namespace std::literals;
    auto injector = boost::di::make_injector(
      boost::di::bind<resource_locators::LuaScriptFinder>()
        .to<LuaLocatorImpl>(),
      boost::di::bind<sf::VideoMode>().to(sf::VideoMode{ 800, 600 }),
      boost::di::bind<std::string>().named("title"s).to("RhythmGame"),
      boost::di::bind<drawing::Scene>().to<drawing::SplashScene>(),
      boost::di::bind<drawing::Window>.to<drawing::SplashWindow>(),
      boost::di::bind<state_transitions::SceneStateMachine>.to<state_transitions::SceneStateMachineImpl>(),
      boost::di::bind<state_transitions::WindowStateMachine>.to<state_transitions::WindowStateMachineImpl>());
    auto wiringStart = std::chrono::high_resolution_clock::now();
    auto windowManager =
      injector.create<std::unique_ptr<state_transitions::WindowStateMachine>>();

    std::cout << (std::chrono::high_resolution_clock::now() - wiringStart);

    auto start = std::chrono::high_resolution_clock::now();
    while (windowManager->isOpen()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - start;
        start = now;
        windowManager->update(std::chrono::nanoseconds{ delta });
    }
}

auto
main() -> int
{
    co::WaitGroup wg;
    wg.add();
    int ret;
    go([&]() { ret = mainCo(); });
    wg.wait();

    return ret;
}
