#ifndef RHYTHMGAME_SETUPSTATE_H
#define RHYTHMGAME_SETUPSTATE_H

#include "../../.conan/data/sfml/2.5.1/_/_/package/8c725b9a48d15986d670001801d4f5f3c09d097d/include/SFML/Graphics/Texture.hpp"
#include "../../.conan/data/sol2/3.3.0/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/include/sol/state.hpp"
#include "../src/lua/Bootstrapper.h"
#include "../src/resource_managers/TextureLoaderImpl.h"
#include "../src/resource_managers/FontLoaderImpl.h"

inline auto
getStateWithAllDefinitions() -> sol::state
{
    sol::state state;
    state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    lua::Bootstrapper bootstrapper;
    bootstrapper.defineCommonTypes(state);

    auto textureLoader = resource_managers::TextureLoaderImpl{};
    bootstrapper.bindTextureLoader(state, textureLoader);

    auto fontLoader = resource_managers::FontLoaderImpl{};
    bootstrapper.bindFontLoader(state, fontLoader);

    return state;
}

class StateSetup
{
    sol::state state;
    lua::Bootstrapper bootstrapper;

  public:
    StateSetup()
    {
        state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    }

    template<typename... Args>
    auto addEventToState(auto& event, std::string name) -> void
    {
        bootstrapper.addEvent<decltype(event), Args...>(
          state, event, std::move(name));
    }

    explicit operator sol::state() &&
    {
        bootstrapper.defineCommonTypes(state);

        auto textureLoader = resource_managers::TextureLoaderImpl{};
        bootstrapper.bindTextureLoader(state, textureLoader);

        auto fontLoader = resource_managers::FontLoaderImpl{};
        bootstrapper.bindFontLoader(state, fontLoader);

        return std::move(state);
    }
};

#endif // RHYTHMGAME_SETUPSTATE_H