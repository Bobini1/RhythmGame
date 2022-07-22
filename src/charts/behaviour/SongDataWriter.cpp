//
// Created by bobini on 22.07.2022.
//

#include "SongDataWriter.h"

namespace charts::behaviour {
SongDataWriter::SongDataWriter(sol::state_view lua)
  : lua(std::move(lua))
{
}
auto
SongDataWriter::writeVar(const std::string& key, auto var) -> void
{
    lua[key] = var;
}

} // namespace charts::behaviour