//
// Created by bobini on 20.08.22.
//

#ifndef RHYTHMGAME_INJECTOR_H
#define RHYTHMGAME_INJECTOR_H
#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <boost/di.hpp>
#include <future>
#include <thread>
#include "state_transitions/SceneStateMachineImpl.h"
#include "state_transitions/WindowStateMachineImpl.h"

constexpr auto title = []{};

namespace boost {
inline namespace ext {
namespace di {

template <>
struct [[maybe_unused]] ctor_traits<drawing::SplashWindow> {
    /*<<no intrusive way of defining named parameters>>*/
    BOOST_DI_INJECT_TRAITS(std::unique_ptr<drawing::Scene> splashScene,
                           const sf::VideoMode& mode,
                           (named = title) const std::string&,
                           const sf::ContextSettings& settings = sf::ContextSettings());
};

}  // namespace di
}  // namespace ext
}  // namespace boost

namespace wiring {
auto
getInjector() -> auto
{
    class LuaScriptFinderImpl : public resource_locators::LuaScriptFinder
    {
      public:
        auto findHandlerScript(const std::string&  /*screen*/) -> std::string override
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
    constexpr auto splashWidth = 800U;
    constexpr auto splashHeight = 600U;
    auto injector = boost::di::make_injector(
      boost::di::bind<resource_locators::LuaScriptFinder>()
        .to<LuaScriptFinderImpl>(),
      boost::di::bind<sf::VideoMode>().to(sf::VideoMode{ splashWidth, splashHeight }),
      boost::di::bind<std::string>().named(title).to("RhythmGame"),
      boost::di::bind<drawing::Scene>().to<drawing::SplashScene>(),
      boost::di::bind<drawing::Window>.to<drawing::SplashWindow>(),
      boost::di::bind<state_transitions::SceneStateMachine>.to<state_transitions::SceneStateMachineImpl>(),
      boost::di::bind<state_transitions::WindowStateMachine>.to<state_transitions::WindowStateMachineImpl>());
    return injector;
}
} // namespace wiring
#endif // RHYTHMGAME_INJECTOR_H
