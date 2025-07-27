//
// Created by bobini on 20.09.23.
//

#include "QStringToPath.h"

namespace support {
auto
qStringToPath(const QString& qString) -> std::filesystem::path
{
#if defined(_WIN32)
    return std::filesystem::path(qString.toStdWString());
#else
    return qString.toStdString();
#endif
}
} // namespace support