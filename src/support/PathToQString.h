//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_PATHTOQSTRING_H
#define RHYTHMGAME_PATHTOQSTRING_H

#include <QString>
#include <filesystem>
namespace support {
auto
pathToQString(const std::filesystem::path& path) -> QString;
} // namespace support

#endif // RHYTHMGAME_PATHTOQSTRING_H
