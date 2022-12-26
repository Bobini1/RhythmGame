//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_SONGDATAWRITERTOLUA_H
#define RHYTHMGAME_SONGDATAWRITERTOLUA_H
#include <string>
#include "lua/Lua.h"
#include <charts/behaviour/SongDataWriter.h>

namespace charts::behaviour {

/**
 * @brief Writes song data to Lua state.
 */
class SongDataWriterToLua
{
    sol::state& lua;

  public:
    /**
     * @brief Constructs SongDataWriterToLua with the lua state to write to.
     * @param lua Lua state.
     */
    explicit SongDataWriterToLua(sol::state& lua);

    /**
     * @brief Writes variable to Lua state.
     * @param key Name of the variable.
     * @param var Value of the variable.
     */
    template<typename T>
    auto writeVar(const std::string& key, T var) -> void
    {
        lua[key] = var;
    }
};

static_assert(SongDataWriter<SongDataWriterToLua, int>);
} // namespace charts::behaviour
#endif // RHYTHMGAME_SONGDATAWRITERTOLUA_H
