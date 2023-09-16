//
// Created by bobini on 14.09.23.
//

#include <stack>
#include <iostream>
#include "SongDbScanner.h"
#include <boost/lockfree/stack.hpp>
#include "db/SqliteCppDb.h"

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
              QUrl::fromLocalFile(QString::fromStdString(path.string()));
            threadPool.start(
              [&db, url = std::move(url)] {
                  static thread_local const ChartDataFactory chartDataFactory;
                  try {
                      auto chartComponents =
                        chartDataFactory.loadChartData(url);
                      chartComponents.chartData->save(db);
                  } catch (const std::exception& e) {
                      //                spdlog::error("Error while loading chart
                      //                {}:
                      //                {}",
                      //                              url.toString().toStdString(),
                      //                              e.what());
                  }
              },
              QThread::HighPriority);
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
            threadPool.start([&] { scanFolder(entry, threadPool, *db); });
        } else {
            spdlog::error("Resource path {} is not a directory", entry);
        }
    }
    threadPool.waitForDone();
}
} // namespace resource_managers