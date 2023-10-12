//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_QSTRINGTOPATH_H
#define RHYTHMGAME_QSTRINGTOPATH_H

#include <filesystem>
#include <QString>
namespace support {
auto
qStringToPath(const QString& qString) -> std::filesystem::path;
} // namespace support

#endif // RHYTHMGAME_QSTRINGTOPATH_H
