//
// Created by bobini on 18.09.23.
//

#include "RootSongFoldersConfig.h"
#include "support/QStringToPath.h"
#include <algorithm>
#include <QtConcurrentRun>
#include <utility>
#include <qdir.h>
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
    // get all root folders
    for (const auto result = getRootFolders.executeAndGetAll<RootFolderDTO>();
         const auto& [folder, status] : result) {
        auto statusEnum = static_cast<RootSongFolder::Status>(status);
        rootFolders.push_back(QSharedPointer<RootSongFolder>::create(
          QString::fromStdString(folder), statusEnum));
        QQmlEngine::setObjectOwnership(rootFolders.back().get(),
                                       QQmlEngine::CppOwnership);
    }
    return rootFolders;
}

auto
validatePath(const QString& path) -> bool
{
    if (path.isEmpty()) {
        return false;
    }
    const auto dir = QDir{ QUrl{ path }.toLocalFile() };
    return dir.exists();
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
        spdlog::error("Attempted to scan an invalid directory: {}",
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
    const auto path = QUrl(folder).toLocalFile();
    const auto dir = QDir{ path };
    const auto canonical = dir.canonicalPath() + "/";
    if (canonical == "/") {
        return false;
    }
    for (const auto& rootFolder : folders) {
        if (auto folderName = rootFolder->getName();
            folderName.startsWith(canonical) ||
            canonical.startsWith(folderName)) {
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
        if (!scanItems.empty()) {
            performTask();
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
    scanFuture = QtConcurrent::run([this, which] {
        clear(which);
        scanner.scanDirectory(
          support::qStringToPath(which),
          [this](QString newCurrentScannedFolder) {
              QMetaObject::invokeMethod(
                this,
                [this,
                 newCurrentScannedFolder = std::move(newCurrentScannedFolder)] {
                    currentScannedFolder = newCurrentScannedFolder;
                    emit currentScannedFolderChanged();
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
ScanningQueue::clear(const QString& which)
{
    const auto folderNameStd = which.toStdString();

    removeSongsStartingWith.reset();
    removeSongsStartingWith.bind(":dir", folderNameStd);
    removeSongsStartingWith.execute();
    db->execute("DELETE FROM parent_dir WHERE parent_dir.id NOT IN "
                "(SELECT directory FROM charts)");
    db->execute("DELETE FROM note_data WHERE note_data.sha256 NOT IN "
                "(SELECT sha256 FROM charts)");
    db->execute("DELETE FROM preview_files WHERE directory NOT IN "
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