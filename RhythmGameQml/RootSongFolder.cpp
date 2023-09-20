//
// Created by bobini on 20.09.23.
//

#include "RootSongFolder.h"

namespace qml_components {
Folder*
RootSongFolder::get()
{
    auto children = QList<QString>{};
    auto query =
      db->createStatement("SELECT path FROM parent_dir WHERE parent_dir = ?");
    query.bind(1, "/");
    auto result = query.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        children.append(QString::fromStdString(row));
    }
    auto chartQuery =
      db->createStatement("SELECT * FROM charts WHERE directory_in_db "
                          "= ? ORDER BY title ASC");
    chartQuery.bind(1, "/");
    auto chartResult =
      chartQuery.executeAndGetAll<gameplay_logic::ChartData::DTO>();
    return new Folder{ "/", std::move(children), std::move(chartResult), db };
}
RootSongFolder::RootSongFolder(db::SqliteCppDb* db, QObject* parent)
  : QObject(parent)
  , db(db)
{
}
auto
RootSongFolder::create(QQmlEngine* engine, QJSEngine* scriptEngine)
  -> RootSongFolder*
{
    Q_ASSERT(instance);
    return instance;
}
void
RootSongFolder::setInstance(RootSongFolder* newInstance)
{
    RootSongFolder::instance = newInstance;
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
}
} // namespace qml_components