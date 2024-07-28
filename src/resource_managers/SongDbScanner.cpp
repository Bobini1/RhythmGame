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
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#endif
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
#ifdef _WIN32
using NtQueryDirectoryFile_t = NTSTATUS(NTAPI *)(_In_ HANDLE FileHandle, _In_opt_ HANDLE Event, _In_opt_ PIO_APC_ROUTINE ApcRoutine,
                                                _In_opt_ PVOID ApcContext, _Out_ PIO_STATUS_BLOCK IoStatusBlock, _Out_ PVOID FileInformation,
                                                _In_ ULONG Length, _In_ FILE_INFORMATION_CLASS FileInformationClass, _In_ BOOLEAN ReturnSingleEntry,
                                                _In_opt_ PUNICODE_STRING FileName, _In_ BOOLEAN RestartScan);

typedef struct _FILE_DIRECTORY_INFORMATION {
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
    static auto NtQueryDirectoryFile = reinterpret_cast<NtQueryDirectoryFile_t>(GetProcAddress(ntdllh, "NtQueryDirectoryFile"));
    auto isb = IO_STATUS_BLOCK{};
    memset(&isb, 0, sizeof(isb));
    isb.Status = -1;
    static constexpr auto max_bytes = 65536;
    char buffer[65536];
    HANDLE hDirectory = CreateFileW(
      directory.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS,
      NULL
    );


    updateCurrentScannedFolder(support::pathToQString(directory));
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;

    while (true) {
        auto status = NtQueryDirectoryFile(
          hDirectory,
          NULL,
          NULL,
          NULL,
          &isb,
          buffer,
          sizeof(buffer),
          FileDirectoryInformation,
          FALSE,
          NULL,
          false
        );

        if (status == STATUS_NO_MORE_FILES) {
            break; // No more files to process
        }

        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) {
            spdlog::error("NtQueryDirectoryFile failed. NTSTATUS: {:#x}\n", status);
            break;
        }

        if (*stop) {
            break;
        }

        auto* fileInfo = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(buffer);

        do {
            auto path = std::wstring_view{fileInfo->FileName, fileInfo->FileNameLength / sizeof(WCHAR)};
            if (fileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY && path != L"." && path != L"..") {
                if (!isSongDirectory) {
                    directoriesToScan.emplace_back(path);
                }
            } else if (auto extension = llfio::path_view(path).extension();
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
            } else if (path.starts_with(L"preview") &&
                                                    (extension.compare(".mp3") == 0 || extension.compare(".ogg") == 0  ||
                                                     extension.compare(".wav") == 0  || extension.compare(".flac") == 0 )) {
                threadPool.start([&db, directory, path = std::filesystem::path(path)] {
                    addPreviewFileToDb(db, directory, path);
                });
            }

            if (fileInfo->NextEntryOffset == 0) {
                break;
            }
            fileInfo = (PFILE_DIRECTORY_INFORMATION)((BYTE*)fileInfo + fileInfo->NextEntryOffset);
        } while (true);
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
    if (isSongDirectory) {
        addDirToParentDirs(
          threadPool, db, root, support::pathToQString(parentDirectory));
    }
}
#else
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
#endif

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

            // Read up to ten directory_entry
            std::vector<llfio::directory_handle::buffer_type> buffer(20000);
            scanFolder(directory,
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
    } catch (const std::exception& e) {
        spdlog::error("Error scanning directory {}: {}", directory.string(), e.what());
    }
    threadPool.waitForDone();
    spdlog::info("Scanning {} took {}", directory.string(), sw);
    updateCurrentScannedFolder("");
}
} // namespace resource_managers