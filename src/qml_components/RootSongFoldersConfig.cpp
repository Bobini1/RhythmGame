//
// Created by bobini on 18.09.23.
//

#include "RootSongFoldersConfig.h"
#include "resource_managers/SongAssetStore.h"
#include "support/QStringToPath.h"
#include <algorithm>
#include <QtConcurrentRun>
#include <utility>
#include <qdir.h>
#include <qfileinfo.h>
#include <spdlog/spdlog.h>

namespace qml_components {

namespace {

auto
getStartupRootFolders(db::SqliteCppDb::Statement& getRootFolders)
  -> std::vector<QSharedPointer<RootSongFolder>>
{
    struct RootFolderDTO
    {
        std::string folder;
        int status{};
    };
    auto rootFolders = std::vector<QSharedPointer<RootSongFolder>>{};
    for (const auto result = getRootFolders.executeAndGetAll<RootFolderDTO>();
         const auto& [folder, status] : result) {
        auto statusEnum = static_cast<RootSongFolder::Status>(status);
        if (statusEnum == RootSongFolder::Status::InProgress) {
            statusEnum = RootSongFolder::Status::NotScanned;
        }
        rootFolders.push_back(QSharedPointer<RootSongFolder>::create(
          QString::fromStdString(folder), statusEnum));
        QQmlEngine::setObjectOwnership(rootFolders.back().get(),
                                       QQmlEngine::CppOwnership);
    }
    return rootFolders;
}

auto
canonicalSongSource(const QString& source) -> QString
{
    if (source.isEmpty()) {
        return {};
    }
    const auto url = QUrl{ source };
    const auto path = url.isLocalFile() ? url.toLocalFile() : source;
    const auto info = QFileInfo{ path };
    if (info.isDir()) {
        auto canonical = QDir{ path }.canonicalPath();
        if (!canonical.isEmpty() && !canonical.endsWith('/')) {
            canonical += '/';
        }
        return canonical;
    }
    if (info.isFile() &&
        resource_managers::SongAssetStore::isArchivePath(
          support::qStringToPath(path)) &&
        !resource_managers::SongAssetStore::isSplitArchivePath(
          support::qStringToPath(path))) {
        return info.canonicalFilePath();
    }
    return {};
}

auto
validatePath(const QString& path) -> bool
{
    return !canonicalSongSource(path).isEmpty();
}
} // namespace

RootSongFoldersConfig::RootSongFoldersConfig(RootSongFolders* folders,
                                             ScanningQueue* scanningQueue,
                                             QObject* parent)
  : QObject(parent)
  , folders(folders)
  , scanningQueue(scanningQueue)
{
}
QVariant
ScanningQueue::at(const int index) const
{
    if (index < 0 || index >= scanItems.size()) {
        return QVariant{};
    }
    return QVariant::fromValue(scanItems[index].get());
}
auto
ScanningQueue::scan(RootSongFolder* which) -> bool
{
    if (which == nullptr) {
        return false;
    }
    auto shared = which->sharedFromThis();
    if (!validatePath(which->getName())) {
        spdlog::error("Attempted to scan an invalid song source: {}",
                      which->getName().toStdString());
        return false;
    }
    if (std::ranges::find(scanItems, shared) != scanItems.end()) {
        return false;
    }
    beginInsertRows(QModelIndex(), scanItems.size(), scanItems.size());
    scanItems.push_back(std::move(shared));
    endInsertRows();
    if (scanItems.size() == 1) {
        performTask();
    }
    return true;
}
auto
RootSongFoldersConfig::getFolders() const -> RootSongFolders*
{
    return folders;
}
auto
RootSongFoldersConfig::getScanningQueue() const -> ScanningQueue*
{
    return scanningQueue;
}
auto
ScanningQueue::getCurrentScannedFolder() const -> QString
{
    return currentScannedFolder;
}

RootSongFolder::RootSongFolder(QString name, const Status status)
  : name(std::move(name))
  , status(status)
{
}
auto
RootSongFolder::getName() const -> QString
{
    return name;
}
auto
RootSongFolder::getStatus() const -> Status
{
    return status;
}
void
RootSongFolder::updateStatus(const Status newStatus)
{
    if (newStatus != status) {
        status = newStatus;
        emit statusChanged();
    }
}
auto
RootSongFolders::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid()) {
        return 0;
    }
    return folders.size();
}
auto
RootSongFolders::data(const QModelIndex& index, const int role) const
  -> QVariant
{
    if (role == Qt::DisplayRole && index.row() < folders.size() &&
        index.row() >= 0) {
        return QVariant::fromValue(folders[index.row()].get());
    }
    return QVariant{};
}
RootSongFolders::RootSongFolders(db::SqliteCppDb* db,
                                 ScanningQueue* scanningQueue,
                                 QObject* parent)
  : QAbstractListModel(parent)
  , db(db)
  , scanningQueue(scanningQueue)
{
    folders = getStartupRootFolders(getRootFolders);
    for (const auto& folder : folders) {
        if (folder->getStatus() == RootSongFolder::Status::NotScanned) {
            scanningQueue->scan(folder.get());
        }
    }
}
auto
RootSongFolders::add(const QString& folder) -> bool
{
    const auto canonical = canonicalSongSource(folder);
    if (canonical.isEmpty()) {
        return false;
    }
    for (const auto& rootFolder : folders) {
        const auto existing = rootFolder->getName();
        if (existing == canonical ||
            (existing.endsWith('/') && canonical.startsWith(existing)) ||
            (canonical.endsWith('/') && existing.startsWith(canonical))) {
            return false;
        }
    }
    addRootDir.reset();
    addRootDir.bind(":path", canonical.toStdString());
    addRootDir.execute();
    beginInsertRows(QModelIndex(), folders.size(), folders.size());
    folders.push_back(QSharedPointer<RootSongFolder>::create(
      canonical, RootSongFolder::Status::NotScanned));
    QQmlEngine::setObjectOwnership(folders.back().get(),
                                   QQmlEngine::CppOwnership);
    endInsertRows();
    scanningQueue->scan(folders.back().get());
    return true;
}
void
RootSongFolders::remove(const int index)
{
    if (index < 0 || index >= folders.size()) {
        return;
    }
    removeRootDir.reset();
    removeRootDir.bind(":path", folders[index]->getName().toStdString());
    removeRootDir.execute();
    for (auto i = 0; i < scanningQueue->rowCount(); ++i) {
        if (scanningQueue->at(i).value<RootSongFolder*>() ==
            folders[index].get()) {
            scanningQueue->remove(i);
            break;
        }
    }
    scanningQueue->clear(folders[index]->getName());
    beginRemoveRows(QModelIndex(), index, index);
    folders.erase(folders.begin() + index);
    endRemoveRows();
    if (scanningQueue->rowCount() == 0) {
        emit chartSetMutationCommitted();
    }
}
auto
RootSongFolders::at(const int index) const -> QVariant
{
    if (index < 0 || index >= folders.size()) {
        return QVariant{};
    }
    return QVariant::fromValue(folders[index].get());
}
ScanningQueue::ScanningQueue(db::SqliteCppDb* db,
                             resource_managers::SongDbScanner scanner,
                             QObject* parent)
  : QAbstractListModel(parent)
  , db(db)
  , scanner(scanner)
{
    connect(&scanFutureWatcher, &QFutureWatcher<void>::finished, [this] {
        // fixme: what if we press stop between scanning is finished and this
        // line?
        scanItems.front()->updateStatus(stop
                                          ? RootSongFolder::Status::NotScanned
                                          : RootSongFolder::Status::Scanned);
        updateStatus.reset();
        updateStatus.bind(":dir", scanItems.front()->getName().toStdString());
        updateStatus.bind(":status",
                          static_cast<int>(scanItems.front()->getStatus()));
        updateStatus.execute();
        stop = false;
        beginRemoveRows(QModelIndex(), 0, 0);
        scanItems.pop_front();
        endRemoveRows();
        setCurrentScannedFolder({});
        if (!scanItems.empty()) {
            performTask();
        } else {
            emit queueDrained();
        }
    });
}
void
ScanningQueue::performTask()
{
    const auto& folder = scanItems.front();
    folder->updateStatus(RootSongFolder::Status::InProgress);
    updateStatus.reset();
    updateStatus.bind(":dir", folder->getName().toStdString());
    updateStatus.bind(":status", static_cast<int>(folder->getStatus()));
    updateStatus.execute();
    scanImpl(folder->getName());
}
void
ScanningQueue::remove(const int index)
{
    if (index < 0 || index >= scanItems.size()) {
        return;
    }
    if (index == 0) {
        stop = true;
    } else {
        beginRemoveRows(QModelIndex(), index, index);
        scanItems.erase(scanItems.begin() + index);
        endRemoveRows();
    }
}
void
ScanningQueue::scanImpl(const QString& which)
{
    scanFuture = QtConcurrent::run([this, which] {
        clear(which);
        scanner.scanDirectory(
          support::qStringToPath(which),
          [this](QString newCurrentScannedFolder) {
              QMetaObject::invokeMethod(
                this,
                [this,
                 newCurrentScannedFolder = std::move(newCurrentScannedFolder)] {
                    setCurrentScannedFolder(newCurrentScannedFolder);
                },
                Qt::QueuedConnection);
          },
          &stop);
        if (stop) {
            clear(which);
        }
    });
    scanFutureWatcher.setFuture(scanFuture);
}
void
ScanningQueue::setCurrentScannedFolder(QString folder)
{
    if (currentScannedFolder == folder) {
        return;
    }
    currentScannedFolder = std::move(folder);
    emit currentScannedFolderChanged();
}
void
ScanningQueue::clear(const QString& which)
{
    auto sourcePrefix = which;
    if (!sourcePrefix.endsWith('/')) {
        sourcePrefix += '/';
    }

    auto removeSongsStartingWith =
      db->createStatement("DELETE FROM charts WHERE path LIKE :dir || '%'");
    removeSongsStartingWith.bind(":dir", sourcePrefix.toStdString());
    removeSongsStartingWith.execute();
    db->execute("WITH RECURSIVE "
                "chart_dirs(dir) AS ( "
                "  SELECT pd.dir "
                "  FROM parent_dir pd "
                "  WHERE pd.id IN (SELECT directory FROM charts) "
                "), "
                "keep(dir) AS ( "
                "  SELECT dir FROM chart_dirs "
                "  UNION "
                "  SELECT p.dir "
                "  FROM parent_dir p "
                "  JOIN parent_dir child ON child.parent_dir = p.dir "
                "  JOIN keep k ON k.dir = child.dir "
                ") "
                "DELETE FROM parent_dir "
                "WHERE dir NOT IN (SELECT dir FROM keep);");
    db->execute("DELETE FROM note_data WHERE note_data.sha256 NOT IN "
                "(SELECT sha256 FROM charts)");
    db->execute(
      "DELETE FROM histogram_data WHERE NOT EXISTS "
      "(SELECT 1 FROM charts WHERE charts.id = histogram_data.chart_id)");
    db->execute("DELETE FROM preview_files WHERE directory NOT IN "
                "(SELECT chart_directory FROM charts)");
    db->execute("DELETE FROM readme_files WHERE directory NOT IN "
                "(SELECT chart_directory FROM charts)");
}
auto
ScanningQueue::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid()) {
        return 0;
    }
    return scanItems.size();
}
auto
ScanningQueue::data(const QModelIndex& index, int role) const -> QVariant
{
    if (role == Qt::DisplayRole && index.row() < scanItems.size() &&
        index.row() >= 0) {
        return QVariant::fromValue(scanItems[index.row()].get());
    }
    return QVariant{};
}
ScanningQueue::~ScanningQueue()
{
    const auto rows = rowCount();
    for (auto i = rows; i >= 0; i--) {
        remove(i);
    }
    scanFuture.waitForFinished();
}
} // namespace qml_components
