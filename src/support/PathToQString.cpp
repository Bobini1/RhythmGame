//
// Created by bobini on 20.09.23.
//

#include "PathToQString.h"

namespace support {
auto
pathToQString(const std::filesystem::path& path) -> QString
{
#if defined(_WIN32)
    auto string = QString::fromStdWString(path.wstring());
    std::ranges::replace(string, '\\', '/');
    return string;
#else
    return QString::fromStdString(path.string());
#endif
}
} // namespace support