#ifndef RHYTHMGAME_SETUPSTATE_H
#define RHYTHMGAME_SETUPSTATE_H

#include <SFML/Graphics/Texture.hpp>
#include <sol/state.hpp>
#include "lua/Bootstrapper.h"
#include "resource_managers/TextureLoaderImpl.h"
#include "resource_managers/FontLoaderImpl.h"

inline auto
getStateWithAllDefinitions() -> sol::state
{
    sol::state state;
    state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    const lua::Bootstrapper bootstrapper;
    bootstrapper.defineCommonTypes(state);

    auto textureLoader = resource_managers::TextureLoaderImpl{};
    bootstrapper.bindTextureLoader(state, textureLoader);

    auto fontLoader = resource_managers::FontLoaderImpl{};
    bootstrapper.bindFontLoader(state, fontLoader);

    return state;
}

#endif // RHYTHMGAME_SETUPSTATE_H