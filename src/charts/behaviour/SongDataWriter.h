//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_SONGDATAWRITER_H
#define RHYTHMGAME_SONGDATAWRITER_H
#include <string>
#include <any>
#include <sol/sol.hpp>

namespace charts::behaviour {

/**
 * @brief Writes song data to Lua state.
 */
class SongDataWriter
{
    sol::state& lua;

  public:
    /**
     * @brief Constructs SongDataWriter with the lua state to write to.
     * @param lua Lua state.
     */
    explicit SongDataWriter(sol::state& lua);

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

} // namespace charts::behaviour
#endif // RHYTHMGAME_SONGDATAWRITER_H
