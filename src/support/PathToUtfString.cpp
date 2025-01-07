//
// Created by bobini on 20.09.23.
//

#include "PathToUtfString.h"
#include <QString>
#include <ranges>

namespace support {
auto
pathToUtfString(const std::filesystem::path& path) -> std::string
{
#if defined(_WIN32)
    auto string = QString::fromStdWString(path.wstring()).toStdString();
    std::ranges::replace(string, '\\', '/');
    return string;
#else
    return path.string();
#endif
}
} // namespace support