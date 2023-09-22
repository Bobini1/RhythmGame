//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_SONGFOLDERFACTORY_H
#define RHYTHMGAME_SONGFOLDERFACTORY_H

#include <QObject>
#include <QtQmlIntegration>
#include "Folder.h"
namespace qml_components {

class SongFolderFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getCharts =
      db->createStatement("SELECT * FROM charts WHERE directory_in_db "
                          "= ? ORDER BY title ASC");
    db::SqliteCppDb::Statement getFolders =
      db->createStatement("SELECT path FROM parent_dir WHERE parent_dir = ? "
                          "ORDER BY path ASC");
    // query to get count of charts and subfolders
    db::SqliteCppDb::Statement getSize =
      db->createStatement("SELECT COUNT(*) FROM parent_dir WHERE parent_dir = "
                          "? UNION SELECT COUNT(*) FROM charts WHERE "
                          "directory_in_db = ?");

  public:
    explicit SongFolderFactory(db::SqliteCppDb* db, QObject* parent = nullptr);
    Q_INVOKABLE Folder* open(QString path);
    Q_INVOKABLE int folderSize(QString path);
};

} // namespace qml_components

#endif // RHYTHMGAME_SONGFOLDERFACTORY_H
