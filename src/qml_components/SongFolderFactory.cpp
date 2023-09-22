//
// Created by bobini on 20.09.23.
//

#include "SongFolderFactory.h"

namespace qml_components {
Folder*
SongFolderFactory::open(QString path)
{
    auto children = QList<QString>{};
    auto pathStd = path.toStdString();
    getFolders.reset();
    getFolders.bind(1, pathStd);
    auto result = getFolders.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        children.append(QString::fromStdString(row));
    }
    getCharts.reset();
    getCharts.bind(1, pathStd);
    auto chartResult =
      getCharts.executeAndGetAll<gameplay_logic::ChartData::DTO>();
    return new Folder{ path, std::move(children), std::move(chartResult), db };
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
} // namespace qml_components