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
#include <spdlog/stopwatch.h>
#include <llfio.hpp>

namespace llfio = LLFIO_V2_NAMESPACE;

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
            if (current == folder || folder.isEmpty()) {
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
           llfio::directory_handle dirHandle,
           std::filesystem::path parentDirectory,
           std::vector<llfio::directory_handle::buffer_type>& buffer,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           const QString& root,
           std::function<void(QString)> updateCurrentScannedFolder,
           std::atomic_bool* stop)
{
    updateCurrentScannedFolder(support::pathToQString(directory));
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;

    // Very similar to reading from a file handle, we need
    // to achieve a single snapshot read to be race free.
    llfio::directory_handle::buffers_type entries(buffer);
    //auto NtQueryDirectoryFile = reinterpret_cast<NtQueryDirectoryFile_t>(GetProcAddress(ntdllh, "NtQueryDirectoryFile"))
    for(;;)
    {
        //NtQueryDirectoryFile()
        entries = dirHandle.read(
                      {std::move(entries)}   // buffers to fill
                      ).value();               // If failed, throw a filesystem_error exception

        // If there were fewer entries in the directory than buffers
        // passed in, we are done.
        if(entries.done())
        {
            break;
        }
        // Otherwise double the size of the buffer
        buffer.resize(buffer.size() << 1);
        // Set the next read attempt to use the newly enlarged buffer.
        // buffers_type may cache internally reusable state depending
        // on platform, to efficiently reuse that state pass in the
        // old entries by rvalue ref.
        entries = { buffer, std::move(entries) };
    }

    for (const auto& entry : entries) {
        if (*stop) {
            break;
        }
        const auto& path = entry.leafname;
        if (entry.stat.st_type == llfio::filesystem::file_type::directory && !isSongDirectory) {
            directoriesToScan.push_back(path.path());
        } else if (auto extension = path.extension();
                   extension.compare(".bms") == 0 || extension.compare(".bme") == 0  ||
                   extension.compare(".bml") == 0  || extension.compare(".pms") == 0 ) {
            isSongDirectory = true;
            directoriesToScan.clear();
            if (extension.compare(".pms") == 0) {
                continue;
            }
            loadChart(
              threadPool, db, support::pathToQString(parentDirectory), directory / path);
            // converting to string should not break stuff even on windows
            // in this case
        } else if (auto pathCopy = path.path(); pathCopy.filename().string().starts_with("preview") &&
                   (extension.compare(".mp3") == 0 || extension.compare(".ogg") == 0  ||
                    extension.compare(".wav") == 0  || extension.compare(".flac") == 0 )) {
            threadPool.start([&db, directory, pathCopy = std::move(pathCopy)] {
                addPreviewFileToDb(db, directory, pathCopy);
            });
        }
    }
    for (const auto& entry : directoriesToScan) {
        if (*stop) {
            break;
        }
        auto entryHandle = llfio::directory(dirHandle, entry).value();
        scanFolder(directory / entry,
                   std::move(entryHandle),
                   directory,
                   buffer,
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
    auto sw = spdlog::stopwatch{};
    auto threadPool = QThreadPool{};
    try {
        if (is_directory(directory)) {
            const auto root = support::pathToQString(directory);
            auto p = llfio::path(directory).value();
            auto entryHandle = llfio::directory(p, "").value();

            // Read up to ten directory_entry
            std::vector<llfio::directory_handle::buffer_type> buffer(20000);
            scanFolder(directory,
                       std::move(entryHandle),
                       {},
                       buffer,
                       threadPool,
                       *db,
                       root,
                       updateCurrentScannedFolder,
                       stop);
        } else {
            spdlog::error("Resource path {} is not a directory",
                          directory.string());
        }
    } catch (const std::exception& e) {
        spdlog::error("Error scanning directory {}: {}", directory.string(), e.what());
    }
    threadPool.waitForDone();
    spdlog::info("Scanning {} took {}", directory.string(), sw);
    updateCurrentScannedFolder("");
}
} // namespace resource_managers