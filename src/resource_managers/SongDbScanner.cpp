//
// Created by bobini on 14.09.23.
//

#include <stack>
#include <iostream>
#include "SongDbScanner.h"
#include <boost/lockfree/stack.hpp>
#include "db/SqliteCppDb.h"
#include <fmt/xchar.h>

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db)
  : db(db)
{
}

void
scanFolder(std::filesystem::path directory,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           QString directoryInDb)
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
            isSongDirectory = true;
            directoriesToScan.clear();
            if (extension == ".pms") {
                continue;
            }
            auto url =
#if defined(_WIN32)
              QUrl::fromLocalFile(QString::fromStdWString(path.wstring()));
#else
              QUrl::fromLocalFile(QString::fromStdString(path.string()));
#endif
            threadPool.start([&db,
                              url = std::move(url),
                              directoryInDb]() mutable {
                try {
                    static thread_local const ChartDataFactory chartDataFactory;
                    auto randomGenerator = [](charts::parser_models::
                                                ParsedBmsChart::RandomRange
                                                  randomRange) {
                        static thread_local auto randomEngine =
                          std::default_random_engine{ std::random_device{}() };
                        return std::uniform_int_distribution{
                            charts::parser_models::ParsedBmsChart::RandomRange{
                              1 },
                            randomRange
                        }(randomEngine);
                    };
                    // remove the last part of directoryInDb
                    directoryInDb.remove(directoryInDb.size() - 1, 1);
                    auto lastSlashIndex = directoryInDb.lastIndexOf("/");
                    directoryInDb.remove(lastSlashIndex + 1,
                                         directoryInDb.size() - lastSlashIndex -
                                           1);
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
    }
    for (const auto& entry : directoriesToScan) {
        auto nextDirectoryInDb = directoryInDb;
        nextDirectoryInDb +=
          QString::fromStdString(entry.filename().string() + "/");
        scanFolder(entry, threadPool, db, std::move(nextDirectoryInDb));
    }
}

void
SongDbScanner::scanDirectories(std::span<const std::string> directories)
{
    auto threadPool = QThreadPool{};
    for (const auto& entry : directories) {
        if (std::filesystem::is_directory(entry)) {
            // pass only the last part of the entry directory as directoryInDb
            auto directoryInDb = QString::fromStdString(
              "/" + std::filesystem::path(entry).filename().string() + "/");
            scanFolder(entry, threadPool, *db, directoryInDb);
        } else {
            spdlog::error("Resource path {} is not a directory", entry);
        }
    }
    threadPool.waitForDone();
}
} // namespace resource_managers