//
// Created by PC on 08/10/2023.
//

#include "FileQuery.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include <spdlog/spdlog.h>
namespace {

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (!file.is_regular_file() && !file.is_directory()) {
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
auto
FileQuery::exists(const QString& path) -> bool
{
    const auto url = QUrl(path);
    return url.isLocalFile() && QFile::exists(url.toLocalFile()) &&
           !url.fileName().isEmpty();
}

auto
FileQuery::getSelectableFilesForDirectory(const QString& directory) const
  -> QList<QString>
{
    try {
        return ::getSelectableFilesForDirectory(
          support::qStringToPath(directory));
    } catch (const std::exception& e) {
        spdlog::error("Error getting selectable files for directory {}: {}",
                      directory.toStdString(),
                      e.what());
        return {};
    }
}
} // namespace qml_components