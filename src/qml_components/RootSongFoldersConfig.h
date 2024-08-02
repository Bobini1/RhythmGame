//
// Created by bobini on 18.09.23.
//

#ifndef RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H
#define RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H

#include "db/SqliteCppDb.h"
#include "resource_managers/SongDbScanner.h"

#include <qfuture.h>
#include <QAbstractListModel>
#include <QFutureWatcher>

namespace qml_components {
class RootSongFolder final
  : public QObject
  , public QEnableSharedFromThis<RootSongFolder>
{
    Q_OBJECT
  public:
    enum Status
    {
        NotScanned,
        InProgress,
        Scanned,
    };

  private:
    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(Status status READ getStatus NOTIFY statusChanged)
    QString name;
    Status status;

  public:
    RootSongFolder(QString name, Status status);
    auto getName() const -> QString;
    auto getStatus() const -> Status;
    void updateStatus(Status newStatus);

  signals:
    void statusChanged();
};

class ScanningQueue;
class RootSongFolders final : public QAbstractListModel
{
    Q_OBJECT
    std::vector<QSharedPointer<RootSongFolder>> folders;
    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getRootFolders =
      db->createStatement("SELECT path, status FROM root_dir");
    db::SqliteCppDb::Statement removeRootDir =
      db->createStatement("DELETE FROM root_dir WHERE path = :path");
    db::SqliteCppDb::Statement addRootDir =
      db->createStatement("INSERT INTO root_dir (path) VALUES (:path)");
    ScanningQueue* scanningQueue{};

  public:
    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index = QModelIndex(),
              int role = Qt::DisplayRole) const -> QVariant override;
    explicit RootSongFolders(db::SqliteCppDb* db,
                             ScanningQueue* scanningQueue,
                             QObject* parent = nullptr);
    Q_INVOKABLE bool add(const QString& folder);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE QVariant at(int index) const;
};

class ScanningQueue final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentScannedFolder READ getCurrentScannedFolder NOTIFY
                 currentScannedFolderChanged)
    QString currentScannedFolder;
    std::deque<QSharedPointer<RootSongFolder>> scanItems;
    QFuture<void> scanFuture;
    QFutureWatcher<void> scanFutureWatcher;
    std::atomic_bool stop;

    db::SqliteCppDb* db;
    resource_managers::SongDbScanner scanner;
    db::SqliteCppDb::Statement removeSongsStartingWith = db->createStatement(
      "DELETE FROM charts WHERE directory LIKE :dir || '%'");
    db::SqliteCppDb::Statement removeParentDirsStartingWith =
      db->createStatement("DELETE FROM parent_dir WHERE dir LIKE :dir || '%'");
    db::SqliteCppDb::Statement updateStatus =
      db->createStatement("UPDATE root_dir SET status = :status WHERE path = "
                          ":dir");

    void scanImpl(const QString& which);
    void clearImpl(const QString& which);

  public:
    ~ScanningQueue() override;
    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index = QModelIndex(),
              int role = Qt::DisplayRole) const -> QVariant override;
    explicit ScanningQueue(db::SqliteCppDb* db,
                           resource_managers::SongDbScanner scanner,
                           QObject* parent = nullptr);
    Q_INVOKABLE void performTask();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE QVariant at(int index) const;
    Q_INVOKABLE bool scan(RootSongFolder* which);
    auto getCurrentScannedFolder() const -> QString;

  signals:
    void currentScannedFolderChanged();
};

class RootSongFoldersConfig final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(RootSongFolders* folders READ getFolders CONSTANT)
    Q_PROPERTY(ScanningQueue* scanningQueue READ getScanningQueue CONSTANT)
    RootSongFolders* folders;
    ScanningQueue* scanningQueue;

  public:
    explicit RootSongFoldersConfig(RootSongFolders* folders,
                                   ScanningQueue* scanningQueue,
                                   QObject* parent = nullptr);
    auto getFolders() const -> RootSongFolders*;
    auto getScanningQueue() const -> ScanningQueue*;
};

} // namespace qml_components

#endif // RHYTHMGAME_ROOTSONGFOLDERSCONFIG_H
