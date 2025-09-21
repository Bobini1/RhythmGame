//
// Created by PC on 20/09/2025.
//

#include "MigrateTo1Point2.h"

#include "support/QStringToPath.h"

#include <QGuiApplication>
#include <QFileInfo>

namespace migrations {

void
migrateTo1Point2(const std::filesystem::path& dataFolder)
{
    auto programFolder = QFileInfo(QGuiApplication::applicationFilePath());
    auto legacyDataFolder = programFolder.filesystemPath().parent_path() / "data";
    if (!std::filesystem::exists(legacyDataFolder)) {
        return;
    }
    // copy song_db.sqlite, song_db.sqlite-shm and song_db.sqlite-wal if they exist
    auto dbFile = legacyDataFolder / "song_db.sqlite";
    auto walFile = legacyDataFolder / "song_db.sqlite-wal";
    auto shmFile = legacyDataFolder / "song_db.sqlite-shm";
    std::error_code ec;
    std::filesystem::rename(dbFile,
                               dataFolder / "song_db.sqlite",
                               ec);
    std::filesystem::rename(walFile,
                               dataFolder / "song_db.sqlite-shm",
                               ec);
    std::filesystem::rename(shmFile,
                               dataFolder / "song_db.sqlite-wal",
                               ec);
    auto profilesFolder = legacyDataFolder / "profiles";
    std::filesystem::rename(profilesFolder,
                               dataFolder / "profiles",
                               ec);
    auto avatarsFolder = legacyDataFolder / "avatars";
    std::filesystem::rename(avatarsFolder,
                               dataFolder / "avatars",
                               ec);
   std::filesystem::remove_all(legacyDataFolder / "themes", ec);
   std::filesystem::remove(legacyDataFolder / "log.txt", ec);
   std::filesystem::remove(legacyDataFolder, ec);
}
} // namespace migrations