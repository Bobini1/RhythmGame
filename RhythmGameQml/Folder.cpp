//
// Created by bobini on 19.09.23.
//

#include <QtConcurrentMap>
#include "Folder.h"

namespace qml_components {
Folder::Folder(QString path,
               QList<QString> childrenDirs,
               std::vector<gameplay_logic::ChartData::DTO> childrenCharts,
               db::SqliteCppDb* db,
               QObject* parent)
  : QAbstractListModel(parent)
  , path(std::move(path))
  , db(db)
  , childrenFolders(std::move(childrenDirs))
  , chartData(QtConcurrent::blockingMapped(
      childrenCharts,
      [](const gameplay_logic::ChartData::DTO& dto) {
          return gameplay_logic::ChartData::load(dto).release();
      }))
{
    for (auto& chart : chartData) {
        chart->moveToThread(QThread::currentThread());
        chart->setParent(this);
    }
}
auto
Folder::rowCount(const QModelIndex& parent) const -> int
{
    return childrenFolders.size() + chartData.size();
}
auto
Folder::data(const QModelIndex& index, int role) const -> QVariant
{
    if (role == Qt::DisplayRole) {
        if (index.row() < childrenFolders.size()) {
            return childrenFolders.at(index.row());
        }
        if (index.row() < childrenFolders.size() + chartData.size()) {
            return QVariant::fromValue(
              chartData.at(index.row() - childrenFolders.size()));
        }
    }
    return {};
}
auto
Folder::roleNames() const -> QHash<int, QByteArray>
{
    return { { Qt::DisplayRole, "display" } };
}
auto
Folder::openFolder(QString path) -> Folder*
{
    auto children = QList<QString>{};
    auto query =
      db->createStatement("SELECT path FROM parent_dir WHERE parent_dir = ?");
    query.bind(1, path.toStdString());
    auto result = query.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        children.append(QString::fromStdString(row));
    }
    auto chartQuery =
      db->createStatement("SELECT * FROM charts WHERE directory_in_db "
                          "= ? ORDER BY title ASC");
    chartQuery.bind(1, path.toStdString());
    auto chartResult =
      chartQuery.executeAndGetAll<gameplay_logic::ChartData::DTO>();
    return new Folder{
        std::move(path), std::move(children), std::move(chartResult), db
    };
}
auto
Folder::parentFolder() -> Folder*
{
    if (path == "/") {
        return nullptr;
    }
    auto parent = path;
    parent.resize(parent.size() - 1);
    auto lastSlashIndex = parent.lastIndexOf("/");
    parent.remove(lastSlashIndex + 1, parent.size() - lastSlashIndex - 1);
    return openFolder(parent);
}
auto
Folder::getPath() const -> QString
{
    return path;
}
} // namespace qml_components