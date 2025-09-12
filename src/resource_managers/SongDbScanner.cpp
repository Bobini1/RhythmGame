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
#ifdef _WIN32
#include <Windows.h>
#include <winternl.h>
#include <ntstatus.h>
#else
#include <llfio.hpp>
namespace llfio = LLFIO_V2_NAMESPACE;
#endif
#include <spdlog/spdlog.h>

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db)
  : db(db)
{
}

int64_t
addDirToParentDirs(db::SqliteCppDb& db, QString root, QString folder)
{
    auto insert = db.createStatement("INSERT OR IGNORE INTO parent_dir "
                                     "(parent_dir, dir) VALUES (:parent_dir, "
                                     ":dir)");
    if (folder.isEmpty()) {
        return -1;
    }
    if (folder.back() != '/') {
        folder += '/';
    }
    auto parent = std::string{};
    auto current = root;
    auto rest = folder.right(folder.size() - root.size());
    try {
        while (true) {
            insert.reset();
            if (parent.empty()) {
                insert.bind(":parent_dir");
            } else {
                insert.bind(std::string(":parent_dir"), parent);
            }
            insert.bind(":dir", parent = current.toStdString());
            insert.execute();
            if (current == folder || folder.isEmpty()) {
                break;
            }
            current = current + rest.left(rest.indexOf('/') + 1);
            rest = rest.right(rest.size() - rest.indexOf('/') - 1);
        }
        auto getIdQuery =
          db.createStatement("SELECT id FROM parent_dir WHERE dir = :dir");
        getIdQuery.bind(":dir", folder.toStdString());
        return getIdQuery.executeAndGet<int64_t>().value();
    } catch (const std::exception& e) {
        spdlog::error("Failed to add directory to parent dirs: {}", e.what());
        return -1;
    }
}
void
loadChart(QThreadPool& threadPool,
          db::SqliteCppDb& db,
          int64_t directory,
          const std::filesystem::path& path,
          std::atomic_bool* stop)
{
    threadPool.start([&db, path, directory, stop] {
        if (*stop) {
            return;
        }
        try {
            thread_local constexpr ChartDataFactory chartDataFactory;
            auto randomGenerator =
              [](charts::ParsedBmsChart::RandomRange randomRange) {
                  thread_local auto randomEngine =
                    std::default_random_engine{ std::random_device{}() };
                  if (randomRange <= 1) {
                      return charts::ParsedBmsChart::RandomRange{ 1 };
                  }
                  return std::uniform_int_distribution{
                      charts::ParsedBmsChart::RandomRange{ 1 }, randomRange
                  }(randomEngine);
              };

            const auto chartComponents =
              chartDataFactory.loadChartData(path, randomGenerator, directory);
            chartComponents.chartData->save(db);
            // ChartDataFactory::makeNotes(chartComponents.notesData.notes,
            //                             chartComponents.notesData.bpmChanges,
            //                             chartComponents.notesData.barLines)
            //   ->save(db,
            //   chartComponents.chartData->getSha256().toStdString());
        } catch (const std::exception& e) {
            try {
                spdlog::error("Failed to load chart data for {}: {}",
                              path.string(),
                              e.what());
            } catch (const std::exception& e2) {
                spdlog::error("Failed to load chart data for ({}): {}",
                              e2.what(),
                              e.what());
            }
        }
    });
}

void
addPreviewFileToDb(db::SqliteCppDb& db,
                   const std::filesystem::path& directory,
                   const std::filesystem::path& path)
{
    try {
        auto previewPath = support::pathToUtfString((path));
        auto directoryPath = support::pathToUtfString((directory / ""));
        auto statement = db.createStatement(
          "INSERT OR REPLACE INTO preview_files (path, directory) "
          "VALUES (?, ?)");
        statement.reset();
        statement.bind(1, previewPath);
        statement.bind(2, directoryPath);
        statement.execute();
    } catch (const std::exception& e) {
        spdlog::error("Failed to add preview file to db: {}", e.what());
    }
}
#ifdef _WIN32
using NtQueryDirectoryFile_t =
  NTSTATUS(NTAPI*)(_In_ HANDLE FileHandle,
                   _In_opt_ HANDLE Event,
                   _In_opt_ PIO_APC_ROUTINE ApcRoutine,
                   _In_opt_ PVOID ApcContext,
                   _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                   _Out_ PVOID FileInformation,
                   _In_ ULONG Length,
                   _In_ FILE_INFORMATION_CLASS FileInformationClass,
                   _In_ BOOLEAN ReturnSingleEntry,
                   _In_opt_ PUNICODE_STRING FileName,
                   _In_ BOOLEAN RestartScan);

typedef struct _FILE_DIRECTORY_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

void
scanFolder(std::filesystem::path directory,
           std::filesystem::path parentDirectory,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           const QString& root,
           std::function<void(QString)> updateCurrentScannedFolder,
           std::atomic_bool* stop)
{
    static HMODULE ntdllh = GetModuleHandleA("NTDLL.DLL");
    static auto NtQueryDirectoryFile = reinterpret_cast<NtQueryDirectoryFile_t>(
      GetProcAddress(ntdllh, "NtQueryDirectoryFile"));
    auto isb = IO_STATUS_BLOCK{};
    memset(&isb, 0, sizeof(isb));
    isb.Status = -1;
    static constexpr auto max_bytes = 65536;
    char buffer[65536];
    HANDLE hDirectory = CreateFileW(directory.c_str(),
                                    FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);

    updateCurrentScannedFolder(
      support::pathToQString(directory.lexically_normal()));
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    auto parentDirQString = support::pathToQString(parentDirectory);
    if (!parentDirQString.isEmpty()) {
        if (parentDirQString.back() != '/') {
            parentDirQString += '/';
        }
    }
    auto previewPath = std::filesystem::path{};
    auto dirId = int64_t{ 0 };

    while (true) {
        auto status = NtQueryDirectoryFile(hDirectory,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &isb,
                                           buffer,
                                           sizeof(buffer),
                                           FileDirectoryInformation,
                                           FALSE,
                                           NULL,
                                           false);

        if (status == STATUS_NO_MORE_FILES) {
            break; // No more files to process
        }

        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) {
            spdlog::error("NtQueryDirectoryFile failed. NTSTATUS: {:#x}\n",
                          status);
            break;
        }

        if (*stop) {
            break;
        }

        auto* fileInfo = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(buffer);

        do {
            auto path =
              std::wstring_view{ fileInfo->FileName,
                                 fileInfo->FileNameLength / sizeof(WCHAR) };
            if (fileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                path != L"." && path != L"..") {
                if (!isSongDirectory) {
                    directoriesToScan.emplace_back(path);
                }
            } else if (auto extension = std::filesystem::path(path).extension();
                       extension.compare(".bms") == 0 ||
                       extension.compare(".bme") == 0 ||
                       extension.compare(".bml") == 0 ||
                       extension.compare(".pms") == 0) {
                if (!isSongDirectory) {
                    dirId = addDirToParentDirs(db, root, parentDirQString);
                    isSongDirectory = true;
                }
                directoriesToScan.clear();
                if (extension.compare(".pms") != 0) {
                    loadChart(threadPool, db, dirId, directory / path, stop);
                }
            } else if (path.starts_with(L"preview") &&
                       (extension.compare(".mp3") == 0 ||
                        extension.compare(".ogg") == 0 ||
                        extension.compare(".wav") == 0 ||
                        extension.compare(".flac") == 0)) {
                previewPath = directory / path;
            }

            if (fileInfo->NextEntryOffset == 0) {
                break;
            }
            fileInfo = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(
              reinterpret_cast<BYTE*>(fileInfo) + fileInfo->NextEntryOffset);
        } while (true);
    }
    if (!previewPath.empty() && isSongDirectory) {
        threadPool.start([&db, directory, previewPath] {
            addPreviewFileToDb(db, directory, previewPath);
        });
    }
    for (const auto& entry : directoriesToScan) {
        if (*stop) {
            break;
        }
        scanFolder(directory / entry,
                   directory,
                   threadPool,
                   db,
                   root,
                   updateCurrentScannedFolder,
                   stop);
    }
}
#else
void
scanFolder(const std::filesystem::path& directory,
           const std::filesystem::path& parentDirectory,
           QThreadPool& threadPool,
           db::SqliteCppDb& db,
           const QString& root,
           const std::function<void(QString)>& updateCurrentScannedFolder,
           std::vector<llfio::directory_handle::buffer_type>& buffer,
           std::atomic_bool* stop)
{
    updateCurrentScannedFolder(support::pathToQString(directory));
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    auto previewPath = std::filesystem::path{};
    auto parentDirQString = support::pathToQString(parentDirectory);
    auto dirId = int64_t{ 0 };
    auto dh = llfio::directory( //
                {},             // path_handle to base directory
                directory       // path_view to path fragment relative to base
                                // directory default mode is read only default
                // creation is open existing default caching is all
                // default flags is none
                )
                .value(); // If failed, throw a filesystem_error exception

    // Very similar to reading from a file handle, we need
    // to achieve a single snapshot read to be race free.
    buffer.resize(buffer.capacity());
    auto entries = llfio::directory_handle::buffers_type{ std::span(buffer) };
    for (;;) {
        entries = dh.read({ std::move(entries) } // buffers to fill
                          )
                    .value(); // If failed, throw a filesystem_error exception

        // If there were fewer entries in the directory than buffers
        // passed in, we are done.
        if (entries.done()) {
            break;
        }
        // Otherwise double the size of the buffer
        buffer.resize(buffer.size() << 1);
        // Set the next read attempt to use the newly enlarged buffer.
        // buffers_type may cache internally reusable state depending
        // on platform, to efficiently reuse that state pass in the
        // old entries by rvalue ref.
        entries = { std::span(buffer), std::move(entries) };
    }

    for (const auto& entry : entries) {
        if (*stop) {
            break;
        }
        auto path = entry.leafname.path();
        if (entry.stat.st_type == llfio::filesystem::file_type::directory &&
            !isSongDirectory) {
            directoriesToScan.push_back(directory / path);
        } else if (const auto extension = path.extension();
                   extension == ".bms" || extension == ".bme" ||
                   extension == ".bml" || extension == ".pms") {
            if (!isSongDirectory) {
                dirId = addDirToParentDirs(db, root, parentDirQString);
                isSongDirectory = true;
            }
            directoriesToScan.clear();
            if (extension.compare(".pms") != 0) {
                loadChart(threadPool, db, dirId, directory / path, stop);
            }
        } else if (path.string().starts_with("preview") &&
                   (extension == ".mp3" || extension == ".ogg" ||
                    extension == ".wav" || extension == ".flac")) {
            previewPath = directory / path;
        }
    }
    if (!previewPath.empty() && isSongDirectory) {
        threadPool.start([&db, directory, previewPath] {
            addPreviewFileToDb(db, directory, previewPath);
        });
    }
    for (const auto& entry : directoriesToScan) {
        if (*stop) {
            break;
        }
        buffer.clear();
        scanFolder(entry,
                   directory,
                   threadPool,
                   db,
                   root,
                   updateCurrentScannedFolder,
                   buffer,
                   stop);
    }
}
#endif

void
SongDbScanner::scanDirectory(
  const std::filesystem::path& directory,
  const std::function<void(QString)>& updateCurrentScannedFolder,
  std::atomic_bool* stop) const
{
    auto sw = spdlog::stopwatch{};
    auto threadPool = QThreadPool{};
    try {
#ifndef _WIN32
        auto buffer = std::vector<llfio::directory_handle::buffer_type>(100);
#endif
        if (is_directory(directory)) {
            const auto root = support::pathToQString(directory);
            scanFolder(directory,
                       {},
                       threadPool,
                       *db,
                       root,
                       updateCurrentScannedFolder,
#ifndef _WIN32
                       buffer,
#endif
                       stop);
        } else {
            spdlog::error("Resource path {} is not a directory",
                          directory.string());
        }
    } catch (const std::exception& e) {
        spdlog::error(
          "Error scanning directory {}: {}", directory.string(), e.what());
    }
    threadPool.waitForDone();
    try {
        if (*stop) {
            spdlog::info(
              "Scanning {} cancelled after {} seconds", directory.string(), sw);
        } else {
            spdlog::info("Scanning {} took {} seconds", directory.string(), sw);
        }
    } catch (...) {
    }
    updateCurrentScannedFolder("");
}
} // namespace resource_managers