//
// Created by PC on 08/10/2023.
//

#include "FileQuery.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"
namespace {

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (!file.is_regular_file()) {
            continue;
        }
        if (file.path().extension() == ".ini") {
            continue;
        }
        // ignore files starting with dot
        if (file.path().filename().string().front() == '.') {
            continue;
        }
        files.push_back(support::pathToQString(file.path().filename()));
    }
    return files;
}
} // namespace

namespace qml_components {
bool
FileQuery::exists(const QString& path)
{
    const auto url = QUrl(path);
    return url.isLocalFile() && QFile::exists(url.toLocalFile()) &&
           !url.fileName().isEmpty();
}

QList<QString>
FileQuery::getSelectableFilesForDirectory(const QString& directory) const
{
    return ::getSelectableFilesForDirectory(support::qStringToPath(directory));
}
} // namespace qml_components