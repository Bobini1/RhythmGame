#include "SongAssetStore.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <iconv.h>
#include <zip.h>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringConverter>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>
#include <array>
#include <deque>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
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

auto
zipError(const QString& prefix, zip_error_t* error) -> std::runtime_error
{
    const auto* detail = error ? zip_error_strerror(error) : nullptr;
    return std::runtime_error(QStringLiteral("%1: %2")
                                .arg(prefix,
                                     detail
                                       ? QString::fromUtf8(detail)
                                       : QStringLiteral("unknown ZIP error"))
                                .toStdString());
}

auto
zipArchiveError(zip_t* archive, const QString& prefix) -> std::runtime_error
{
    return zipError(prefix, archive ? zip_get_error(archive) : nullptr);
}

struct ZipArchiveDeleter
{
    void operator()(zip_t* value) const
    {
        if (value) {
            zip_discard(value);
        }
    }
};

struct ZipFileDeleter
{
    void operator()(zip_file_t* value) const
    {
        if (value) {
            zip_fclose(value);
        }
    }
};

using ZipArchiveHandle = std::unique_ptr<zip_t, ZipArchiveDeleter>;
using ZipFileHandle = std::unique_ptr<zip_file_t, ZipFileDeleter>;

auto
openPhysicalZip(const std::filesystem::path& path) -> ZipArchiveHandle
{
    const auto supportError = SongAssetStore::archiveSupportError(path);
    if (!supportError.isEmpty()) {
        throw std::runtime_error(supportError.toStdString());
    }

    auto error = zip_error_t{};
    zip_error_init(&error);
#ifdef _WIN32
    const auto file =
      CreateFileW(path.c_str(),
                  GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        zip_error_fini(&error);
        throw std::system_error(static_cast<int>(GetLastError()),
                                std::system_category(),
                                QStringLiteral("Could not open ZIP archive %1")
                                  .arg(support::pathToQString(path))
                                  .toStdString());
    }
    auto* source =
      zip_source_win32handle_create(file, 0, ZIP_LENGTH_TO_END, &error);
    if (!source) {
        CloseHandle(file);
    }
#else
    auto* source =
      zip_source_file_create(path.c_str(), 0, ZIP_LENGTH_TO_END, &error);
#endif
    if (!source) {
        auto exception =
          zipError(QStringLiteral("Could not open ZIP archive %1")
                     .arg(support::pathToQString(path)),
                   &error);
        zip_error_fini(&error);
        throw exception;
    }

    auto* archive = zip_open_from_source(source, ZIP_RDONLY, &error);
    if (!archive) {
        auto exception =
          zipError(QStringLiteral("Could not open ZIP archive %1")
                     .arg(support::pathToQString(path)),
                   &error);
        zip_source_free(source);
        zip_error_fini(&error);
        throw exception;
    }
    zip_error_fini(&error);
    return ZipArchiveHandle{ archive };
}

auto
decodeUtf8(const QByteArrayView value) -> std::optional<QString>
{
    auto decoder = QStringDecoder{ QStringDecoder::Utf8 };
    auto decoded = QString{ decoder.decode(value) };
    if (decoder.hasError()) {
        return std::nullopt;
    }
    return decoded;
}

class Cp932Decoder
{
  public:
#ifdef _WIN32
    Cp932Decoder() = default;
#else
    Cp932Decoder()
      : converter(iconv_open("UTF-8", "CP932"))
    {
        if (converter == invalidConverter()) {
            converter = iconv_open("UTF-8", "SHIFT-JIS");
        }
    }
#endif

    ~Cp932Decoder()
    {
#ifndef _WIN32
        if (converter != invalidConverter()) {
            iconv_close(converter);
        }
#endif
    }

    Cp932Decoder(const Cp932Decoder&) = delete;
    Cp932Decoder& operator=(const Cp932Decoder&) = delete;

    auto decode(const QByteArrayView value) -> std::optional<QString>
    {
#ifdef _WIN32
        const auto size = MultiByteToWideChar(
          932, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (size <= 0) {
            return std::nullopt;
        }
        auto decoded = std::wstring(static_cast<size_t>(size), L'\0');
        if (MultiByteToWideChar(932,
                                0,
                                value.data(),
                                static_cast<int>(value.size()),
                                decoded.data(),
                                size) != size) {
            return std::nullopt;
        }
        return QString::fromWCharArray(decoded.data(), size);
#else
        if (converter == invalidConverter()) {
            return std::nullopt;
        }
        iconv(converter, nullptr, nullptr, nullptr, nullptr);
        auto output = QByteArray{};
        output.resize(std::max<qsizetype>(16, value.size() * 4 + 4));
        auto* input = const_cast<char*>(value.data());
        auto inputRemaining = static_cast<size_t>(value.size());
        auto* destination = output.data();
        auto outputRemaining = static_cast<size_t>(output.size());
        if (iconv(converter,
                  &input,
                  &inputRemaining,
                  &destination,
                  &outputRemaining) == static_cast<size_t>(-1) ||
            inputRemaining != 0) {
            return std::nullopt;
        }
        output.resize(output.size() - static_cast<qsizetype>(outputRemaining));
        return decodeUtf8(output);
#endif
    }

  private:
#ifndef _WIN32
    static auto invalidConverter() -> iconv_t
    {
        return reinterpret_cast<iconv_t>(-1);
    }

    iconv_t converter;
#endif
};

auto
unicodeExtraPath(zip_t* archive,
                 const zip_uint64_t index,
                 const QByteArrayView rawName) -> std::optional<QString>
{
    auto size = zip_uint16_t{};
    const auto* extra = zip_file_extra_field_get_by_id(
      archive, index, 0x7075, 0, &size, ZIP_FL_CENTRAL | ZIP_FL_UNCHANGED);
    if (!extra || size < 5 || extra[0] != 1) {
        return std::nullopt;
    }
    const auto expectedCrc = static_cast<zip_uint32_t>(extra[1]) |
                             (static_cast<zip_uint32_t>(extra[2]) << 8U) |
                             (static_cast<zip_uint32_t>(extra[3]) << 16U) |
                             (static_cast<zip_uint32_t>(extra[4]) << 24U);
    const auto actualCrc = static_cast<zip_uint32_t>(
      crc32(0,
            reinterpret_cast<const Bytef*>(rawName.data()),
            static_cast<uInt>(rawName.size())));
    if (expectedCrc != actualCrc) {
        return std::nullopt;
    }
    return decodeUtf8(
      QByteArrayView{ reinterpret_cast<const char*>(extra + 5), size - 5 });
}

auto
decodeEntryName(zip_t* archive, const zip_uint64_t index, Cp932Decoder& cp932)
  -> QString
{
    const auto* rawValue =
      zip_get_name(archive, index, ZIP_FL_ENC_RAW | ZIP_FL_UNCHANGED);
    if (!rawValue) {
        throw zipArchiveError(archive,
                              QStringLiteral("Could not read ZIP entry name"));
    }
    const auto raw =
      QByteArrayView{ rawValue, static_cast<qsizetype>(strlen(rawValue)) };
    if (const auto unicode = unicodeExtraPath(archive, index, raw)) {
        return *unicode;
    }
    const auto* strict =
      zip_get_name(archive, index, ZIP_FL_ENC_STRICT | ZIP_FL_UNCHANGED);
    if (!strict) {
        throw zipArchiveError(
          archive, QStringLiteral("Could not decode ZIP entry name"));
    }
    const auto strictName = QString::fromUtf8(strict);
    if (const auto utf8 = decodeUtf8(raw); utf8 && *utf8 == strictName) {
        // In strict mode libzip only preserves non-ASCII UTF-8 when the ZIP
        // entry explicitly has the UTF-8 flag. Without that flag, prefer the
        // encoding used by legacy Japanese song archives.
        return *utf8;
    }
    if (const auto legacy = cp932.decode(raw)) {
        return *legacy;
    }
    return strictName;
}

struct IndexedZipEntry
{
    QString path;
    bool directory = false;
    bool encrypted = false;
    std::optional<zip_uint64_t> uncompressedSize;
    std::optional<zip_uint16_t> compressionMethod;
};

struct IndexedZipLookup
{
    size_t hash{};
    zip_uint64_t index{};
};

class IndexedZipArchive : public std::enable_shared_from_this<IndexedZipArchive>
{
  public:
    IndexedZipArchive(ZipArchiveHandle archive,
                      std::shared_ptr<IndexedZipArchive> parent = {})
      : archive(std::move(archive))
      , parent(std::move(parent))
    {
        buildIndex();
    }

    ~IndexedZipArchive()
    {
        auto locks = lockParents();
        archive.reset();
    }

    IndexedZipArchive(const IndexedZipArchive&) = delete;
    IndexedZipArchive& operator=(const IndexedZipArchive&) = delete;

    [[nodiscard]] auto allEntries() const -> const std::vector<IndexedZipEntry>&
    {
        return entries;
    }

    [[nodiscard]] auto entryCount() const -> size_t { return entries.size(); }

    [[nodiscard]] auto entry(const zip_uint64_t index) const
      -> const IndexedZipEntry&
    {
        return entries.at(static_cast<size_t>(index));
    }

    [[nodiscard]] auto findExact(QString path) const
      -> std::optional<zip_uint64_t>
    {
        path = normalizeArchivePath(std::move(path)).toCaseFolded();
        const auto hash = qHash(path);
        const auto first = std::ranges::lower_bound(
          exactEntries, hash, {}, &IndexedZipLookup::hash);
        const auto last = std::ranges::upper_bound(
          exactEntries, hash, {}, &IndexedZipLookup::hash);
        for (auto match = first; match != last; ++match) {
            const auto& value = entry(match->index);
            if (!value.directory && value.path.toCaseFolded() == path) {
                return match->index;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] auto findUniqueBasename(QString basename) const
      -> std::optional<zip_uint64_t>
    {
        basename = std::move(basename).toCaseFolded();
        const auto hash = qHash(basename);
        const auto first = std::ranges::lower_bound(
          basenameEntries, hash, {}, &IndexedZipLookup::hash);
        const auto last = std::ranges::upper_bound(
          basenameEntries, hash, {}, &IndexedZipLookup::hash);
        auto result = std::optional<zip_uint64_t>{};
        for (auto match = first; match != last; ++match) {
            const auto& value = entry(match->index);
            if (QFileInfo(value.path).fileName().toCaseFolded() != basename) {
                continue;
            }
            if (result) {
                return std::nullopt;
            }
            result = match->index;
        }
        return result;
    }

    auto read(const zip_uint64_t index,
              const std::atomic_bool* stop = nullptr) const -> QByteArray
    {
        auto result = QByteArray{};
        stream(
          index,
          QStringLiteral("Could not read ZIP entry"),
          [&result](const char* data, const qint64 size) {
              result.append(data, size);
          },
          stop);
        return result;
    }

    void extract(const zip_uint64_t index,
                 QIODevice& destination,
                 const std::atomic_bool* stop = nullptr) const
    {
        stream(
          index,
          QStringLiteral("Could not extract ZIP entry"),
          [&destination](const char* data, const qint64 size) {
              if (destination.write(data, size) != size) {
                  throw std::runtime_error(
                    "Could not write extracted song asset");
              }
          },
          stop);
    }

    [[nodiscard]] auto openNested(const zip_uint64_t index) const
      -> std::shared_ptr<IndexedZipArchive>
    {
        auto locks = lockChain();
        auto error = zip_error_t{};
        zip_error_init(&error);
        auto* source = zip_source_zip_file_create(
          archive.get(), index, 0, 0, -1, nullptr, &error);
        if (!source) {
            auto exception = zipError(
              QStringLiteral("Could not open nested ZIP entry"), &error);
            zip_error_fini(&error);
            throw exception;
        }
        if (zip_source_is_seekable(source) != 1) {
            zip_source_free(source);
            zip_error_fini(&error);
            return {};
        }
        auto* child = zip_open_from_source(source, ZIP_RDONLY, &error);
        if (!child) {
            auto exception = zipError(
              QStringLiteral("Could not open nested ZIP archive"), &error);
            zip_source_free(source);
            zip_error_fini(&error);
            throw exception;
        }
        zip_error_fini(&error);
        return std::make_shared<IndexedZipArchive>(
          ZipArchiveHandle{ child },
          const_cast<IndexedZipArchive*>(this)->shared_from_this());
    }

  private:
    void stream(const zip_uint64_t index,
                const QString& errorContext,
                const std::function<void(const char*, qint64)>& consume,
                const std::atomic_bool* stop) const
    {
        throwIfCancelled(stop);
        auto locks = lockChain();
        auto file = openFile(index);
        auto buffer = std::array<char, archiveReadBlockSize>{};
        for (;;) {
            throwIfCancelled(stop);
            const auto bytes =
              zip_fread(file.get(), buffer.data(), buffer.size());
            if (bytes == 0) {
                break;
            }
            if (bytes < 0) {
                throw zipError(errorContext, zip_file_get_error(file.get()));
            }
            consume(buffer.data(), bytes);
        }
    }
    void appendMutexes(std::vector<std::mutex*>& mutexes) const
    {
        if (parent) {
            parent->appendMutexes(mutexes);
        }
        mutexes.push_back(&mutex);
    }

    [[nodiscard]] auto lockParents() const
      -> std::vector<std::unique_lock<std::mutex>>
    {
        auto mutexes = std::vector<std::mutex*>{};
        if (parent) {
            parent->appendMutexes(mutexes);
        }
        auto locks = std::vector<std::unique_lock<std::mutex>>{};
        locks.reserve(mutexes.size());
        for (auto* value : mutexes) {
            locks.emplace_back(*value);
        }
        return locks;
    }

    [[nodiscard]] auto lockChain() const
      -> std::vector<std::unique_lock<std::mutex>>
    {
        auto mutexes = std::vector<std::mutex*>{};
        appendMutexes(mutexes);
        auto locks = std::vector<std::unique_lock<std::mutex>>{};
        locks.reserve(mutexes.size());
        for (auto* value : mutexes) {
            locks.emplace_back(*value);
        }
        return locks;
    }

    [[nodiscard]] auto openFile(const zip_uint64_t index) const -> ZipFileHandle
    {
        auto* value = zip_fopen_index(archive.get(), index, ZIP_FL_UNCHANGED);
        if (!value) {
            throw zipArchiveError(archive.get(),
                                  QStringLiteral("Could not open ZIP entry"));
        }
        return ZipFileHandle{ value };
    }

    void buildIndex()
    {
        const auto count = zip_get_num_entries(archive.get(), ZIP_FL_UNCHANGED);
        if (count < 0) {
            throw zipArchiveError(
              archive.get(), QStringLiteral("Could not enumerate ZIP archive"));
        }
        entries.reserve(static_cast<size_t>(count));
        auto cp932 = Cp932Decoder{};
        for (auto index = zip_uint64_t{};
             index < static_cast<zip_uint64_t>(count);
             ++index) {
            auto decoded = decodeEntryName(archive.get(), index, cp932);
            const auto directory =
              decoded.endsWith('/') || decoded.endsWith('\\');
            auto path = normalizeArchivePath(std::move(decoded));
            auto stat = zip_stat_t{};
            zip_stat_init(&stat);
            if (zip_stat_index(archive.get(), index, ZIP_FL_UNCHANGED, &stat) !=
                0) {
                throw zipArchiveError(
                  archive.get(), QStringLiteral("Could not inspect ZIP entry"));
            }
            const auto encrypted =
              (stat.valid & ZIP_STAT_ENCRYPTION_METHOD) != 0 &&
              stat.encryption_method != ZIP_EM_NONE;
            const auto uncompressedSize =
              (stat.valid & ZIP_STAT_SIZE) != 0
                ? std::optional<zip_uint64_t>{ stat.size }
                : std::nullopt;
            const auto compressionMethod =
              (stat.valid & ZIP_STAT_COMP_METHOD) != 0
                ? std::optional<zip_uint16_t>{ stat.comp_method }
                : std::nullopt;
            entries.push_back({ path,
                                directory,
                                encrypted,
                                uncompressedSize,
                                compressionMethod });
            if (path.isEmpty()) {
                continue;
            }
            exactEntries.push_back({ qHash(path.toCaseFolded()), index });
            if (!directory) {
                basenameEntries.push_back(
                  { qHash(QFileInfo(path).fileName().toCaseFolded()), index });
            }
        }
        const auto byHash = [](const IndexedZipLookup& left,
                               const IndexedZipLookup& right) {
            return left.hash < right.hash;
        };
        std::ranges::sort(exactEntries, byHash);
        std::ranges::sort(basenameEntries, byHash);
    }

    ZipArchiveHandle archive;
    std::shared_ptr<IndexedZipArchive> parent;
    mutable std::mutex mutex;
    std::vector<IndexedZipEntry> entries;
    std::vector<IndexedZipLookup> exactEntries;
    std::vector<IndexedZipLookup> basenameEntries;
};

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

class SongAssetStore::Impl
{
  public:
    struct LocatedContainer
    {
        std::shared_ptr<IndexedZipArchive> archive;
        std::filesystem::path virtualArchivePath;
        QString internalDirectory;
    };

    struct LocatedEntry
    {
        std::shared_ptr<IndexedZipArchive> archive;
        zip_uint64_t index;
        std::filesystem::path virtualPath;
    };

    explicit Impl(std::filesystem::path materializationDirectory)
      : materializationDirectory(std::move(materializationDirectory))
    {
    }

    [[nodiscard]] auto physicalArchive(const std::filesystem::path& path) const
      -> std::shared_ptr<IndexedZipArchive>
    {
        const auto key = physicalArchiveKey(path);
        const auto identity = normalizedPhysicalPath(path);
        {
            auto lock = std::scoped_lock{ cacheMutex };
            if (const auto found = physicalArchives.constFind(identity);
                found != physicalArchives.cend() && found->key == key) {
                return found->archive;
            }
        }

        auto opened =
          std::make_shared<IndexedZipArchive>(openPhysicalZip(path));
        auto lock = std::scoped_lock{ cacheMutex };
        if (const auto found = physicalArchives.constFind(identity);
            found != physicalArchives.cend() && found->key == key) {
            return found->archive;
        }
        physicalArchives.insert(identity, { key, opened });
        return opened;
    }

    [[nodiscard]] auto nestedArchive(
      const std::shared_ptr<IndexedZipArchive>& parent,
      const zip_uint64_t index,
      const std::filesystem::path& virtualPath,
      const std::atomic_bool* stop = nullptr) const
      -> std::shared_ptr<IndexedZipArchive>
    {
        throwIfCancelled(stop);
        const auto& entry = parent->entry(index);
        if (!entry.compressionMethod ||
            *entry.compressionMethod != ZIP_CM_STORE) {
            const auto declaredSize =
              entry.uncompressedSize
                ? QStringLiteral("%1 bytes")
                    .arg(static_cast<qulonglong>(*entry.uncompressedSize))
                : QStringLiteral("unknown");
            throw std::runtime_error(
              QStringLiteral(
                "Nested ZIP is compressed inside its parent and is "
                "unsupported: %1 (declared size: %2). Repack the outer ZIP "
                "with nested .zip entries set to Store or no compression.")
                .arg(support::pathToQString(virtualPath), declaredSize)
                .toStdString());
        }
        const auto digest = materializationKeyDigest(virtualPath);
        if (!digest) {
            throw std::runtime_error(
              QStringLiteral("Could not identify nested ZIP archive %1")
                .arg(support::pathToQString(virtualPath))
                .toStdString());
        }
        const auto key =
          QStringLiteral("nested:") + QString::fromLatin1(digest->toHex());
        {
            auto lock = std::scoped_lock{ cacheMutex };
            if (const auto found = archives.constFind(key);
                found != archives.cend()) {
                if (auto cached = found->lock()) {
                    retainNestedArchive(key, cached);
                    return cached;
                }
            }
        }

        auto opened = parent->openNested(index);
        if (!opened) {
            throw std::runtime_error(
              QStringLiteral(
                "Nested ZIP is not directly seekable and is unsupported: %1. "
                "Repack the outer ZIP with nested .zip entries set to Store "
                "or no compression.")
                .arg(support::pathToQString(virtualPath))
                .toStdString());
        }

        auto lock = std::scoped_lock{ cacheMutex };
        if (const auto found = archives.constFind(key);
            found != archives.cend()) {
            if (auto cached = found->lock()) {
                retainNestedArchive(key, cached);
                return cached;
            }
        }
        archives.insert(key, opened);
        retainNestedArchive(key, opened);
        return opened;
    }

    [[nodiscard]] auto materializeEntry(
      const std::shared_ptr<IndexedZipArchive>& archive,
      const zip_uint64_t index,
      const std::filesystem::path& virtualPath,
      const std::atomic_bool* stop = nullptr) const -> std::filesystem::path
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
            return *target;
        }

        auto temporary =
          QTemporaryFile{ support::pathToQString(materializationDirectory) +
                          QStringLiteral("/.extract-XXXXXX") };
        if (!temporary.open()) {
            throw std::runtime_error("Could not create temporary song asset");
        }
        archive->extract(index, temporary, stop);
        if (!temporary.flush()) {
            throw std::runtime_error("Could not flush extracted song asset");
        }
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

    [[nodiscard]] auto locateContainer(
      const std::filesystem::path& virtualDirectory,
      const std::atomic_bool* stop = nullptr) const
      -> std::optional<LocatedContainer>
    {
        throwIfCancelled(stop);
        const auto boundary = findPhysicalBoundary(virtualDirectory);
        if (!boundary ||
            (boundary->remainder.isEmpty() && !boundary->trailingSeparator)) {
            return std::nullopt;
        }

        auto archive = physicalArchive(boundary->physicalFile);
        auto virtualArchivePath = boundary->physicalFile;
        auto remaining = boundary->remainder;
        for (;;) {
            throwIfCancelled(stop);
            auto prefixes = QStringList{};
            for (auto index = remaining.indexOf('/'); index >= 0;
                 index = remaining.indexOf('/', index + 1)) {
                prefixes.push_back(remaining.left(index));
            }
            if (!remaining.isEmpty() && SongAssetStore::isArchivePath(
                                          support::qStringToPath(remaining))) {
                prefixes.push_back(remaining);
            }
            std::ranges::sort(prefixes,
                              [](const QString& left, const QString& right) {
                                  return left.size() > right.size();
                              });
            prefixes.removeDuplicates();

            auto selectedPrefix = QString{};
            auto selectedIndex = std::optional<zip_uint64_t>{};
            for (const auto& prefix : prefixes) {
                throwIfCancelled(stop);
                if (!SongAssetStore::isArchivePath(
                      support::qStringToPath(prefix))) {
                    continue;
                }
                const auto nestedVirtualPath =
                  support::qStringToPath(joinVirtual(
                    support::pathToQString(virtualArchivePath), prefix));
                const auto supportError =
                  SongAssetStore::archiveSupportError(nestedVirtualPath);
                if (!supportError.isEmpty()) {
                    throw std::runtime_error(supportError.toStdString());
                }
                if (const auto index = archive->findExact(prefix)) {
                    selectedPrefix = prefix;
                    selectedIndex = index;
                    break;
                }
            }
            if (!selectedIndex) {
                return LocatedContainer{ archive,
                                         virtualArchivePath,
                                         remaining };
            }

            virtualArchivePath = support::qStringToPath(joinVirtual(
              support::pathToQString(virtualArchivePath), selectedPrefix));
            archive =
              nestedArchive(archive, *selectedIndex, virtualArchivePath, stop);
            remaining.remove(0, selectedPrefix.size());
            while (remaining.startsWith('/')) {
                remaining.remove(0, 1);
            }
        }
    }

    [[nodiscard]] auto locateEntry(const std::filesystem::path& virtualPath,
                                   const std::atomic_bool* stop = nullptr) const
      -> std::optional<LocatedEntry>
    {
        const auto container =
          locateContainer(virtualPath.parent_path() / "", stop);
        if (!container) {
            return std::nullopt;
        }

        auto selected = std::optional<zip_uint64_t>{};
        const auto candidates =
          candidatePaths(support::pathToQString(virtualPath.filename()));
        for (const auto& candidate : candidates) {
            const auto internal = normalizeArchivePath(
              joinVirtual(container->internalDirectory, candidate));
            if (internal.isEmpty()) {
                continue;
            }
            if (const auto exact = container->archive->findExact(internal);
                exact && !container->archive->entry(*exact).encrypted) {
                selected = exact;
                break;
            }
        }
        if (!selected) {
            for (const auto& candidate : candidates) {
                const auto fallback = container->archive->findUniqueBasename(
                  QFileInfo(candidate).fileName());
                if (!fallback ||
                    container->archive->entry(*fallback).encrypted) {
                    continue;
                }
                selected = fallback;
                break;
            }
        }
        if (!selected) {
            return std::nullopt;
        }

        const auto& entry = container->archive->entry(*selected);
        return LocatedEntry{
            container->archive,
            *selected,
            support::qStringToPath(
              joinVirtual(support::pathToQString(container->virtualArchivePath),
                          entry.path))
        };
    }

  private:
    struct RetainedArchive
    {
        QString key;
        std::shared_ptr<IndexedZipArchive> archive;
    };

    void retainNestedArchive(
      const QString& key,
      const std::shared_ptr<IndexedZipArchive>& archive) const
    {
        for (auto retained = hotArchives.begin(); retained != hotArchives.end();
             ++retained) {
            if (retained->archive != archive) {
                continue;
            }
            hotArchives.erase(retained);
            break;
        }
        hotArchives.push_front({ key, archive });

        // Root ZIPs are retained separately, so even a huge outer central
        // directory cannot evict the active inner song pack. Keep a modest
        // number of recent inner packs to make selection and gameplay reuse
        // their indexes without retaining every pack visited by a full scan.
        constexpr auto hotArchiveLimit = size_t{ 32 };
        while (hotArchives.size() > hotArchiveLimit) {
            auto evicted = std::move(hotArchives.back());
            hotArchives.pop_back();
            if (evicted.archive.use_count() == 1) {
                archives.remove(evicted.key);
            }
        }
    }

    [[nodiscard]] static auto physicalArchiveKey(
      const std::filesystem::path& path) -> QString
    {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path, ec);
        if (ec) {
            throw std::filesystem::filesystem_error(
              "Could not inspect ZIP archive", path, ec);
        }
        const auto modified = std::filesystem::last_write_time(path, ec);
        if (ec) {
            throw std::filesystem::filesystem_error(
              "Could not inspect ZIP archive", path, ec);
        }
        return QStringLiteral("physical:%1:%2:%3")
          .arg(normalizedPhysicalPath(path),
               QString::number(static_cast<qulonglong>(size)),
               QString::number(modified.time_since_epoch().count()));
    }

    std::filesystem::path materializationDirectory;
    mutable std::mutex cacheMutex;
    mutable QHash<QString, RetainedArchive> physicalArchives;
    mutable QHash<QString, std::weak_ptr<IndexedZipArchive>> archives;
    mutable std::deque<RetainedArchive> hotArchives;
};

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
    impl = std::make_unique<Impl>(materializationDirectory);
}

SongAssetStore::~SongAssetStore() = default;

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
        std::shared_ptr<IndexedZipArchive> archive;
        std::shared_ptr<IndexedZipArchive> parent;
        std::optional<zip_uint64_t> parentIndex;
        std::filesystem::path virtualArchivePath;
        QString virtualPrefix;
    };
    auto pending = std::deque<PendingArchive>{};
    auto prefix = normalizedVirtualPath(archivePath);
    if (!prefix.endsWith('/')) {
        prefix += '/';
    }
    pending.push_back(
      { impl->physicalArchive(archivePath), {}, {}, archivePath, prefix });
    while (!pending.empty()) {
        if (stop && *stop) {
            return;
        }
        auto current = std::move(pending.front());
        pending.pop_front();
        if (!current.archive) {
            try {
                current.archive =
                  impl->nestedArchive(current.parent,
                                      *current.parentIndex,
                                      current.virtualArchivePath,
                                      stop);
            } catch (const std::exception& error) {
                spdlog::warn("Skipping nested archive {}: {}",
                             current.virtualPrefix.toStdString(),
                             error.what());
                continue;
            }
        }
        auto entryIndex = zip_uint64_t{};
        for (const auto& entry : current.archive->allEntries()) {
            const auto currentEntryIndex = entryIndex++;
            if (stop && *stop) {
                return;
            }
            const auto& relative = entry.path;
            if (entry.directory || relative.isEmpty()) {
                continue;
            }
            const auto virtualPath = current.virtualPrefix + relative;
            if (entry.encrypted) {
                spdlog::warn("Encrypted archive entry is unsupported: {}",
                             virtualPath.toStdString());
                continue;
            }
            if (isSplitArchivePath(support::qStringToPath(relative))) {
                spdlog::error(
                  "{}",
                  archiveSupportError(support::qStringToPath(virtualPath))
                    .toStdString());
                continue;
            }
            if (isArchivePath(support::qStringToPath(relative))) {
                const auto nestedVirtualPath =
                  support::qStringToPath(virtualPath);
                const auto supportError =
                  archiveSupportError(nestedVirtualPath);
                if (!supportError.isEmpty()) {
                    spdlog::error("{}", supportError.toStdString());
                    continue;
                }
                pending.push_front({ {},
                                     current.archive,
                                     currentEntryIndex,
                                     nestedVirtualPath,
                                     virtualPath + '/' });
                continue;
            }

            auto data = std::optional<QByteArray>{};
            const auto fsVirtualPath = support::qStringToPath(virtualPath);
            if (wantsContents && wantsContents(fsVirtualPath)) {
                data = current.archive->read(currentEntryIndex, stop);
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

    const auto container = impl->locateContainer(virtualDirectory, stop);
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
    auto uniquePaths = std::unordered_set<std::filesystem::path>{};
    for (const auto& virtualPath : virtualPaths) {
        throwIfCancelled(stop);
        if (virtualPath.empty() || !uniquePaths.insert(virtualPath).second ||
            existingMaterialization(materializationDirectory, virtualPath)) {
            continue;
        }
        const auto located = impl->locateEntry(virtualPath, stop);
        if (!located) {
            continue;
        }
        const auto local = impl->materializeEntry(
          located->archive, located->index, located->virtualPath, stop);
        recordMaterializationResolution(
          materializationDirectory, virtualPath, local);
    }
}

auto
SongAssetStore::read(const std::filesystem::path& virtualPath,
                     const std::atomic_bool* stop) const -> QByteArray
{
    throwIfCancelled(stop);
    if (isArchived(virtualPath)) {
        const auto located = impl->locateEntry(virtualPath, stop);
        if (!located) {
            throw std::runtime_error(QStringLiteral("Song asset not found: %1")
                                       .arg(support::pathToQString(virtualPath))
                                       .toStdString());
        }
        return located->archive->read(located->index, stop);
    }

    auto localPath = virtualPath;
    auto error = std::error_code{};
    if (!std::filesystem::is_regular_file(localPath, error)) {
        const auto resolved = resolveLocalAssets(virtualPath.parent_path(),
                                                 { virtualPath.filename() });
        const auto found = resolved.find(virtualPath.filename());
        if (found == resolved.end()) {
            throw std::runtime_error(QStringLiteral("Song asset not found: %1")
                                       .arg(support::pathToQString(virtualPath))
                                       .toStdString());
        }
        localPath = found->second;
    }
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
