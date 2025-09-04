//
// Created by bobini on 05.10.23.
//

#ifndef RHYTHMGAME_PREVIEWFILEPATHFETCHER_H
#define RHYTHMGAME_PREVIEWFILEPATHFETCHER_H

#include <QVariantHash>
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
    /**
     * @brief Fetches the preview file paths for the given directories.
     * @param directories A list of directories to fetch preview file paths for.
     * @return A hash map where the key is the directory and the value is the
     * preview file path. The result will not contain entries for directories
     * that do not have a preview file path.
     */
    Q_INVOKABLE QVariantHash getPreviewFilePaths(QList<QString> directories) const;
};

} // namespace qml_components

#endif // RHYTHMGAME_PREVIEWFILEPATHFETCHER_H
