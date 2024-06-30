//
// Created by bobini on 14.09.23.
//

#include <stack>
#include <utility>
#include "SongDbScanner.h"
#include "db/SqliteCppDb.h"
#include "support/PathToQString.h"
#include "support/PathToUtfString.h"

#include <qthreadpool.h>

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db)
  : db(db)
{
}

void
addDirToParentDirs(QThreadPool& threadPool,
                   db::SqliteCppDb& db,
                   QString root,
                   QString folder)
{
    threadPool.start([&db, root, folder]() mutable {
        thread_local auto insert =
          db.createStatement("INSERT OR IGNORE INTO parent_dir "
                             "(parent_dir, dir) VALUES (:parent_dir, "
                             ":dir)");
        thread_local auto getIdQuery =
          db.createStatement("SELECT id FROM parent_dir WHERE dir = :dir");
        auto parent = int64_t{ -1 };
        auto current = root;
        auto rest = folder.right(folder.size() - root.size() - 1);
        while (true) {
            insert.reset();
            if (parent == int64_t{ -1 }) {
                insert.bind(":parent_dir");
            } else {
                insert.bind(std::string(":parent_dir"), parent);
            }
            insert.bind(":dir", current.toStdString());
            insert.execute();
            if (current == folder) {
                break;
            }
            getIdQuery.reset();
            getIdQuery.bind(":dir", current.toStdString());
            parent = getIdQuery.executeAndGet<int64_t>().value();
            current = current + rest.left(rest.indexOf('/'));
            rest = rest.right(rest.size() - rest.indexOf('/') - 1);
        }
    });
}
void
loadChart(QThreadPool& threadPool,
          db::SqliteCppDb& db,
          const QString& directory,
          const std::filesystem::path& path)
{
    auto url = support::pathToQString(path);
    threadPool.start([&db, url = std::move(url), directory]() mutable {
        try {
            thread_local constexpr ChartDataFactory chartDataFactory;
            auto randomGenerator =
              [](charts::parser_models::ParsedBmsChart::RandomRange
                   randomRange) {
                  thread_local auto randomEngine =
                    std::default_random_engine{ std::random_device{}() };
                  return std::uniform_int_distribution{
                      charts::parser_models::ParsedBmsChart::RandomRange{ 1 },
                      randomRange
                  }(randomEngine);
              };

            const auto chartComponents =
              chartDataFactory.loadChartData(url, randomGenerator, directory);
            chartComponents.chartData->save(db);
            chartComponents.bmsNotes->save(
              db, chartComponents.chartData->getSha256().toStdString());
        } catch (const std::exception& e) {
            spdlog::error("Failed to load chart data for {}: {}",
                          url.toStdString(),
                          e.what());
        }
    });
}

void
addPreviewFileToDb(db::SqliteCppDb& db,
                   const std::filesystem::path& directory,
                   const std::filesystem::path& path)
{
    thread_local auto statement = db.createStatement(
      "INSERT OR REPLACE INTO preview_files (path, directory) "
      "VALUES (?, ?)");
    statement.reset();
    statement.bind(1, support::pathToUtfString(path));
    statement.bind(2, support::pathToUtfString(directory / ""));
    statement.execute();
}

void
scanFolder(std::filesystem::path directory,
           std::filesystem::path parentDirectory,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           const QString& root,
           std::function<void(QString)> updateCurrentScannedFolder,
           std::atomic_bool* stop)
{
    updateCurrentScannedFolder(support::pathToQString(directory));
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (*stop) {
            break;
        }
        const auto& path = entry.path();
        if (is_directory(entry) && !isSongDirectory) {
            directoriesToScan.push_back(path);
        } else if (auto extension = path.extension();
                   extension == ".bms" || extension == ".bme" ||
                   extension == ".bml" || extension == ".pms") {
            isSongDirectory = true;
            directoriesToScan.clear();
            if (extension == ".pms") {
                continue;
            }
            loadChart(
              threadPool, db, support::pathToQString(parentDirectory), path);
            // converting to string should not break stuff even on windows
            // in this case
        } else if (path.filename().string().starts_with("preview") &&
                   (extension == ".mp3" || extension == ".ogg" ||
                    extension == ".wav" || extension == ".flac")) {
            threadPool.start([&db, directory, path] {
                addPreviewFileToDb(db, directory, path);
            });
        }
    }
    for (const auto& entry : directoriesToScan) {
        if (*stop) {
            break;
        }
        scanFolder(entry,
                   directory,
                   threadPool,
                   db,
                   root,
                   updateCurrentScannedFolder,
                   stop);
    }
    if (isSongDirectory) {
        addDirToParentDirs(
          threadPool, db, root, support::pathToQString(parentDirectory));
    }
}

void
SongDbScanner::scanDirectory(
  std::filesystem::path directory,
  std::function<void(QString)> updateCurrentScannedFolder,
  std::atomic_bool* stop) const
{
    auto threadPool = QThreadPool{};
    if (is_directory(directory)) {
        const auto root = support::pathToQString(directory);
        scanFolder(std::move(directory),
                   {},
                   threadPool,
                   *db,
                   root,
                   updateCurrentScannedFolder,
                   stop);
    } else {
        spdlog::error("Resource path {} is not a directory",
                      directory.string());
    }
    threadPool.waitForDone();
    updateCurrentScannedFolder("");
}
} // namespace resource_managers