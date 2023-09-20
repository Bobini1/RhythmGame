//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_PATHTOUTFSTRING_H
#define RHYTHMGAME_PATHTOUTFSTRING_H

#include <filesystem>
#include <string>
namespace support {

auto
pathToUtfString(const std::filesystem::path& path) -> std::string;

} // namespace support

#endif // RHYTHMGAME_PATHTOUTFSTRING_H
