//
// Created by bobini on 18.09.23.
//

#include "RootSongFoldersConfig.h"
#include "support/QStringToPath.h"
#include "support/UtfStringToPath.h"
#include <algorithm>
#include <QtConcurrentRun>

namespace qml_components {
auto
RootSongFoldersConfig::getStatus() const -> RootSongFoldersConfig::Status
{
    return Status::Loading;
}
RootSongFoldersConfig::RootSongFoldersConfig(
  db::SqliteCppDb* db,
  resource_managers::SongDbScanner scanner,
  QObject* parent)
  : QObject(parent)
  , db(db)
  , scanner(scanner)
{
    // get all root folders
    auto result = getPendingRootDirs.executeAndGetAll<std::string>();
    for (const auto& row : result) {
        folders.append(QString::fromStdString(row));
    }
}
void
RootSongFoldersConfig::scanNew()
{
    if (status == Status::Loading) {
        spdlog::warn("Can't scan - already scanning");
        return;
    }
    status = Status::Loading;
    emit statusChanged();
    spdlog::info("Scanning new folders");
    scanFuture = QtConcurrent::run([this] {
        scanNewImpl();
        status = Status::Ready;
        emit statusChanged();
        spdlog::info("Scanning new folders finished");
    });
}
auto
RootSongFoldersConfig::getFolders() const -> QStringList
{
    return folders;
}
void
RootSongFoldersConfig::setFolders(QStringList folders)
{
    this->folders = std::move(folders);
    // remove pending root dirs
    db->execute("DELETE FROM pending_root_dir");
    // add all root dirs
    for (const auto& folder : this->folders) {
        addToPendingRootDirs.reset();
        addToPendingRootDirs.bind(":path", folder.toStdString());
        addToPendingRootDirs.execute();
    }
    emit foldersChanged();
}
void
RootSongFoldersConfig::scanAll()
{
    if (status == Status::Loading) {
        spdlog::warn("Can't scan - already scanning");
        return;
    }
    status = Status::Loading;
    emit statusChanged();
    spdlog::info("Scanning all folders");
    scanFuture = QtConcurrent::run([this] {
        scanAllImpl();
        status = Status::Ready;
        emit statusChanged();
        spdlog::info("Scanning all folders finished");
    });
}
void
RootSongFoldersConfig::scanNewImpl()
{
    auto foldersVector = std::vector<std::string>{};
    foldersVector.reserve(folders.size());
    for (const auto& folder : folders) {
        foldersVector.emplace_back(support::qStringToPath(folder));
    }
    getRootFolders.reset();
    auto currentRootFolders = getRootFolders.executeAndGetAll<std::string>();

    auto removedFolders = std::vector<std::string>{};
    std::set_difference(currentRootFolders.begin(),
                        currentRootFolders.end(),
                        foldersVector.begin(),
                        foldersVector.end(),
                        std::back_inserter(removedFolders));
    for (const auto& removedFolder : removedFolders) {
        removeSongsStartingWith.reset();
        removeSongsStartingWith.bind(":path", removedFolder);
        removeSongsStartingWith.execute();
    }

    // remove everything from parent_dir
    db->execute("DELETE FROM parent_dir");

    // rebuild parent dirs from what was left
    auto directoryInDbResult =
      getDistinctDirectoryInDb.executeAndGetAll<std::string>();
    for (const auto& row : directoryInDbResult) {
        auto directoryInDb = QString::fromStdString(row);
        while (directoryInDb.size() != 1) {
            auto parentDirectory = directoryInDb;
            parentDirectory.resize(directoryInDb.size() - 1);
            auto lastSlashIndex = parentDirectory.lastIndexOf("/");
            parentDirectory.remove(lastSlashIndex + 1,
                                   directoryInDb.size() - lastSlashIndex - 1);
            addParentDir.reset();
            addParentDir.bind(":parent_dir", parentDirectory.toStdString());
            addParentDir.bind(":path", directoryInDb.toStdString());
            addParentDir.execute();
            directoryInDb = std::move(parentDirectory);
        }
    }

    // remove removed root folders
    for (const auto& removedFolder : removedFolders) {
        removeRootDir.reset();
        removeRootDir.bind(":path", removedFolder);
        removeRootDir.execute();
    }

    auto addedFolders = std::vector<std::string>{};
    std::set_difference(foldersVector.begin(),
                        foldersVector.end(),
                        currentRootFolders.begin(),
                        currentRootFolders.end(),
                        std::back_inserter(addedFolders));

    // add new root folders
    for (const auto& addedFolder : addedFolders) {
        addRootDir.reset();
        addRootDir.bind(":path", addedFolder);
        addRootDir.execute();
    }

    auto addedFoldersPaths = std::vector<std::filesystem::path>{};
    addedFoldersPaths.reserve(addedFolders.size());
    for (const auto& addedFolder : addedFolders) {
        addedFoldersPaths.emplace_back(support::utfStringToPath(addedFolder));
    }

    scanner.scanDirectories(addedFoldersPaths);
}
void
RootSongFoldersConfig::scanAllImpl()
{
    // remove everything from parent_dir
    db->execute("DELETE FROM parent_dir");
    // remove all root folders
    db->execute("DELETE FROM root_dir");
    // add current folders as root folders
    for (const auto& folder : folders) {
        addRootDir.reset();
        addRootDir.bind(":path", folder.toStdString());
        addRootDir.execute();
    }

    // remove all songs
    db->execute("DELETE FROM charts");

    // scan all folders
    auto foldersVector = std::vector<std::filesystem::path>{};
    foldersVector.reserve(folders.size());
    for (const auto& folder : folders) {
        foldersVector.emplace_back(support::qStringToPath(folder));
    }
    scanner.scanDirectories(foldersVector);
}
void
RootSongFoldersConfig::clear()
{
    if (status == Status::Loading) {
        spdlog::warn("Can't clear - already scanning");
        return;
    }
    status = Status::Loading;
    emit statusChanged();
    spdlog::info("Clearing database");
    scanFuture = QtConcurrent::run([this] {
        db->execute("DELETE FROM charts");
        db->execute("DELETE FROM parent_dir");
        db->execute("DELETE FROM root_dir");
        status = Status::Ready;
        emit statusChanged();
        spdlog::info("Clearing database finished");
    });
}
} // namespace qml_components