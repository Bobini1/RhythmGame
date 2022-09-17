//
// Created by bobini on 22.07.2022.
//

#include "SongDataWriterToLua.h"

namespace charts::behaviour {
SongDataWriterToLua::SongDataWriterToLua(sol::state& lua)
  : lua(lua)
{
}

} // namespace charts::behaviour