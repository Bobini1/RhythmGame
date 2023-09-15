//
// Created by bobini on 14.09.23.
//

#include <stack>
#include <iostream>
#include "SongDbScanner.h"

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db)
  : db(db)
{
}

void
SongDbScanner::scanDirectories(std::span<const std::string> directories)
{
    struct DirStatus
    {
        std::vector<std::filesystem::path> directoriesToScan;
        bool isSongDirectory = false;
    };
    auto directoriesToScan = std::stack<DirStatus>{};
    // create a stack of directories to scan
    for (const auto& entry : directories) {
        if (std::filesystem::is_directory(entry)) {
            directoriesToScan.push({ { entry }, false });
        } else {
            spdlog::error("Resource path {} is not a directory", entry);
        }
    }
    // create a local thread pool
    auto threadPool = QThreadPool{};

    // scan directories
    while (!directoriesToScan.empty()) {
        auto& dirStatus = directoriesToScan.top();
        if (dirStatus.directoriesToScan.empty()) {
            directoriesToScan.pop();
            continue;
        }
        auto directory = std::move(dirStatus.directoriesToScan.back());
        dirStatus.directoriesToScan.pop_back();
        directoriesToScan.emplace();
        auto& newDirStatus = directoriesToScan.top();
        for (const auto& entry :
             std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_directory(entry) &&
                !dirStatus.isSongDirectory) {
                newDirStatus.directoriesToScan.push_back(entry.path());
            } else if (entry.path().extension() == ".bms" ||
                       entry.path().extension() == ".bme" ||
                       entry.path().extension() == ".bml" ||
                       entry.path().extension() == ".pms") {
                dirStatus.isSongDirectory = true;
                newDirStatus.directoriesToScan.clear();
                if (entry.path().extension() == ".pms") {
                    continue;
                }
                auto url = QUrl::fromLocalFile(
                  QString::fromStdString(entry.path().string()));
                threadPool.start([this, url = std::move(url)] {
                    static thread_local ChartDataFactory chartDataFactory;
                    try {
                        auto chartComponents =
                          chartDataFactory.loadChartData(url);
                        chartComponents.chartData->save(*db);
                        std::cout << url.toString().toStdString() << std::endl;
                    } catch (const std::exception& e) {
                        //                        spdlog::error("Error while
                        //                        loading chart {}: {}",
                        //                                      url.toString().toStdString(),
                        //                                      e.what());
                    }
                });
            }
        }
    }
}
} // resource_managers