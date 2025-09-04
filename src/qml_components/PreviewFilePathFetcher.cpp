//
// Created by bobini on 05.10.23.
//

#include "PreviewFilePathFetcher.h"

#include <spdlog/stopwatch.h>
#include <spdlog/spdlog.h>

namespace qml_components {
PreviewFilePathFetcher::PreviewFilePathFetcher(db::SqliteCppDb* db,
                                               QObject* parent)
  : QObject(parent)
  , db(db)
{
}
auto
PreviewFilePathFetcher::getPreviewFilePaths(QList<QString> directories) const
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

        auto statement = db->createStatement(
            "SELECT directory, path FROM preview_files WHERE directory IN (" +
            placeholders.toStdString() + ")");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        auto queryResults = statement.executeAndGetAll<std::tuple<std::string, std::string>>();

        for (const auto& row : queryResults) {
            auto directory = QString::fromStdString(std::get<0>(row));
            auto path = QString::fromStdString(std::get<1>(row));
            result[directory] = path;
        }
    }

    spdlog::debug("Fetched {} preview file paths in {}s", result.size(), sw);

    return result;
}
} // namespace qml_components