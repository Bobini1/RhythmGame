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
    const auto archivePath = support::qStringToPath(path);
    if (info.isFile() &&
        (resource_managers::SongAssetStore::isArchivePath(archivePath) ||
         resource_managers::SongAssetStore::isSplitArchivePath(archivePath))) {
        const auto supportError =
          resource_managers::SongAssetStore::archiveSupportError(archivePath);
        if (!supportError.isEmpty()) {
            spdlog::error("{}", supportError.toStdString());
            return {};
        }
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
        return true;
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
    threadPool.setMaxThreadCount(1);
    progressTimer.setInterval(std::chrono::milliseconds(100));
    connect(&progressTimer, &QTimer::timeout, this, [this] {
        QString folder;
        {
            const auto lock = std::lock_guard(progressMutex);
            folder = std::exchange(pendingScannedFolder, {});
        }
        if (!folder.isEmpty()) {
            setCurrentScannedFolder(std::move(folder));
        }
    });
    connect(&scanFutureWatcher,
            &QFutureWatcher<RootSongFolder::Status>::finished,
            this,
            [this] {
                auto status = RootSongFolder::Status::NotScanned;
                try {
                    status = scanFutureWatcher.result();
                } catch (const std::exception& error) {
                    spdlog::error("Song scan failed: {}", error.what());
                }
                scanItems.front()->updateStatus(status);
                stop = false;
                beginRemoveRows(QModelIndex(), 0, 0);
                scanItems.pop_front();
                endRemoveRows();
                progressTimer.stop();
                {
                    const auto lock = std::lock_guard(progressMutex);
                    pendingScannedFolder.clear();
                }
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
    progressTimer.start();
    scanFuture = QtConcurrent::run(&threadPool, [this, which] {
        saveStatus(which, RootSongFolder::Status::InProgress);
        clear(which);
        scanner.scanDirectory(
          support::qStringToPath(which),
          [this](QString newCurrentScannedFolder) {
              const auto lock = std::lock_guard(progressMutex);
              pendingScannedFolder = std::move(newCurrentScannedFolder);
          },
          &stop);
        const auto status = stop ? RootSongFolder::Status::NotScanned
                                 : RootSongFolder::Status::Scanned;
        if (status == RootSongFolder::Status::NotScanned) {
            clear(which);
        }
        saveStatus(which, status);
        return status;
    });
    scanFutureWatcher.setFuture(scanFuture);
}
void
ScanningQueue::saveStatus(const QString& folder, RootSongFolder::Status status)
{
    updateStatus.reset();
    updateStatus.bind(":dir", folder.toStdString());
    updateStatus.bind(":status", static_cast<int>(status));
    updateStatus.execute();
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

    auto transaction = db->transaction();
    auto removeSongsStartingWith =
      db->createStatement("DELETE FROM charts WHERE instr(path, :dir) = 1");
    removeSongsStartingWith.bind(":dir", sourcePrefix.toStdString());
    removeSongsStartingWith.execute();
    auto removeDirectories =
      db->createStatement("DELETE FROM parent_dir WHERE instr(dir, :dir) = 1");
    removeDirectories.bind(":dir", sourcePrefix.toStdString());
    removeDirectories.execute();
    db->execute("DELETE FROM note_data WHERE note_data.sha256 NOT IN "
                "(SELECT sha256 FROM charts)");
    db->execute(
      "DELETE FROM histogram_data WHERE NOT EXISTS "
      "(SELECT 1 FROM charts WHERE charts.id = histogram_data.chart_id)");
    for (const auto* table : { "preview_files", "readme_files" }) {
        auto removeAssets =
          db->createStatement(std::string("DELETE FROM ") + table +
                              " WHERE instr(directory, :dir) = 1");
        removeAssets.bind(":dir", sourcePrefix.toStdString());
        removeAssets.execute();
    }
    transaction.commit();
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
