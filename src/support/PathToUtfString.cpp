//
// Created by bobini on 20.09.23.
//

#include "PathToUtfString.h"
#include <QString>

namespace support {
auto
pathToUtfString(const std::filesystem::path& path) -> std::string
{
#if defined(_WIN32)
    return QString::fromStdWString(path.wstring()).toStdString();
#else
    return path.string();
#endif
}
} // namespace support