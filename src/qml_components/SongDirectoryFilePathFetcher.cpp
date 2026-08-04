//
// Created by bobini on 05.10.23.
//

#include "SongDirectoryFilePathFetcher.h"

#include "resource_managers/SongAssetStore.h"
#include "support/QStringToPath.h"
#include <spdlog/stopwatch.h>
#include <spdlog/spdlog.h>
#include <utility>

namespace qml_components {
SongDirectoryFilePathFetcher::SongDirectoryFilePathFetcher(
  db::SqliteCppDb* db,
  resource_managers::SongAssetStore* assetStore,
  QObject* parent)
  : QObject(parent)
  , db(db)
  , assetStore(assetStore)
{
}
auto
SongDirectoryFilePathFetcher::getFilePaths(QList<QString> directories,
                                           const std::string& table) const
  -> QVariantHash
{
    QVariantHash result;

    auto sw = spdlog::stopwatch{};

    if (directories.isEmpty()) {
        return result;
    }

    auto uniqueDirs = QSet<QString>{};
    for (const auto& dir : directories) {
        uniqueDirs.insert(dir);
    }
    directories = uniqueDirs.values();

    constexpr int maxVariables = 999;

    for (int i = 0; i < directories.size(); i += maxVariables) {
        auto chunk = directories.mid(i, maxVariables);
        auto placeholders = QString("?, ").repeated(chunk.size()).chopped(2);

        auto statement = db->createStatement("SELECT directory, path FROM " +
                                             table + " WHERE directory IN (" +
                                             placeholders.toStdString() + ")");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        auto queryResults =
          statement.executeAndGetAll<std::tuple<std::string, std::string>>();

        for (const auto& row : queryResults) {
            auto directory = QString::fromStdString(std::get<0>(row));
            auto path = QString::fromStdString(std::get<1>(row));
            result[directory] = path;
        }
    }

    spdlog::debug("Fetched {} {} file paths in {}s", result.size(), table, sw);

    return result;
}

auto
SongDirectoryFilePathFetcher::getPreviewFilePaths(
  QList<QString> directories) const -> QVariantHash
{
    auto result = getFilePaths(std::move(directories), "preview_files");
    for (auto iterator = result.begin(); iterator != result.end(); ++iterator) {
        const auto path = iterator.value().toString();
        const auto fsPath = support::qStringToPath(path);
        if (assetStore->isArchived(fsPath)) {
            iterator.value() =
              resource_managers::SongAssetStore::audioUrl(fsPath);
        }
    }
    return result;
}

auto
SongDirectoryFilePathFetcher::getReadmeFilePaths(
  QList<QString> directories) const -> QVariantHash
{
    return getFilePaths(std::move(directories), "readme_files");
}
} // namespace qml_components
