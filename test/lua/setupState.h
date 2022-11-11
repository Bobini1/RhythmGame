#ifndef RHYTHMGAME_SETUPSTATE_H
#define RHYTHMGAME_SETUPSTATE_H

#include <SFML/Graphics/Texture.hpp>
#include <sol/state.hpp>
#include "lua/Bootstrapper.h"

inline sol::state
getStateWithAllDefinitions()
{
    sol::state state;
    state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    const lua::Bootstrapper bootstrapper;
    bootstrapper.defineTypes(state);
    return state;
}

#endif // RHYTHMGAME_SETUPSTATE_H