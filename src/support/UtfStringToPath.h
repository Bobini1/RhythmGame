//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_UTFSTRINGTOPATH_H
#define RHYTHMGAME_UTFSTRINGTOPATH_H

#include <filesystem>
namespace support {
auto
utfStringToPath(const std::string& utfString) -> std::filesystem::path;
} // namespace support

#endif // RHYTHMGAME_UTFSTRINGTOPATH_H
