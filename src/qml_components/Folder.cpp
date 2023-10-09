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
      [this](const gameplay_logic::ChartData::DTO& dto) {
          auto chartData = gameplay_logic::ChartData::load(dto);
          chartData->moveToThread(thread());
          return chartData.release();
      }))
{
    for (auto& chart : chartData) {
        chart->setParent(this);
    }
}
auto
Folder::rowCount(const QModelIndex& parent) const -> int
{
    return (childrenFolders.size() + chartData.size() > 0)
             ? std::max(
                 minimumAmount,
                 static_cast<int>(childrenFolders.size() + chartData.size()))
             : 0;
}
auto
Folder::data(const QModelIndex& index, int role) const -> QVariant
{
    if (role == Qt::DisplayRole) {
        auto idx = index.row();
        if (idx < childrenFolders.size()) {
            return childrenFolders.at(idx);
        }
        if (idx < childrenFolders.size() + chartData.size()) {
            return QVariant::fromValue(
              chartData.at(idx - childrenFolders.size()));
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
Folder::parentFolder() -> QString
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
auto
Folder::getPath() const -> QString
{
    return path;
}
auto
Folder::at(int index) -> QVariant
{
    return data(createIndex(index, 0));
}
} // namespace qml_components