//
// Created by bobini on 14.09.23.
//

#ifdef _WIN32
#include <Windows.h>
#include <winternl.h>
#include <ntstatus.h>
#else
#include <llfio.hpp>
namespace llfio = LLFIO_V2_NAMESPACE;
#endif
#include <stack>
#include <utility>
#include "SongDbScanner.h"
#include "SongAssetStore.h"
#include "db/SqliteCppDb.h"
#include "support/PathToQString.h"
#include "support/PathToUtfString.h"
#include "support/QStringToPath.h"

#include <algorithm>
#include <cctype>
#include <QHash>
#include <QSet>
#include <qthreadpool.h>
#include <spdlog/stopwatch.h>
#include <spdlog/spdlog.h>

namespace resource_managers {
SongDbScanner::SongDbScanner(db::SqliteCppDb* db, SongAssetStore* assetStore)
  : db(db)
  , assetStore(assetStore)
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
          std::function<void(QString)> updateCurrentScannedFolder,
          std::atomic_bool* stop)
{
    threadPool.start([&db, path, directory, updateCurrentScannedFolder, stop] {
        if (*stop) {
            return;
        }
        try {
            updateCurrentScannedFolder(support::pathToQString(path));
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

            const auto chartComponents = [&] {
                thread_local constexpr ChartDataFactory chartDataFactory;
                if (path.extension() == ".bmson") {
                    return chartDataFactory.loadBmsonChartData(path, directory);
                }
                return chartDataFactory.loadChartData(
                  path, randomGenerator, directory);
            }();
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
loadArchivedChart(QThreadPool& threadPool,
                  db::SqliteCppDb& db,
                  int64_t directory,
                  const std::filesystem::path& virtualPath,
                  QByteArray contents,
                  std::function<void(QString)> updateCurrentScannedFolder,
                  std::atomic_bool* stop)
{
    threadPool.start([&db,
                      virtualPath,
                      directory,
                      contents = std::move(contents),
                      updateCurrentScannedFolder,
                      stop] {
        if (*stop) {
            return;
        }
        try {
            updateCurrentScannedFolder(support::pathToQString(virtualPath));
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
            const auto view =
              std::string_view{ contents.constData(),
                                static_cast<size_t>(contents.size()) };
            thread_local constexpr ChartDataFactory chartDataFactory;
            const auto extension =
              support::pathToQString(virtualPath.extension()).toLower();
            const auto chartComponents =
              extension == QStringLiteral(".bmson")
                ? chartDataFactory.loadBmsonChartData(
                    view, virtualPath, directory)
                : chartDataFactory.loadChartData(
                    view, virtualPath, randomGenerator, directory);
            chartComponents.chartData->save(db);
        } catch (const std::exception& error) {
            spdlog::error("Failed to load archived chart data for {}: {}",
                          support::pathToUtfString(virtualPath),
                          error.what());
        }
    });
}

void
addSongDirectoryFileToDb(db::SqliteCppDb& db,
                         const char* table,
                         const char* label,
                         const std::filesystem::path& directory,
                         const std::filesystem::path& path)
{
    try {
        auto filePath = support::pathToUtfString((path));
        auto directoryPath = support::pathToUtfString((directory / ""));
        auto statement =
          db.createStatement(std::string("INSERT OR REPLACE INTO ") + table +
                             " (path, directory) VALUES (?, ?)");
        statement.reset();
        statement.bind(1, filePath);
        statement.bind(2, directoryPath);
        statement.execute();
    } catch (const std::exception& e) {
        spdlog::error("Failed to add {} file to db: {}", label, e.what());
    }
}

void
addPreviewFileToDb(db::SqliteCppDb& db,
                   const std::filesystem::path& directory,
                   const std::filesystem::path& path)
{
    addSongDirectoryFileToDb(db, "preview_files", "preview", directory, path);
}

void
addReadmeFileToDb(db::SqliteCppDb& db,
                  const std::filesystem::path& directory,
                  const std::filesystem::path& path)
{
    addSongDirectoryFileToDb(db, "readme_files", "readme", directory, path);
}

auto
lowercaseExtension(const std::filesystem::path& path) -> std::string
{
    auto extension = support::pathToUtfString(path.extension());
    std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

auto
fileNameStartsWithDot(const std::filesystem::path& path) -> bool
{
    const auto filename = path.filename();
    if (filename.empty()) {
        return false;
    }

    const auto& native = filename.native();
    return !native.empty() &&
           native.front() ==
             static_cast<std::filesystem::path::value_type>('.');
}

auto
isReadmeCandidate(const std::filesystem::path& path) -> bool
{
    if (fileNameStartsWithDot(path)) {
        return false;
    }
    return lowercaseExtension(path) == ".txt";
}

auto
isChartCandidate(const std::filesystem::path& path) -> bool
{
    const auto extension = lowercaseExtension(path);
    return extension == ".bms" || extension == ".bme" || extension == ".bml" ||
           extension == ".pms" || extension == ".bmson";
}

auto
isPreviewCandidate(const std::filesystem::path& path) -> bool
{
    auto filename = support::pathToUtfString(path.filename());
    std::ranges::transform(
      filename, filename.begin(), [](const unsigned char value) {
          return static_cast<char>(std::tolower(value));
      });
    const auto extension = lowercaseExtension(path);
    return filename.starts_with("preview") &&
           (extension == ".mp3" || extension == ".ogg" || extension == ".wav" ||
            extension == ".flac");
}

void
scanSongArchive(const std::filesystem::path& archivePath,
                QThreadPool& threadPool,
                db::SqliteCppDb& db,
                SongAssetStore& assetStore,
                const QString& root,
                const std::function<void(QString)>& updateCurrentScannedFolder,
                std::atomic_bool* stop)
{
    if (SongAssetStore::isSplitArchivePath(archivePath)) {
        spdlog::warn("Split archive is unsupported: {}",
                     support::pathToUtfString(archivePath));
        return;
    }
    auto chartEntries = std::vector<SongAssetStore::ArchiveEntry>{};
    auto previewPaths = QHash<QString, std::filesystem::path>{};
    auto readmePaths = QHash<QString, std::filesystem::path>{};
    updateCurrentScannedFolder(support::pathToQString(archivePath));
    assetStore.walkArchive(
      archivePath,
      [](const std::filesystem::path& path) {
          return isChartCandidate(path) && lowercaseExtension(path) != ".pms";
      },
      [&](SongAssetStore::ArchiveEntry entry) {
          if (*stop) {
              return;
          }
          const auto directoryPath = entry.virtualPath.parent_path() / "";
          const auto directory = support::pathToQString(directoryPath);
          if (isChartCandidate(entry.virtualPath)) {
              chartEntries.push_back(std::move(entry));
              return;
          }
          if (isPreviewCandidate(entry.virtualPath)) {
              previewPaths.insert(directory, entry.virtualPath);
          } else if (isReadmeCandidate(entry.virtualPath) &&
                     !readmePaths.contains(directory)) {
              readmePaths.insert(directory, entry.virtualPath);
          }
      },
      stop);

    auto allChartDirectories = QSet<QString>{};
    for (const auto& entry : chartEntries) {
        allChartDirectories.insert(
          support::pathToQString(entry.virtualPath.parent_path() / ""));
    }
    auto candidates = allChartDirectories.values();
    std::ranges::sort(candidates, [](const auto& left, const auto& right) {
        return left.size() < right.size();
    });
    auto chartDirectories = QSet<QString>{};
    for (const auto& candidate : candidates) {
        const auto belowSongDirectory =
          std::ranges::any_of(chartDirectories, [&](const auto& parent) {
              return candidate.startsWith(parent, Qt::CaseInsensitive);
          });
        if (!belowSongDirectory) {
            chartDirectories.insert(candidate);
        }
    }

    for (auto& entry : chartEntries) {
        const auto directory =
          support::pathToQString(entry.virtualPath.parent_path() / "");
        if (!chartDirectories.contains(directory)) {
            continue;
        }
        auto listingDirectory = support::pathToQString(
          entry.virtualPath.parent_path().parent_path() / "");
        if (!listingDirectory.startsWith(root)) {
            listingDirectory = root;
        }
        const auto directoryId = addDirToParentDirs(db, root, listingDirectory);
        if (lowercaseExtension(entry.virtualPath) != ".pms" && entry.contents) {
            loadArchivedChart(threadPool,
                              db,
                              directoryId,
                              entry.virtualPath,
                              std::move(*entry.contents),
                              updateCurrentScannedFolder,
                              stop);
        }
    }

    for (const auto& directory : chartDirectories) {
        const auto directoryPath = support::qStringToPath(directory);
        if (const auto preview = previewPaths.constFind(directory);
            preview != previewPaths.cend()) {
            addPreviewFileToDb(db, directoryPath, *preview);
        }
        if (const auto readme = readmePaths.constFind(directory);
            readme != readmePaths.cend()) {
            addReadmeFileToDb(db, directoryPath, *readme);
        }
    }

    threadPool.waitForDone();
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
           SongAssetStore& assetStore,
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

    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto archivesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    auto parentDirQString = support::pathToQString(parentDirectory);
    if (!parentDirQString.isEmpty()) {
        if (parentDirQString.back() != '/') {
            parentDirQString += '/';
        }
    }
    auto previewPath = std::filesystem::path{};
    auto readmePath = std::filesystem::path{};
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
                       extension.compare(".pms") == 0 ||
                       extension.compare(".bmson") == 0) {
                const auto chartPath = (directory / path).lexically_normal();
                if (!isSongDirectory) {
                    dirId = addDirToParentDirs(db, root, parentDirQString);
                    isSongDirectory = true;
                }
                directoriesToScan.clear();
                if (extension.compare(".pms") != 0) {
                    loadChart(threadPool,
                              db,
                              dirId,
                              chartPath,
                              updateCurrentScannedFolder,
                              stop);
                }
            } else if (SongAssetStore::isArchivePath(
                         std::filesystem::path(path)) ||
                       SongAssetStore::isSplitArchivePath(
                         std::filesystem::path(path))) {
                archivesToScan.push_back(directory / path);
            } else if (path.starts_with(L"preview") &&
                       (extension.compare(".mp3") == 0 ||
                        extension.compare(".ogg") == 0 ||
                        extension.compare(".wav") == 0 ||
                        extension.compare(".flac") == 0)) {
                previewPath = directory / path;
            } else if (readmePath.empty() &&
                       isReadmeCandidate(std::filesystem::path(path))) {
                readmePath = directory / path;
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
    if (!readmePath.empty() && isSongDirectory) {
        threadPool.start([&db, directory, readmePath] {
            addReadmeFileToDb(db, directory, readmePath);
        });
    }
    for (const auto& archive : archivesToScan) {
        if (*stop) {
            break;
        }
        try {
            scanSongArchive(archive,
                            threadPool,
                            db,
                            assetStore,
                            root,
                            updateCurrentScannedFolder,
                            stop);
        } catch (const std::exception& error) {
            spdlog::error("Failed to scan song archive {}: {}",
                          support::pathToUtfString(archive),
                          error.what());
        }
    }
    for (const auto& entry : directoriesToScan) {
        if (*stop) {
            break;
        }
        scanFolder(directory / entry,
                   directory,
                   threadPool,
                   db,
                   assetStore,
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
           SongAssetStore& assetStore,
           const QString& root,
           const std::function<void(QString)>& updateCurrentScannedFolder,
           std::vector<llfio::directory_handle::buffer_type>& buffer,
           std::atomic_bool* stop)
{
    auto directoriesToScan = std::vector<std::filesystem::path>{};
    auto archivesToScan = std::vector<std::filesystem::path>{};
    auto isSongDirectory = false;
    auto previewPath = std::filesystem::path{};
    auto readmePath = std::filesystem::path{};
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
            const auto chartPath = directory / path;
            if (!isSongDirectory) {
                dirId = addDirToParentDirs(db, root, parentDirQString);
                isSongDirectory = true;
            }
            directoriesToScan.clear();
            if (extension.compare(".pms") != 0) {
                loadChart(threadPool,
                          db,
                          dirId,
                          chartPath,
                          updateCurrentScannedFolder,
                          stop);
            }
        } else if (SongAssetStore::isArchivePath(path) ||
                   SongAssetStore::isSplitArchivePath(path)) {
            archivesToScan.push_back(directory / path);
        } else if (path.string().starts_with("preview") &&
                   (extension == ".mp3" || extension == ".ogg" ||
                    extension == ".wav" || extension == ".flac")) {
            previewPath = directory / path;
        } else if (readmePath.empty() && isReadmeCandidate(path)) {
            readmePath = directory / path;
        }
    }
    if (!previewPath.empty() && isSongDirectory) {
        threadPool.start([&db, directory, previewPath] {
            addPreviewFileToDb(db, directory, previewPath);
        });
    }
    if (!readmePath.empty() && isSongDirectory) {
        threadPool.start([&db, directory, readmePath] {
            addReadmeFileToDb(db, directory, readmePath);
        });
    }
    for (const auto& archive : archivesToScan) {
        if (*stop) {
            break;
        }
        try {
            scanSongArchive(archive,
                            threadPool,
                            db,
                            assetStore,
                            root,
                            updateCurrentScannedFolder,
                            stop);
        } catch (const std::exception& error) {
            spdlog::error("Failed to scan song archive {}: {}",
                          support::pathToUtfString(archive),
                          error.what());
        }
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
                   assetStore,
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
                       *assetStore,
                       root,
                       updateCurrentScannedFolder,
#ifndef _WIN32
                       buffer,
#endif
                       stop);
        } else if (is_regular_file(directory) &&
                   SongAssetStore::isArchivePath(directory) &&
                   !SongAssetStore::isSplitArchivePath(directory)) {
            auto root = support::pathToQString(directory);
            if (!root.endsWith('/')) {
                root += '/';
            }
            scanSongArchive(directory,
                            threadPool,
                            *db,
                            *assetStore,
                            root,
                            updateCurrentScannedFolder,
                            stop);
        } else {
            spdlog::error(
              "Resource path {} is not a directory or supported song archive",
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
