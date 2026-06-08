//
// Created by bobini on 05.10.23.
//

#ifndef RHYTHMGAME_SONGDIRECTORYFILEPATHFETCHER_H
#define RHYTHMGAME_SONGDIRECTORYFILEPATHFETCHER_H

#include <QVariantHash>
#include "db/SqliteCppDb.h"
#include <string>
namespace qml_components {

class SongDirectoryFilePathFetcher : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;

    auto getFilePaths(QList<QString> directories, const std::string& table)
      const -> QVariantHash;

  public:
    explicit SongDirectoryFilePathFetcher(db::SqliteCppDb* db,
                                          QObject* parent = nullptr);
    /**
     * @brief Fetches the preview file paths for the given directories.
     * @param directories A list of directories to fetch preview file paths for.
     * @return A hash map where the key is the directory and the value is the
     * preview file path. The result will not contain entries for directories
     * that do not have a preview file path.
     */
    Q_INVOKABLE QVariantHash getPreviewFilePaths(QList<QString> directories) const;

    /**
     * @brief Fetches the readme file paths for the given directories.
     * @param directories A list of directories to fetch readme file paths for.
     * @return A hash map where the key is the directory and the value is the
     * readme file path. The result will not contain entries for directories
     * that do not have a readme file path.
     */
    Q_INVOKABLE QVariantHash getReadmeFilePaths(QList<QString> directories) const;
};

} // namespace qml_components

#endif // RHYTHMGAME_SONGDIRECTORYFILEPATHFETCHER_H
