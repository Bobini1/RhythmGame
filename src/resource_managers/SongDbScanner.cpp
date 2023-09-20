//
// Created by bobini on 14.09.23.
//

#include <stack>
#include "SongDbScanner.h"
#include "db/SqliteCppDb.h"
#include "support/PathToQString.h"

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db)
  : db(db)
{
}

void
addDirToParentDirs(QThreadPool& threadPool,
                   db::SqliteCppDb& db,
                   QString directoryInDb)
{ // add to parent_dir
    threadPool.start([&db, directoryInDb]() mutable {
        try {
            static thread_local auto insertQuery = db.createStatement(
              "INSERT INTO parent_dir (parent_dir, path) VALUES (:parent_dir, "
              ":path)");
            while (directoryInDb.size() != 1) {
                auto parentDirectory = directoryInDb;
                parentDirectory.resize(parentDirectory.size() - 1);
                auto lastSlashIndex = parentDirectory.lastIndexOf("/");
                parentDirectory.remove(lastSlashIndex + 1,
                                       parentDirectory.size() - lastSlashIndex -
                                         1);
                insertQuery.bind(":parent_dir", parentDirectory.toStdString());
                insertQuery.bind(":path", directoryInDb.toStdString());
                insertQuery.execute();
                insertQuery.reset();
                directoryInDb = std::move(parentDirectory);
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to add {} to parent_dir: {}",
                          directoryInDb.toStdString(),
                          e.what());
        }
    });
}
void
loadChart(QThreadPool& threadPool,
          db::SqliteCppDb& db,
          QString& directoryInDb,
          const std::filesystem::path& path)
{
    auto url =
#if defined(_WIN32)
      QUrl::fromLocalFile(QString::fromStdWString(path.wstring()));
#else
      QUrl::fromLocalFile(QString::fromStdString(path.string()));
#endif
    threadPool.start([&db, url = std::move(url), directoryInDb]() mutable {
        try {
            static thread_local const ChartDataFactory chartDataFactory;
            auto randomGenerator =
              [](charts::parser_models::ParsedBmsChart::RandomRange
                   randomRange) {
                  static thread_local auto randomEngine =
                    std::default_random_engine{ std::random_device{}() };
                  return std::uniform_int_distribution{
                      charts::parser_models::ParsedBmsChart::RandomRange{ 1 },
                      randomRange
                  }(randomEngine);
              };

            auto chartComponents = chartDataFactory.loadChartData(
              url, randomGenerator, directoryInDb);
            chartComponents.chartData->save(db);
        } catch (const std::exception& e) {
            spdlog::error("Failed to load chart data for {}: {}",
                          url.toString().toStdString(),
                          e.what());
        }
    });
}

auto
getParentDirectory(const QString& dir) -> QString
{
    auto lastSlashIndex = dir.lastIndexOf("/");
    auto parentDirectory = dir;
    parentDirectory.remove(lastSlashIndex + 1,
                           parentDirectory.size() - lastSlashIndex - 1);
    return parentDirectory;
}

void
scanFolder(std::filesystem::path directory,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           QString directoryInDb,
           QString parentDirectoryInDb)
{
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        auto path = entry.path();
        if (std::filesystem::is_directory(entry) && !isSongDirectory) {
            directoriesToScan.push_back(path);
        } else if (auto extension = path.extension();
                   extension == ".bms" || extension == ".bme" ||
                   extension == ".bml" || extension == ".pms") {
            if (!isSongDirectory) {
                addDirToParentDirs(threadPool, db, directoryInDb);
            }
            isSongDirectory = true;
            directoriesToScan.clear();
            if (extension == ".pms") {
                continue;
            }
            loadChart(threadPool, db, parentDirectoryInDb, path);
        }
    }
    for (const auto& entry : directoriesToScan) {
        auto nextDirectoryInDb = directoryInDb;
        nextDirectoryInDb += support::pathToQString(entry.filename()) + "/";
        scanFolder(
          entry, threadPool, db, std::move(nextDirectoryInDb), directoryInDb);
    }
}

void
SongDbScanner::scanDirectories(
  std::span<const std::filesystem::path> directories)
{
    auto threadPool = QThreadPool{};
    for (const auto& entry : directories) {
        if (is_directory(entry)) {
            // pass only the last part of the entry directory as
            // directoryInDb
            auto directoryInDb =
              QStringLiteral("/") +
              support::pathToQString(entry.parent_path().filename()) +
              QStringLiteral("/");
            scanFolder(
              entry, threadPool, *db, directoryInDb, QStringLiteral("/"));
        } else {
            spdlog::error("Resource path {} is not a directory",
                          entry.string());
        }
    }
    threadPool.waitForDone();
}
} // namespace resource_managers