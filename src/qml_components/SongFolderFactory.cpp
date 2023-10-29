//
// Created by bobini on 20.09.23.
//

#include "SongFolderFactory.h"
#include "gameplay_logic/ChartData.h"

namespace qml_components {
QVariantList
SongFolderFactory::open(QString path)
{
    auto folder = QVariantList{};
    auto pathStd = path.toStdString();
    getFolders.reset();
    getFolders.bind(1, pathStd);
    auto result = getFolders.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        folder.append(QString::fromStdString(row));
    }
    getCharts.reset();
    getCharts.bind(1, pathStd);
    auto chartResult =
      getCharts.executeAndGetAll<gameplay_logic::ChartData::DTO>();
    for (const auto& row : chartResult) {
        auto loadedChart = gameplay_logic::ChartData::load(row);
        QQmlEngine::setObjectOwnership(loadedChart.get(),
                                       QQmlEngine::JavaScriptOwnership);
        folder.append(QVariant::fromValue(loadedChart.release()));
    }
    return folder;
}
SongFolderFactory::SongFolderFactory(db::SqliteCppDb* db, QObject* parent)
  : QObject(parent)
  , db(db)
{
}
auto
SongFolderFactory::folderSize(QString path) -> int
{
    auto pathStd = path.toStdString();
    getSize.reset();
    getSize.bind(1, pathStd);
    getSize.bind(2, pathStd);
    auto result = getSize.executeAndGetAll<int>();
    return result[0] + result[1];
}
QString
SongFolderFactory::parentFolder(QString path)
{
    if (path == QStringLiteral("/")) {
        return QStringLiteral("");
    }
    auto parent = path;
    parent.resize(parent.size() - 1);
    auto lastSlashIndex = parent.lastIndexOf("/");
    parent.remove(lastSlashIndex + 1, parent.size() - lastSlashIndex - 1);
    return parent;
}
QVariantList
SongFolderFactory::search(QString query)
{
    auto folder = QVariantList{};
    auto queryStd = query.toStdString();
    auto queryLike = "%" + queryStd + "%";
    searchFolders.reset();
    searchFolders.bind(1, queryLike);
    auto result = searchFolders.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        folder.append(QString::fromStdString(row));
    }
    searchCharts.reset();
    searchCharts.bind(":query", queryLike);
    auto chartResult =
      searchCharts.executeAndGetAll<gameplay_logic::ChartData::DTO>();
    for (const auto& row : chartResult) {
        auto loadedChart = gameplay_logic::ChartData::load(row);
        QQmlEngine::setObjectOwnership(loadedChart.get(),
                                       QQmlEngine::JavaScriptOwnership);
        folder.append(QVariant::fromValue(loadedChart.release()));
    }
    return folder;
}
} // namespace qml_components