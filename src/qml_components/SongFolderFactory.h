//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_SONGFOLDERFACTORY_H
#define RHYTHMGAME_SONGFOLDERFACTORY_H

#include <QObject>
#include "db/SqliteCppDb.h"

namespace qml_components {

class SongFolderFactory : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getCharts =
      db->createStatement("SELECT * FROM charts WHERE directory_in_db "
                          "= ? ORDER BY title, subtitle ASC");
    db::SqliteCppDb::Statement getFolders =
      db->createStatement("SELECT path FROM parent_dir WHERE parent_dir = ? "
                          "ORDER BY path ASC");
    // query to get count of charts and subfolders
    db::SqliteCppDb::Statement getSize =
      db->createStatement("SELECT COUNT(*) FROM parent_dir WHERE parent_dir = "
                          "? UNION SELECT COUNT(*) FROM charts WHERE "
                          "directory_in_db = ?");
    db::SqliteCppDb::Statement searchFolders =
      db->createStatement("SELECT path FROM parent_dir WHERE path LIKE ? "
                          "ORDER BY path ASC");
    db::SqliteCppDb::Statement searchCharts = db->createStatement(
      "SELECT * FROM charts WHERE title LIKE :query OR "
      "artist LIKE :query OR subtitle LIKE :query or subartist LIKE "
      ":query or genre LIKE :query ORDER BY title, subtitle ASC");

  public:
    explicit SongFolderFactory(db::SqliteCppDb* db, QObject* parent = nullptr);
    Q_INVOKABLE QVariantList open(QString path);
    Q_INVOKABLE int folderSize(QString path);
    Q_INVOKABLE QString parentFolder(QString path);
    Q_INVOKABLE QVariantList search(QString query);
};

} // namespace qml_components

#endif // RHYTHMGAME_SONGFOLDERFACTORY_H
