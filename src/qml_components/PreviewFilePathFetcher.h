//
// Created by bobini on 05.10.23.
//

#ifndef RHYTHMGAME_PREVIEWFILEPATHFETCHER_H
#define RHYTHMGAME_PREVIEWFILEPATHFETCHER_H

#include <QObject>
#include "db/SqliteCppDb.h"
namespace qml_components {

class PreviewFilePathFetcher : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getPreviewFilePathStatement =
      db->createStatement("SELECT path FROM preview_files WHERE "
                          "directory = :directory");

  public:
    explicit PreviewFilePathFetcher(db::SqliteCppDb* db,
                                    QObject* parent = nullptr);
    Q_INVOKABLE QString getPreviewFilePath(const QString& directory);
};

} // namespace qml_components

#endif // RHYTHMGAME_PREVIEWFILEPATHFETCHER_H
