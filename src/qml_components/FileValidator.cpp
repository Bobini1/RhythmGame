//
// Created by PC on 08/10/2023.
//

#include "FileValidator.h"
namespace qml_components {
bool
FileValidator::exists(const QString& path)
{
    auto url = QUrl(path);
    return url.isLocalFile() && QFile::exists(url.toLocalFile());
}
} // namespace qml_components