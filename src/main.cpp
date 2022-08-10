#include <co/co.h>
#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <boost/di.hpp>
#include <future>
#include <thread>
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
               "    a = 5\n"
               "    print(fact(a))";
    }
};

using namespace std::literals;

namespace boost {
inline namespace ext {
namespace di {

template <>
struct ctor_traits<drawing::SplashWindow> {
    /*<<no intrusive way of defining named parameters>>*/
    BOOST_DI_INJECT_TRAITS(std::unique_ptr<drawing::Scene> splashScene,
                           const sf::VideoMode& mode,
                           (named = "title"s) const std::string&,
                           const sf::ContextSettings& settings = sf::ContextSettings());
};

}  // namespace di
}  // namespace ext
}  // namespace boost

auto
mainCo() -> int
{
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

    auto wiringDuration =
      std::chrono::high_resolution_clock::now() - wiringStart;

    std::cout << wiringDuration;

    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<bool> finished;
    auto eventManagement = std::jthread{ [&windowManager, &finished] {
        while (!finished) {
            windowManager->pollEvents();
        }
    } };
    while (windowManager->isOpen()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - start;
        start = now;
        windowManager->update(std::chrono::nanoseconds{ delta });
        windowManager->draw();
    }
}

auto
main() -> int
{
    co::WaitGroup waitGroup;
    waitGroup.add();
    int ret{};
    go([&]() { ret = mainCo(); });
    waitGroup.wait();

    return ret;
}
