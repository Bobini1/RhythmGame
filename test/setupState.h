#ifndef RHYTHMGAME_SETUPSTATE_H
#define RHYTHMGAME_SETUPSTATE_H

#include <SFML/Graphics/Texture.hpp>
#include <sol/state.hpp>
#include "lua/Bootstrapper.h"
#include "resource_managers/TextureLoaderImpl.h"
#include "resource_managers/FontLoaderImpl.h"
#include "drawing/animations/AnimationPlayerImpl.h"

class StateSetup
{
    sol::state state;
    lua::EventAttacher eventAttacher;
    resource_managers::TextureLoaderImpl textureLoader;
    resource_managers::FontLoaderImpl fontLoader = {};
    drawing::animations::AnimationPlayerImpl animationPlayer = {};

  public:
    StateSetup()
      : eventAttacher(&state)
    {
        state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    }

    template<typename... Args>
    auto addEventToState(auto& event, std::string name) -> void
    {
        eventAttacher.addEvent<decltype(event), Args...>(
          event, std::move(name));
    }

    drawing::animations::AnimationPlayerImpl& getAnimationPlayer()
    {
        return animationPlayer;
    }

    sol::state& getState()
    {
        return state;
    }

    void defineTypes()
    {
        lua::defineAllTypes(
          state, textureLoader, fontLoader, animationPlayer, eventAttacher);

    }

    explicit operator sol::state() &&
    {
        defineTypes();
        return std::move(state);
    }
};

#endif // RHYTHMGAME_SETUPSTATE_H