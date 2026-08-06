#include "SongAssetStore.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <archive.h>
#include <archive_entry.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>
#include <array>
#include <deque>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <system_error>
#include <unordered_set>

#include <spdlog/spdlog.h>

namespace resource_managers {
namespace {

constexpr auto archiveReadBlockSize = size_t{ 1024 * 64 };

void
throwIfCancelled(const std::atomic_bool* stop)
{
    if (stop && stop->load()) {
        throw std::runtime_error("Song asset request cancelled");
    }
}

struct ArchiveDeleter
{
    void operator()(archive* value) const
    {
        if (value) {
            archive_read_free(value);
        }
    }
};

using ArchiveReader = std::unique_ptr<archive, ArchiveDeleter>;

auto
archiveError(archive* reader, const QString& prefix) -> std::runtime_error
{
    const auto* detail = archive_error_string(reader);
    return std::runtime_error(QStringLiteral("%1: %2")
                                .arg(prefix,
                                     detail ? QString::fromUtf8(detail)
                                            : QStringLiteral("unknown error"))
                                .toStdString());
}

auto
openArchive(const std::filesystem::path& path) -> ArchiveReader
{
    const auto supportError = SongAssetStore::archiveSupportError(path);
    if (!supportError.isEmpty()) {
        throw std::runtime_error(supportError.toStdString());
    }
    auto reader = ArchiveReader{ archive_read_new() };
    if (!reader) {
        throw std::runtime_error("Could not allocate archive reader");
    }
    archive_read_support_filter_all(reader.get());
    archive_read_support_format_all(reader.get());
    // BMS packages frequently use legacy Japanese ZIP names without a
    // Unicode flag. Libarchive still honors explicit Unicode metadata when it
    // is present.
    archive_read_set_format_option(reader.get(), "zip", "hdrcharset", "CP932");
#ifdef _WIN32
    const auto result = archive_read_open_filename_w(
      reader.get(), path.c_str(), archiveReadBlockSize);
#else
    const auto result = archive_read_open_filename(
      reader.get(), path.c_str(), archiveReadBlockSize);
#endif
    if (result != ARCHIVE_OK) {
        throw archiveError(reader.get(),
                           QStringLiteral("Could not open archive %1")
                             .arg(support::pathToQString(path)));
    }
    return reader;
}

auto
normalizeArchivePath(QString path) -> QString
{
    path.replace('\\', '/');
    while (path.startsWith('/')) {
        path.remove(0, 1);
    }
    path = QDir::cleanPath(path);
    if (path == QStringLiteral(".") || path.isEmpty()) {
        return {};
    }
    if (path == QStringLiteral("..") ||
        path.startsWith(QStringLiteral("../")) ||
        path.contains(QStringLiteral("/../")) ||
        (path.size() >= 2 && path[1] == u':')) {
        return {};
    }
    return path;
}

auto
entryPath(archive_entry* entry) -> QString
{
    const auto* value = archive_entry_pathname_utf8(entry);
    if (!value) {
        value = archive_entry_pathname(entry);
    }
    return value ? normalizeArchivePath(QString::fromUtf8(value)) : QString{};
}

auto
normalizedVirtualPath(const std::filesystem::path& path) -> QString
{
    auto value = support::pathToQString(path);
    value.replace('\\', '/');
    return QDir::cleanPath(value);
}

auto
normalizedPhysicalPath(const std::filesystem::path& path) -> QString
{
    std::error_code ec;
    auto physical = std::filesystem::absolute(path, ec).lexically_normal();
    if (ec) {
        physical = path.lexically_normal();
    }
    auto value = normalizedVirtualPath(physical);
#ifdef _WIN32
    value = value.toCaseFolded();
#endif
    return value;
}

auto
joinVirtual(QString directory, QString relative) -> QString
{
    directory.replace('\\', '/');
    relative.replace('\\', '/');
    if (relative.isEmpty()) {
        return QDir::cleanPath(directory);
    }
    while (relative.startsWith('/')) {
        relative.remove(0, 1);
    }
    if (!directory.endsWith('/')) {
        directory += '/';
    }
    return QDir::cleanPath(directory + relative);
}

auto
extensionForMaterialization(const QString& virtualPath) -> QString
{
    auto suffix = QFileInfo(virtualPath).suffix().toLower();
    if (suffix.isEmpty() || suffix.size() > 12 ||
        !std::ranges::all_of(
          suffix, [](const QChar value) { return value.isLetterOrNumber(); })) {
        return {};
    }
    return QStringLiteral(".") + suffix;
}

auto
archiveExtensions() -> const QStringList&
{
    static const auto extensions = QStringList{
        QStringLiteral(".zip"),  QStringLiteral(".7z"),  QStringLiteral(".rar"),
        QStringLiteral(".tar"),  QStringLiteral(".tgz"), QStringLiteral(".tbz"),
        QStringLiteral(".tbz2"), QStringLiteral(".txz"), QStringLiteral(".lha"),
        QStringLiteral(".lzh"),  QStringLiteral(".cab"), QStringLiteral(".xar"),
        QStringLiteral(".cpio"), QStringLiteral(".iso"), QStringLiteral(".ar"),
        QStringLiteral(".gz"),   QStringLiteral(".bz2"), QStringLiteral(".xz"),
        QStringLiteral(".zst")
    };
    return extensions;
}

auto
supportedArchiveExtensions() -> const QStringList&
{
    static const auto extensions = QStringList{ QStringLiteral(".zip") };
    return extensions;
}

auto
assetExtensions() -> const QStringList&
{
    static const auto extensions =
      QStringList{ QStringLiteral(".wav"),  QStringLiteral(".flac"),
                   QStringLiteral(".ogg"),  QStringLiteral(".mp3"),
                   QStringLiteral(".png"),  QStringLiteral(".jpg"),
                   QStringLiteral(".jpeg"), QStringLiteral(".bmp"),
                   QStringLiteral(".gif"),  QStringLiteral(".webp"),
                   QStringLiteral(".tga"),  QStringLiteral(".dds"),
                   QStringLiteral(".cim"),  QStringLiteral(".mpg"),
                   QStringLiteral(".mpeg"), QStringLiteral(".mp4"),
                   QStringLiteral(".avi"),  QStringLiteral(".webm"),
                   QStringLiteral(".wmv"),  QStringLiteral(".mkv") };
    return extensions;
}

struct PhysicalBoundary
{
    std::filesystem::path physicalFile;
    QString remainder;
    bool trailingSeparator = false;
};

auto
findPhysicalBoundary(const std::filesystem::path& virtualPath)
  -> std::optional<PhysicalBoundary>
{
    const auto virtualString = support::pathToQString(virtualPath);
    const auto trailing =
      virtualString.endsWith('/') || virtualString.endsWith('\\');
    const auto normalized = virtualPath.lexically_normal();
    auto current = normalized.root_path();
    auto parts = std::vector<std::filesystem::path>{};
    for (const auto& part : normalized.relative_path()) {
        parts.push_back(part);
    }
    for (size_t index = 0; index < parts.size(); ++index) {
        current /= parts[index];
        std::error_code ec;
        if (!std::filesystem::is_regular_file(current, ec)) {
            continue;
        }
        auto remainder = std::filesystem::path{};
        for (auto rest = index + 1; rest < parts.size(); ++rest) {
            remainder /= parts[rest];
        }
        return PhysicalBoundary{ current,
                                 normalizeArchivePath(
                                   support::pathToQString(remainder)),
                                 trailing };
    }
    return std::nullopt;
}

auto
materializationKeyDigest(const std::filesystem::path& virtualPath)
  -> std::optional<QByteArray>
{
    const auto boundary = findPhysicalBoundary(virtualPath);
    if (!boundary || boundary->remainder.isEmpty()) {
        return std::nullopt;
    }

    std::error_code ec;
    const auto size = std::filesystem::file_size(boundary->physicalFile, ec);
    if (ec) {
        return std::nullopt;
    }
    const auto modified =
      std::filesystem::last_write_time(boundary->physicalFile, ec);
    if (ec) {
        return std::nullopt;
    }
    auto identity = QByteArray{ "song-asset-materialization-v1" };
    identity += '\0';
    identity += normalizedPhysicalPath(boundary->physicalFile).toUtf8();
    identity += '\0';
    identity += QByteArray::number(static_cast<qulonglong>(size));
    identity += '\0';
    identity += QByteArray::number(modified.time_since_epoch().count());
    identity += '\0';
    identity += boundary->remainder.toCaseFolded().toUtf8();
    return QCryptographicHash::hash(identity, QCryptographicHash::Sha256);
}

auto
sourceMaterializationPath(const std::filesystem::path& materializationDirectory,
                          const std::filesystem::path& virtualPath)
  -> std::optional<std::filesystem::path>
{
    const auto digest = materializationKeyDigest(virtualPath);
    if (!digest) {
        return std::nullopt;
    }
    return materializationDirectory /
           support::qStringToPath(
             QStringLiteral("source-") + QString::fromLatin1(digest->toHex()) +
             extensionForMaterialization(support::pathToQString(virtualPath)));
}

auto
resolutionMaterializationPath(
  const std::filesystem::path& materializationDirectory,
  const std::filesystem::path& requestedVirtualPath)
  -> std::optional<std::filesystem::path>
{
    const auto digest = materializationKeyDigest(requestedVirtualPath);
    if (!digest) {
        return std::nullopt;
    }
    return materializationDirectory /
           support::qStringToPath(QStringLiteral("resolve-") +
                                  QString::fromLatin1(digest->toHex()));
}

auto
existingMaterialization(const std::filesystem::path& materializationDirectory,
                        const std::filesystem::path& requestedVirtualPath)
  -> std::optional<std::filesystem::path>
{
    std::error_code ec;
    if (const auto direct = sourceMaterializationPath(materializationDirectory,
                                                      requestedVirtualPath);
        direct && std::filesystem::is_regular_file(*direct, ec)) {
        return direct;
    }
    const auto resolution = resolutionMaterializationPath(
      materializationDirectory, requestedVirtualPath);
    if (!resolution || !std::filesystem::is_regular_file(*resolution, ec)) {
        return std::nullopt;
    }
    auto file = QFile{ support::pathToQString(*resolution) };
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const auto targetName = QString::fromUtf8(file.readAll()).trimmed();
    const auto targetInfo = QFileInfo{ targetName };
    if (targetName.isEmpty() || targetInfo.fileName() != targetName ||
        !targetName.startsWith(QStringLiteral("source-"))) {
        return std::nullopt;
    }
    const auto target =
      materializationDirectory / support::qStringToPath(targetName);
    if (std::filesystem::is_regular_file(target, ec)) {
        return target;
    }
    return std::nullopt;
}

void
recordMaterializationResolution(
  const std::filesystem::path& materializationDirectory,
  const std::filesystem::path& requestedVirtualPath,
  const std::filesystem::path& target)
{
    const auto direct =
      sourceMaterializationPath(materializationDirectory, requestedVirtualPath);
    if (direct && *direct == target) {
        return;
    }
    const auto resolution = resolutionMaterializationPath(
      materializationDirectory, requestedVirtualPath);
    if (!resolution) {
        return;
    }
    auto file = QSaveFile{ support::pathToQString(*resolution) };
    const auto targetName = support::pathToQString(target.filename()).toUtf8();
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(targetName) != targetName.size() || !file.commit()) {
        spdlog::warn("Could not record song asset materialization {}",
                     support::pathToQString(*resolution).toStdString());
    }
}

auto
readCurrentEntry(archive* reader) -> QByteArray
{
    auto result = QByteArray{};
    auto buffer = std::array<char, archiveReadBlockSize>{};
    for (;;) {
        const auto read =
          archive_read_data(reader, buffer.data(), buffer.size());
        if (read == 0) {
            break;
        }
        if (read < 0) {
            throw archiveError(reader,
                               QStringLiteral("Could not read archive entry"));
        }
        result.append(buffer.data(), static_cast<qsizetype>(read));
    }
    return result;
}

auto
materializeCurrentEntry(archive* reader,
                        const std::filesystem::path& materializationDirectory,
                        const std::filesystem::path& virtualPath,
                        const std::atomic_bool* stop = nullptr)
  -> std::filesystem::path
{
    throwIfCancelled(stop);
    const auto target =
      sourceMaterializationPath(materializationDirectory, virtualPath);
    if (!target) {
        throw std::runtime_error(
          QStringLiteral("Could not identify archive source for %1")
            .arg(support::pathToQString(virtualPath))
            .toStdString());
    }
    std::error_code ec;
    if (std::filesystem::is_regular_file(*target, ec)) {
        archive_read_data_skip(reader);
        return *target;
    }
    const auto scratchPath = support::pathToQString(materializationDirectory);
    auto temporary =
      QTemporaryFile{ scratchPath + QStringLiteral("/.extract-XXXXXX") };
    if (!temporary.open()) {
        throw std::runtime_error("Could not create temporary song asset");
    }
    auto buffer = std::array<char, archiveReadBlockSize>{};
    for (;;) {
        throwIfCancelled(stop);
        const auto read =
          archive_read_data(reader, buffer.data(), buffer.size());
        if (read == 0) {
            break;
        }
        if (read < 0) {
            throw archiveError(
              reader, QStringLiteral("Could not extract archive entry"));
        }
        if (temporary.write(buffer.data(), static_cast<qint64>(read)) != read) {
            throw std::runtime_error("Could not write extracted song asset");
        }
    }
    temporary.flush();
    temporary.close();

    const auto targetName = support::pathToQString(*target);
    if (!temporary.rename(targetName)) {
        if (std::filesystem::is_regular_file(*target, ec)) {
            return *target;
        }
        throw std::runtime_error(
          QStringLiteral("Could not finalize extracted song asset %1: %2")
            .arg(targetName, temporary.errorString())
            .toStdString());
    }
    temporary.setAutoRemove(false);
    return *target;
}

void
requireZipFormat(archive* reader, const std::filesystem::path& archivePath)
{
    if ((archive_format(reader) & ARCHIVE_FORMAT_BASE_MASK) ==
        ARCHIVE_FORMAT_ZIP) {
        return;
    }
    throw std::runtime_error(
      QStringLiteral("Unsupported song archive format in %1. Convert or "
                     "repack it as a ZIP archive.")
        .arg(support::pathToQString(archivePath))
        .toStdString());
}

auto
nextHeader(archive* reader,
           archive_entry** entry,
           const std::filesystem::path& archivePath) -> bool
{
    for (;;) {
        const auto result = archive_read_next_header(reader, entry);
        if (result == ARCHIVE_EOF) {
            requireZipFormat(reader, archivePath);
            return false;
        }
        if (result == ARCHIVE_OK) {
            requireZipFormat(reader, archivePath);
            return true;
        }
        if (result == ARCHIVE_WARN) {
            requireZipFormat(reader, archivePath);
            spdlog::warn("Archive warning: {}",
                         archive_error_string(reader)
                           ? archive_error_string(reader)
                           : "unknown warning");
            return true;
        }
        throw archiveError(reader,
                           QStringLiteral("Could not read archive header"));
    }
}

auto
isRegularEntry(archive_entry* entry) -> bool
{
    return archive_entry_filetype(entry) == AE_IFREG;
}

auto
isEncryptedEntry(archive_entry* entry) -> bool
{
    return archive_entry_is_encrypted(entry) == 1;
}

struct LocatedContainer
{
    std::filesystem::path archivePath;
    std::filesystem::path virtualArchivePath;
    QString internalDirectory;
};

auto
locateContainer(const std::filesystem::path& virtualDirectory,
                const std::filesystem::path& materializationDirectory,
                const std::atomic_bool* stop = nullptr)
  -> std::optional<LocatedContainer>
{
    throwIfCancelled(stop);
    const auto boundary = findPhysicalBoundary(virtualDirectory);
    if (!boundary ||
        (boundary->remainder.isEmpty() && !boundary->trailingSeparator)) {
        return std::nullopt;
    }

    auto archivePath = boundary->physicalFile;
    auto virtualArchivePath = boundary->physicalFile;
    auto remaining = boundary->remainder;
    for (;;) {
        throwIfCancelled(stop);
        auto prefixes = QStringList{};
        for (auto index = remaining.indexOf('/'); index >= 0;
             index = remaining.indexOf('/', index + 1)) {
            prefixes.push_back(remaining.left(index));
        }
        if (boundary->trailingSeparator && !remaining.isEmpty()) {
            prefixes.push_back(remaining);
        }
        std::ranges::sort(prefixes,
                          [](const QString& left, const QString& right) {
                              return left.size() > right.size();
                          });
        prefixes.removeDuplicates();
        if (prefixes.isEmpty()) {
            return LocatedContainer{ archivePath,
                                     virtualArchivePath,
                                     remaining };
        }

        auto materializedPrefix = QString{};
        auto materializedArchive = std::filesystem::path{};
        for (const auto& prefix : prefixes) {
            throwIfCancelled(stop);
            if (!SongAssetStore::isArchivePath(
                  support::qStringToPath(prefix))) {
                continue;
            }
            const auto nestedVirtualPath = support::qStringToPath(
              joinVirtual(support::pathToQString(virtualArchivePath), prefix));
            const auto supportError =
              SongAssetStore::archiveSupportError(nestedVirtualPath);
            if (!supportError.isEmpty()) {
                throw std::runtime_error(supportError.toStdString());
            }
            if (const auto materialized = existingMaterialization(
                  materializationDirectory, nestedVirtualPath)) {
                materializedPrefix = prefix;
                materializedArchive = *materialized;
                break;
            }
        }
        if (!materializedPrefix.isEmpty()) {
            virtualArchivePath = support::qStringToPath(joinVirtual(
              support::pathToQString(virtualArchivePath), materializedPrefix));
            archivePath = std::move(materializedArchive);
            remaining.remove(0, materializedPrefix.size());
            while (remaining.startsWith('/')) {
                remaining.remove(0, 1);
            }
            continue;
        }

        auto reader = openArchive(archivePath);
        struct NestedArchiveCandidate
        {
            QString relativePath;
            std::filesystem::path virtualPath;
            std::filesystem::path physicalPath;
        };
        auto candidates = std::vector<NestedArchiveCandidate>{};
        auto* entry = static_cast<archive_entry*>(nullptr);
        while (nextHeader(reader.get(), &entry, archivePath)) {
            throwIfCancelled(stop);
            const auto path = entryPath(entry);
            const auto isPrefix =
              std::ranges::any_of(prefixes, [&path](const QString& prefix) {
                  return path.compare(prefix, Qt::CaseInsensitive) == 0;
              });
            if (!isRegularEntry(entry) || path.isEmpty() ||
                isEncryptedEntry(entry) || !isPrefix ||
                !SongAssetStore::isArchivePath(support::qStringToPath(path))) {
                archive_read_data_skip(reader.get());
                continue;
            }
            const auto nestedVirtualPath = support::qStringToPath(
              joinVirtual(support::pathToQString(virtualArchivePath), path));
            const auto extracted = materializeCurrentEntry(
              reader.get(), materializationDirectory, nestedVirtualPath, stop);
            candidates.push_back(
              { path, nestedVirtualPath, std::move(extracted) });
        }
        if (candidates.empty()) {
            return LocatedContainer{ archivePath,
                                     virtualArchivePath,
                                     remaining };
        }
        const auto selected = std::ranges::max_element(
          candidates, [](const auto& left, const auto& right) {
              return left.relativePath.size() < right.relativePath.size();
          });
        archivePath = selected->physicalPath;
        virtualArchivePath = selected->virtualPath;
        remaining.remove(0, selected->relativePath.size());
        while (remaining.startsWith('/')) {
            remaining.remove(0, 1);
        }
    }
}

auto
candidatePaths(const QString& requested) -> QStringList
{
    auto candidates = QStringList{ normalizeArchivePath(requested) };
    auto info = QFileInfo(requested);
    auto stem = info.path();
    if (stem == QStringLiteral(".")) {
        stem.clear();
    } else if (!stem.isEmpty()) {
        stem += '/';
    }
    stem += info.completeBaseName();
    for (const auto& extension : assetExtensions()) {
        candidates.push_back(stem + extension);
    }
    candidates.removeAll({});
    candidates.removeDuplicates();
    return candidates;
}

auto
relativeKey(const std::filesystem::path& path) -> QString
{
    return normalizeArchivePath(support::pathToQString(path)).toCaseFolded();
}

auto
resolveLocalAssets(const std::filesystem::path& directory,
                   const std::vector<std::filesystem::path>& relativePaths)
  -> std::unordered_map<std::filesystem::path, std::filesystem::path>
{
    auto result =
      std::unordered_map<std::filesystem::path, std::filesystem::path>{};
    auto files = QHash<QString, std::filesystem::path>{};
    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec)) {
        return result;
    }
    for (auto iterator =
           std::filesystem::recursive_directory_iterator(directory, ec);
         iterator != std::filesystem::recursive_directory_iterator{};
         iterator.increment(ec)) {
        if (ec) {
            break;
        }
        if (!iterator->is_regular_file(ec)) {
            continue;
        }
        const auto relative = iterator->path().lexically_relative(directory);
        files.insert(relativeKey(relative), iterator->path());
    }
    for (const auto& requested : relativePaths) {
        for (const auto& candidate :
             candidatePaths(support::pathToQString(requested))) {
            if (const auto found = files.constFind(candidate.toCaseFolded());
                found != files.cend()) {
                result.emplace(requested, *found);
                break;
            }
        }
    }
    return result;
}

auto
revisionForPath(const std::filesystem::path& virtualPath) -> QString
{
    auto physical = virtualPath;
    if (const auto boundary = findPhysicalBoundary(virtualPath)) {
        physical = boundary->physicalFile;
    } else {
        const auto resolved = resolveLocalAssets(virtualPath.parent_path(),
                                                 { virtualPath.filename() });
        if (const auto found = resolved.find(virtualPath.filename());
            found != resolved.end()) {
            physical = found->second;
        }
    }
    std::error_code ec;
    const auto size = std::filesystem::file_size(physical, ec);
    if (ec) {
        return QStringLiteral("0");
    }
    const auto modified = std::filesystem::last_write_time(physical, ec);
    const auto ticks = ec ? 0 : modified.time_since_epoch().count();
    return QString::number(
      static_cast<qulonglong>(size) ^ static_cast<qulonglong>(ticks), 16);
}

auto
encodedPath(const std::filesystem::path& path) -> QString
{
    return QString::fromLatin1(normalizedVirtualPath(path).toUtf8().toBase64(
      QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

} // namespace

SongAssetStore::SongAssetStore(QObject* parent)
  : QObject(parent)
  , temporaryDirectory(QDir::tempPath() +
                       QStringLiteral("/RhythmGame-song-assets-XXXXXX"))
{
    if (!temporaryDirectory.isValid()) {
        throw std::runtime_error("Could not create song asset scratch space");
    }
    materializationDirectory =
      support::qStringToPath(temporaryDirectory.path());
}

auto
SongAssetStore::isArchivePath(const std::filesystem::path& path) -> bool
{
    const auto value = support::pathToQString(path).toLower();
    return std::ranges::any_of(
      archiveExtensions(),
      [&value](const QString& extension) { return value.endsWith(extension); });
}

auto
SongAssetStore::isSupportedArchivePath(const std::filesystem::path& path)
  -> bool
{
    const auto value = support::pathToQString(path).toLower();
    return std::ranges::any_of(
      supportedArchiveExtensions(),
      [&value](const QString& extension) { return value.endsWith(extension); });
}

auto
SongAssetStore::archiveSupportError(const std::filesystem::path& path)
  -> QString
{
    if (isSplitArchivePath(path)) {
        return QStringLiteral(
                 "Split song archives are unsupported: %1. Combine or repack "
                 "the archive as a ZIP file.")
          .arg(support::pathToQString(path));
    }
    if (isSupportedArchivePath(path)) {
        return {};
    }
    return QStringLiteral(
             "Unsupported song archive format: %1. Convert or repack the "
             "archive as a ZIP file.")
      .arg(support::pathToQString(path));
}

auto
SongAssetStore::isSplitArchivePath(const std::filesystem::path& path) -> bool
{
    const auto value = support::pathToQString(path).toLower();
    static const auto numberedSplit =
      QRegularExpression{ QStringLiteral(R"(\.(?:7z|zip)\.\d{3}$)") };
    static const auto splitRar =
      QRegularExpression{ QStringLiteral(R"(\.part\d+\.rar$)") };
    static const auto splitZip =
      QRegularExpression{ QStringLiteral(R"(\.z\d{2}$)") };
    static const auto legacyRar =
      QRegularExpression{ QStringLiteral(R"(\.r\d{2}$)") };
    return numberedSplit.match(value).hasMatch() ||
           splitRar.match(value).hasMatch() ||
           splitZip.match(value).hasMatch() ||
           legacyRar.match(value).hasMatch();
}

auto
SongAssetStore::isArchived(const std::filesystem::path& virtualPath) const
  -> bool
{
    const auto boundary = findPhysicalBoundary(virtualPath);
    return boundary &&
           (!boundary->remainder.isEmpty() || boundary->trailingSeparator);
}

void
SongAssetStore::walkArchive(const std::filesystem::path& archivePath,
                            const WantsContents& wantsContents,
                            const EntryVisitor& visitor,
                            std::atomic_bool* stop) const
{
    struct PendingArchive
    {
        std::filesystem::path physicalPath;
        QString virtualPrefix;
        bool nested;
    };
    auto pending = std::deque<PendingArchive>{};
    auto prefix = normalizedVirtualPath(archivePath);
    if (!prefix.endsWith('/')) {
        prefix += '/';
    }
    pending.push_back({ archivePath, prefix, false });
    while (!pending.empty()) {
        if (stop && *stop) {
            return;
        }
        auto current = std::move(pending.front());
        pending.pop_front();
        auto reader = ArchiveReader{};
        try {
            reader = openArchive(current.physicalPath);
        } catch (const std::exception& error) {
            if (!current.nested) {
                throw;
            }
            spdlog::warn("Skipping unreadable nested archive {}: {}",
                         current.virtualPrefix.toStdString(),
                         error.what());
            continue;
        }
        auto* entry = static_cast<archive_entry*>(nullptr);
        while (nextHeader(reader.get(), &entry, current.physicalPath)) {
            if (stop && *stop) {
                return;
            }
            const auto relative = entryPath(entry);
            if (!isRegularEntry(entry) || relative.isEmpty()) {
                archive_read_data_skip(reader.get());
                continue;
            }
            const auto virtualPath = current.virtualPrefix + relative;
            if (isEncryptedEntry(entry)) {
                spdlog::warn("Encrypted archive entry is unsupported: {}",
                             virtualPath.toStdString());
                archive_read_data_skip(reader.get());
                continue;
            }
            if (isSplitArchivePath(support::qStringToPath(relative))) {
                spdlog::error(
                  "{}",
                  archiveSupportError(support::qStringToPath(virtualPath))
                    .toStdString());
                archive_read_data_skip(reader.get());
                continue;
            }
            if (isArchivePath(support::qStringToPath(relative))) {
                const auto nestedVirtualPath =
                  support::qStringToPath(virtualPath);
                const auto supportError =
                  archiveSupportError(nestedVirtualPath);
                if (!supportError.isEmpty()) {
                    spdlog::error("{}", supportError.toStdString());
                    archive_read_data_skip(reader.get());
                    continue;
                }
                const auto nested = materializeCurrentEntry(
                  reader.get(), materializationDirectory, nestedVirtualPath);
                pending.push_back({ nested, virtualPath + '/', true });
                continue;
            }

            auto data = std::optional<QByteArray>{};
            const auto fsVirtualPath = support::qStringToPath(virtualPath);
            if (wantsContents && wantsContents(fsVirtualPath)) {
                data = readCurrentEntry(reader.get());
            } else {
                archive_read_data_skip(reader.get());
            }
            visitor(ArchiveEntry{ fsVirtualPath, std::move(data) });
        }
    }
}

auto
SongAssetStore::materializeRelative(
  const std::filesystem::path& virtualDirectory,
  const std::vector<std::filesystem::path>& relativePaths,
  const std::atomic_bool* stop) const
  -> std::unordered_map<std::filesystem::path, std::filesystem::path>
{
    throwIfCancelled(stop);
    if (relativePaths.empty()) {
        return {};
    }

    const auto requestedVirtualPath =
      [&virtualDirectory](const std::filesystem::path& relative) {
          return support::qStringToPath(
            joinVirtual(support::pathToQString(virtualDirectory),
                        support::pathToQString(relative)));
      };
    auto result =
      std::unordered_map<std::filesystem::path, std::filesystem::path>{};
    auto unresolved = std::vector<std::filesystem::path>{};
    for (const auto& requested : relativePaths) {
        throwIfCancelled(stop);
        if (const auto materialized = existingMaterialization(
              materializationDirectory, requestedVirtualPath(requested))) {
            result.emplace(requested, *materialized);
        } else {
            unresolved.push_back(requested);
        }
    }
    if (unresolved.empty()) {
        return result;
    }

    const auto container =
      locateContainer(virtualDirectory, materializationDirectory, stop);
    if (!container) {
        auto local = resolveLocalAssets(virtualDirectory, unresolved);
        result.insert(std::make_move_iterator(local.begin()),
                      std::make_move_iterator(local.end()));
        return result;
    }

    auto requestedVirtualPaths = std::vector<std::filesystem::path>{};
    requestedVirtualPaths.reserve(unresolved.size());
    for (const auto& requested : unresolved) {
        requestedVirtualPaths.push_back(requestedVirtualPath(requested));
    }
    materializeRequested(requestedVirtualPaths, stop);
    for (const auto& requested : unresolved) {
        if (const auto materialized = existingMaterialization(
              materializationDirectory, requestedVirtualPath(requested))) {
            result.emplace(requested, *materialized);
        }
    }
    return result;
}

void
SongAssetStore::materializeRequested(
  const std::vector<std::filesystem::path>& virtualPaths,
  const std::atomic_bool* stop) const
{
    struct Candidate
    {
        std::filesystem::path requestedVirtualPath;
        int priority;
    };
    struct Group
    {
        std::filesystem::path archivePath;
        std::filesystem::path virtualArchivePath;
        QMultiHash<QString, Candidate> exactCandidates;
        QMultiHash<QString, Candidate> fallbackCandidates;
    };
    struct FallbackResolution
    {
        int priority = std::numeric_limits<int>::max();
        std::filesystem::path localPath;
    };
    struct FallbackEntry
    {
        int matches = 0;
        std::filesystem::path localPath;
    };

    auto groups = std::vector<Group>{};
    auto uniquePaths = std::unordered_set<std::filesystem::path>{};
    for (const auto& virtualPath : virtualPaths) {
        throwIfCancelled(stop);
        if (virtualPath.empty() || !uniquePaths.insert(virtualPath).second ||
            existingMaterialization(materializationDirectory, virtualPath)) {
            continue;
        }
        const auto parent = virtualPath.parent_path() / "";
        const auto container =
          locateContainer(parent, materializationDirectory, stop);
        if (!container) {
            continue;
        }
        auto group = std::ranges::find_if(groups, [&](const auto& candidate) {
            return candidate.archivePath == container->archivePath &&
                   candidate.virtualArchivePath ==
                     container->virtualArchivePath;
        });
        if (group == groups.end()) {
            groups.push_back({ container->archivePath,
                               container->virtualArchivePath,
                               {},
                               {} });
            group = std::prev(groups.end());
        }
        const auto possible =
          candidatePaths(support::pathToQString(virtualPath.filename()));
        for (auto priority = 0; priority < possible.size(); ++priority) {
            const auto internal = normalizeArchivePath(
              joinVirtual(container->internalDirectory, possible[priority]));
            if (!internal.isEmpty()) {
                const auto candidate = Candidate{ virtualPath, priority };
                group->exactCandidates.insert(internal.toCaseFolded(),
                                              candidate);
                group->fallbackCandidates.insert(
                  QFileInfo(possible[priority]).fileName().toCaseFolded(),
                  candidate);
            }
        }
    }

    for (const auto& group : groups) {
        throwIfCancelled(stop);
        auto priorities = std::unordered_map<std::filesystem::path, int>{};
        auto fallbackResolutions =
          std::unordered_map<std::filesystem::path, FallbackResolution>{};
        auto fallbackEntries = QHash<QString, FallbackEntry>{};
        auto exactMaterialized = std::unordered_set<std::filesystem::path>{};
        auto reader = openArchive(group.archivePath);
        auto* entry = static_cast<archive_entry*>(nullptr);
        while (nextHeader(reader.get(), &entry, group.archivePath)) {
            throwIfCancelled(stop);
            const auto relative = entryPath(entry);
            if (!isRegularEntry(entry) || relative.isEmpty() ||
                isEncryptedEntry(entry)) {
                archive_read_data_skip(reader.get());
                continue;
            }
            const auto exactMatches =
              group.exactCandidates.values(relative.toCaseFolded());
            const auto basename = QFileInfo(relative).fileName().toCaseFolded();
            const auto wantsFallback =
              group.fallbackCandidates.contains(basename);
            auto firstFallback = false;
            if (wantsFallback) {
                auto& fallback = fallbackEntries[basename];
                firstFallback = fallback.matches == 0;
                ++fallback.matches;
            }
            if (exactMatches.isEmpty() && !firstFallback) {
                archive_read_data_skip(reader.get());
                continue;
            }
            const auto entryVirtualPath = support::qStringToPath(joinVirtual(
              support::pathToQString(group.virtualArchivePath), relative));
            const auto local = materializeCurrentEntry(
              reader.get(), materializationDirectory, entryVirtualPath, stop);
            if (!exactMatches.isEmpty()) {
                exactMaterialized.insert(local);
            }
            for (const auto& match : exactMatches) {
                const auto old = priorities.find(match.requestedVirtualPath);
                if (old != priorities.end() && old->second <= match.priority) {
                    continue;
                }
                priorities[match.requestedVirtualPath] = match.priority;
                recordMaterializationResolution(
                  materializationDirectory, match.requestedVirtualPath, local);
            }
            if (firstFallback) {
                fallbackEntries[basename].localPath = local;
            }
        }
        for (auto fallbackEntry = fallbackEntries.cbegin();
             fallbackEntry != fallbackEntries.cend();
             ++fallbackEntry) {
            if (fallbackEntry->matches != 1) {
                if (!exactMaterialized.contains(fallbackEntry->localPath)) {
                    std::error_code ec;
                    std::filesystem::remove(fallbackEntry->localPath, ec);
                }
                continue;
            }
            for (const auto& match :
                 group.fallbackCandidates.values(fallbackEntry.key())) {
                if (priorities.contains(match.requestedVirtualPath)) {
                    continue;
                }
                auto& fallback =
                  fallbackResolutions[match.requestedVirtualPath];
                if (match.priority < fallback.priority) {
                    fallback.priority = match.priority;
                    fallback.localPath = fallbackEntry->localPath;
                }
            }
        }
        for (const auto& [requested, fallback] : fallbackResolutions) {
            if (!priorities.contains(requested)) {
                recordMaterializationResolution(
                  materializationDirectory, requested, fallback.localPath);
            }
        }
    }
}

auto
SongAssetStore::read(const std::filesystem::path& virtualPath) const
  -> QByteArray
{
    const auto localPath = materialize(virtualPath);
    auto file = QFile{ support::pathToQString(localPath) };
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(QStringLiteral("Could not open song asset %1")
                                   .arg(support::pathToQString(virtualPath))
                                   .toStdString());
    }
    return file.readAll();
}

auto
SongAssetStore::materialize(const std::filesystem::path& virtualPath,
                            const std::atomic_bool* stop) const
  -> std::filesystem::path
{
    throwIfCancelled(stop);
    if (!isArchived(virtualPath)) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(virtualPath, ec)) {
            return virtualPath;
        }
    }
    const auto parent = virtualPath.parent_path() / "";
    const auto relative = virtualPath.filename();
    auto result = materializeRelative(parent, { relative }, stop);
    if (const auto found = result.find(relative); found != result.end()) {
        return found->second;
    }
    throw std::runtime_error(QStringLiteral("Song asset not found: %1")
                               .arg(support::pathToQString(virtualPath))
                               .toStdString());
}

auto
SongAssetStore::imageUrl(const std::filesystem::path& virtualPath) -> QString
{
    if (virtualPath.empty()) {
        return {};
    }
    const auto boundary = findPhysicalBoundary(virtualPath);
    if (!boundary ||
        (boundary->remainder.isEmpty() && !boundary->trailingSeparator)) {
        auto physicalPath = virtualPath;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(physicalPath, ec)) {
            const auto resolved = resolveLocalAssets(
              virtualPath.parent_path(), { virtualPath.filename() });
            if (const auto found = resolved.find(virtualPath.filename());
                found != resolved.end()) {
                physicalPath = found->second;
            }
        }
        return QUrl::fromLocalFile(support::pathToQString(physicalPath))
          .toString();
    }
    return QStringLiteral("image://song-assets/") + encodedPath(virtualPath) +
           QStringLiteral("?rev=") + revisionForPath(virtualPath);
}

auto
SongAssetStore::imageUrl(const QString& virtualDirectory,
                         const QString& relativePath) -> QString
{
    if (relativePath.isEmpty()) {
        return {};
    }
    const auto relative = support::qStringToPath(relativePath);
    if (relative.is_absolute()) {
        return imageUrl(relative);
    }
    return imageUrl(
      support::qStringToPath(joinVirtual(virtualDirectory, relativePath)));
}

auto
SongAssetStore::audioUrl(const std::filesystem::path& virtualPath) -> QString
{
    if (virtualPath.empty()) {
        return {};
    }
    return QStringLiteral("rgasset:") + encodedPath(virtualPath);
}

auto
SongAssetStore::isAudioUrl(const QString& source) -> bool
{
    return source.startsWith(QStringLiteral("rgasset:"));
}

auto
SongAssetStore::pathFromUrl(const QString& source) -> std::filesystem::path
{
    auto encoded = source;
    if (isAudioUrl(encoded)) {
        encoded.remove(0, QStringLiteral("rgasset:").size());
    } else {
        const auto marker = QStringLiteral("image://song-assets/");
        if (encoded.startsWith(marker)) {
            encoded.remove(0, marker.size());
        }
    }
    if (const auto query = encoded.indexOf('?'); query >= 0) {
        encoded.truncate(query);
    }
    const auto data =
      QByteArray::fromBase64(encoded.toLatin1(), QByteArray::Base64UrlEncoding);
    return support::qStringToPath(QString::fromUtf8(data));
}

QString
SongAssetStore::imageSource(const QString& virtualDirectory,
                            const QString& relativePath) const
{
    return imageUrl(virtualDirectory, relativePath);
}

QString
SongAssetStore::localFile(const QString& virtualPath) const
{
    try {
        auto path = support::qStringToPath(virtualPath);
        if (isAudioUrl(virtualPath) ||
            virtualPath.startsWith(QStringLiteral("image://song-assets/"))) {
            path = pathFromUrl(virtualPath);
        }
        return support::pathToQString(materialize(path));
    } catch (const std::exception& error) {
        spdlog::warn("Could not materialize song asset {}: {}",
                     virtualPath.toStdString(),
                     error.what());
        return {};
    }
}

QString
SongAssetStore::containingFolder(const QString& virtualPath) const
{
    const auto path = support::qStringToPath(virtualPath);
    if (const auto boundary = findPhysicalBoundary(path);
        boundary &&
        (!boundary->remainder.isEmpty() || boundary->trailingSeparator)) {
        return support::pathToQString(boundary->physicalFile.parent_path());
    }
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec)) {
        return support::pathToQString(path);
    }
    return support::pathToQString(path.parent_path());
}

} // namespace resource_managers
