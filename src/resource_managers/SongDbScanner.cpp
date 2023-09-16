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
           db::SqliteCppDb& db)
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
              [&db, url = std::move(url)] {
                try {
                    static thread_local const ChartDataFactory chartDataFactory;
                    auto chartComponents = chartDataFactory.loadChartData(url);
                    chartComponents.chartData->save(db);
                } catch (const std::exception& e) {
                    spdlog::error("Failed to load chart data for {}: {}",
                                  url.toString().toStdString(),
                                  e.what());
                }
              }();
        }
    }
    for (const auto& entry : directoriesToScan) {
        [entry, &threadPool, &db] { scanFolder(entry, threadPool, db); }();
    }
}

void
SongDbScanner::scanDirectories(std::span<const std::string> directories)
{
    auto threadPool = QThreadPool{};
    for (const auto& entry : directories) {
        if (std::filesystem::is_directory(entry)) {
            [&] { scanFolder(entry, threadPool, *db); }();
        } else {
            spdlog::error("Resource path {} is not a directory", entry);
        }
    }
    threadPool.waitForDone();
}
} // namespace resource_managers