//
// Created by bobini on 20.09.23.
//

#include "SongFolderFactory.h"
#include "gameplay_logic/ChartData.h"
#include "spdlog/stopwatch.h"
#include "spdlog/spdlog.h"

namespace qml_components {
QVariantList
SongFolderFactory::open(QString path)
{
    auto folder = QVariantList{};
    auto pathStd = path.toStdString();
    getFolders.reset();
    if (path.isEmpty()) {
        getFolders.bind(1);
    } else {
        getFolders.bind(1, pathStd);
    }
    auto result = getFolders.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        folder.append(QString::fromStdString(row));
    }
    getCharts.reset();
    if (path.isEmpty()) {
        getCharts.bind(1);
    } else {
        getCharts.bind(1, pathStd);
    }
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

    auto pathStd = path.toStdString();
    getParentFolder.reset();
    getParentFolder.bind(1, pathStd);
    auto result = getParentFolder.executeAndGet<std::string>();
    if (!result.has_value() || result.value().empty()) {
        return "";
    }
    return QString::fromStdString(result.value());
}
QVariantList
SongFolderFactory::search(QString query)
{
    auto sw = spdlog::stopwatch{};
    auto folder = QVariantList{};
    auto queryStd = query.toStdString();
    try {
        searchCharts.reset();
        searchCharts.bind(1, queryStd);
        auto chartResult =
          searchCharts.executeAndGetAll<gameplay_logic::ChartData::DTO>();
        for (const auto& row : chartResult) {
            auto loadedChart = gameplay_logic::ChartData::load(row);
            QQmlEngine::setObjectOwnership(loadedChart.get(),
                                           QQmlEngine::JavaScriptOwnership);
            folder.append(QVariant::fromValue(loadedChart.release()));
        }
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
    }
    spdlog::info("search took {} seconds", sw);
    return folder;
}
} // namespace qml_components