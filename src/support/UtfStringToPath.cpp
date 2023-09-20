//
// Created by bobini on 20.09.23.
//

#include "UtfStringToPath.h"
auto
support::utfStringToPath(const std::string& utfString) -> std::filesystem::path
{
#if defined(_WIN32)
    return QString::fromStdString(utfString).toStdWString();
#else
    return utfString;
#endif
}
