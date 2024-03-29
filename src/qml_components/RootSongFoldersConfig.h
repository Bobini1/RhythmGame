//
// Created by bobini on 18.09.23.
//

#ifndef RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H
#define RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H

#include "db/SqliteCppDb.h"
#include "resource_managers/SongDbScanner.h"

#include <qfuture.h>
namespace qml_components {

class RootSongFoldersConfig : public QObject
{
    Q_OBJECT

  public:
    enum class Status
    {
        Loading,
        Ready
    };
    Q_ENUM(Status)
  private:
    Q_PROPERTY(Status status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QStringList folders READ getFolders WRITE setFolders NOTIFY
                 foldersChanged)
    db::SqliteCppDb* db;
    std::atomic<Status> status = Status::Ready;
    QStringList folders;
    resource_managers::SongDbScanner scanner;
    db::SqliteCppDb::Statement getRootFolders =
      db->createStatement("SELECT path FROM root_dir");
    db::SqliteCppDb::Statement removeRootDir =
      db->createStatement("DELETE FROM root_dir WHERE path = :path");
    db::SqliteCppDb::Statement addRootDir =
      db->createStatement("INSERT INTO root_dir (path) VALUES (:path)");
    // delete all songs where path starts with :path
    db::SqliteCppDb::Statement getSha256OfSongsStartingWith =
      db->createStatement("SELECT sha256 FROM charts WHERE path LIKE :path || "
                          "'%'");
    db::SqliteCppDb::Statement removeSongsStartingWith =
      db->createStatement("DELETE FROM charts WHERE path LIKE :path || '%'");
    db::SqliteCppDb::Statement removeNoteDataStartingWith = db->createStatement(
      "DELETE FROM note_data WHERE sha256 IN (SELECT sha256 "
      "FROM charts WHERE path LIKE :path || '%')");
    db::SqliteCppDb::Statement removePreviewFilesStartingWith =
      db->createStatement("DELETE FROM preview_files WHERE path LIKE :path || "
                          "'%'");
    db::SqliteCppDb::Statement getDistinctDirectoryInDb =
      db->createStatement("SELECT DISTINCT directory_in_db FROM charts");
    db::SqliteCppDb::Statement addParentDir =
      db->createStatement("INSERT INTO parent_dir (parent_dir, path) VALUES "
                          "(:parent_dir, :path)");
    db::SqliteCppDb::Statement addToPendingRootDirs =
      db->createStatement("INSERT INTO pending_root_dir (path) VALUES (:path)");
    db::SqliteCppDb::Statement getPendingRootDirs =
      db->createStatement("SELECT path FROM pending_root_dir");
    QFuture<void> scanFuture;
    void scanNewImpl();
    void scanAllImpl();

  public:
    explicit RootSongFoldersConfig(db::SqliteCppDb* db,
                                   resource_managers::SongDbScanner scanner,
                                   QObject* parent = nullptr);
    auto getStatus() const -> Status;
    auto getFolders() const -> QStringList;
    void setFolders(QStringList folders);
    Q_INVOKABLE void scanNew();
    Q_INVOKABLE void scanAll();
    Q_INVOKABLE void clear();

  signals:
    void statusChanged();
    void foldersChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H
