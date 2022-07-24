//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_SONGDATAWRITER_H
#define RHYTHMGAME_SONGDATAWRITER_H
#include <string>
#include <any>
#include <sol/sol.hpp>

namespace charts::behaviour {

class SongDataWriter
{
    sol::state& lua;

  public:
    explicit SongDataWriter(sol::state& lua);
    template<typename T>
    auto writeVar(const std::string& key, T var) -> void
    {
        lua[key] = var;
    }
};

} // namespace charts::behaviour
#endif // RHYTHMGAME_SONGDATAWRITER_H
