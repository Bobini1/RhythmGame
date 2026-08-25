#include "gameplay_logic/ChartData.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/SongFolderFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongAssetImageProvider.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/SongDbScanner.h"
#include "sounds/AudioEngine.h"
#include "sounds/AudioPlayer.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include "../findTestAssetsFolder.h"

#include <zip.h>
#include <zlib.h>

#include <QBuffer>
#include <QDataStream>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSet>
#include <QTemporaryDir>
#include <QUrl>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <future>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct ZipWriteDeleter
{
    void operator()(zip_t* value) const
    {
        if (value) {
            zip_discard(value);
        }
    }
};

using ZipWriter = std::unique_ptr<zip_t, ZipWriteDeleter>;

auto
openZipWriter(const std::filesystem::path& target) -> ZipWriter
{
    auto error = zip_error_t{};
    zip_error_init(&error);
#ifdef _WIN32
    auto* source =
      zip_source_win32w_create(target.c_str(), 0, ZIP_LENGTH_TO_END, &error);
#else
    auto* source =
      zip_source_file_create(target.c_str(), 0, ZIP_LENGTH_TO_END, &error);
#endif
    INFO(zip_error_strerror(&error));
    REQUIRE(source);
    auto writer = ZipWriter{ zip_open_from_source(
      source, ZIP_CREATE | ZIP_TRUNCATE, &error) };
    if (!writer) {
        zip_source_free(source);
    }
    INFO(zip_error_strerror(&error));
    REQUIRE(writer);
    zip_error_fini(&error);
    return writer;
}

void
writeZip(const std::filesystem::path& target,
         const std::vector<std::pair<std::string, QByteArray>>& files,
         const zip_flags_t nameFlags = ZIP_FL_ENC_UTF_8,
         const bool storeNestedArchives = true)
{
    if (files.empty()) {
        auto file = QFile{ support::pathToQString(target) };
        REQUIRE(file.open(QIODevice::WriteOnly));
        const auto emptyZip =
          QByteArray::fromHex("504b0506000000000000000000000000000000000000");
        REQUIRE(file.write(emptyZip) == emptyZip.size());
        return;
    }
    auto writer = openZipWriter(target);
    for (const auto& [path, contents] : files) {
        auto* source =
          zip_source_buffer(writer.get(),
                            contents.constData(),
                            static_cast<zip_uint64_t>(contents.size()),
                            0);
        REQUIRE(source);
        const auto index =
          zip_file_add(writer.get(), path.c_str(), source, nameFlags);
        if (index < 0) {
            zip_source_free(source);
        }
        INFO(zip_strerror(writer.get()));
        REQUIRE(index >= 0);
        const auto nestedZip =
          path.size() >= 4 && QString::fromStdString(path).endsWith(
                                QStringLiteral(".zip"), Qt::CaseInsensitive);
        const auto method =
          storeNestedArchives && nestedZip ? ZIP_CM_STORE : ZIP_CM_DEFLATE;
        REQUIRE(zip_set_file_compression(
                  writer.get(), static_cast<zip_uint64_t>(index), method, 0) ==
                0);
    }
    auto* archive = writer.release();
    const auto result = zip_close(archive);
    if (result != 0) {
        const auto error = QString::fromUtf8(zip_strerror(archive));
        zip_discard(archive);
        FAIL(error.toStdString());
    }
}

void
writeSevenZip(const std::filesystem::path& target,
              const std::vector<std::pair<std::string, QByteArray>>&)
{
    auto file = QFile{ support::pathToQString(target) };
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(QByteArray::fromHex("377abcaf271c0004")) == 8);
}

auto
readFile(const std::filesystem::path& path) -> QByteArray
{
    auto file = QFile{ support::pathToQString(path) };
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

void
writeStoredZipWithRawName(const std::filesystem::path& archivePath,
                          const QByteArrayView name,
                          const QByteArrayView contents,
                          const quint16 flags = 0)
{
    const auto crc = static_cast<quint32>(
      crc32(0,
            reinterpret_cast<const Bytef*>(contents.data()),
            static_cast<uInt>(contents.size())));
    auto archive = QByteArray{};
    auto stream = QDataStream{ &archive, QIODevice::WriteOnly };
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32{ 0x04034b50 } << quint16{ 20 } << flags << quint16{ 0 }
           << quint16{ 0 } << quint16{ 0 } << crc
           << quint32{ static_cast<quint32>(contents.size()) }
           << quint32{ static_cast<quint32>(contents.size()) }
           << quint16{ static_cast<quint16>(name.size()) } << quint16{ 0 };
    REQUIRE(stream.writeRawData(name.data(), name.size()) == name.size());
    REQUIRE(stream.writeRawData(contents.data(), contents.size()) ==
            contents.size());

    const auto centralOffset = static_cast<quint32>(archive.size());
    stream << quint32{ 0x02014b50 } << quint16{ 20 } << quint16{ 20 } << flags
           << quint16{ 0 } << quint16{ 0 } << quint16{ 0 } << crc
           << quint32{ static_cast<quint32>(contents.size()) }
           << quint32{ static_cast<quint32>(contents.size()) }
           << quint16{ static_cast<quint16>(name.size()) } << quint16{ 0 }
           << quint16{ 0 } << quint16{ 0 } << quint16{ 0 } << quint32{ 0 }
           << quint32{ 0 };
    REQUIRE(stream.writeRawData(name.data(), name.size()) == name.size());
    const auto centralSize =
      static_cast<quint32>(archive.size()) - centralOffset;
    stream << quint32{ 0x06054b50 } << quint16{ 0 } << quint16{ 0 }
           << quint16{ 1 } << quint16{ 1 } << centralSize << centralOffset
           << quint16{ 0 };

    auto file = QFile{ support::pathToQString(archivePath) };
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    REQUIRE(file.write(archive) == archive.size());
}

auto
makePng(const QSize size = QSize{ 2, 3 }) -> QByteArray
{
    auto image = QImage{ size, QImage::Format_RGBA8888 };
    image.fill(Qt::magenta);
    auto contents = QByteArray{};
    auto buffer = QBuffer{ &contents };
    REQUIRE(buffer.open(QIODevice::WriteOnly));
    REQUIRE(image.save(&buffer, "PNG"));
    return contents;
}

void
ensureGuiApplication()
{
    if (QCoreApplication::instance()) {
        return;
    }
    static auto argc = 1;
    static auto executable = QByteArray{ "SongAssetStore.test" };
    static char* argv[] = { executable.data(), nullptr };
    static const auto application =
      std::make_unique<QGuiApplication>(argc, argv);
}

} // namespace

TEST_CASE("SongAssetStore traverses and resolves nested song archives")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song1.zip";
    const auto outer = root / "collection1.zip";
    const auto chart =
      QByteArray{ "#TITLE Nested archive\n#BPM 120\n#WAV01 keys/sound.ogg\n" };
    const auto sound = QByteArray{ "sound bytes" };
    const auto banner = QByteArray{ "image bytes" };

    writeZip(inner,
             { { "song/song1.bms", chart },
               { "song/keys/SOUND.WAV", sound },
               { "song/BANNER.PNG", banner },
               { "song/readme.txt", QByteArray{ "readme" } } });
    writeZip(outer, { { "set/song1.zip", readFile(inner) } });

    auto store = resource_managers::SongAssetStore{};
    auto discovered =
      std::vector<resource_managers::SongAssetStore::ArchiveEntry>{};
    store.walkArchive(
      outer,
      [](const std::filesystem::path& path) {
          return path.extension() == ".bms";
      },
      [&discovered](auto entry) { discovered.push_back(std::move(entry)); });

    const auto virtualChart =
      outer / "set" / "song1.zip" / "song" / "song1.bms";
    REQUIRE(discovered.size() == 4);
    const auto chartEntry =
      std::ranges::find_if(discovered, [](const auto& entry) {
          return entry.virtualPath.extension() == ".bms";
      });
    REQUIRE(chartEntry != discovered.end());
    CHECK(chartEntry->virtualPath == virtualChart);
    REQUIRE(chartEntry->contents);
    CHECK(*chartEntry->contents == chart);
    CHECK(store.read(virtualChart) == chart);

    const auto virtualDirectory = virtualChart.parent_path() / "";
    const auto requestedSound = std::filesystem::path{ "keys/sound.ogg" };
    const auto requestedBanner = std::filesystem::path{ "banner" };
    const auto materialized = store.materializeRelative(
      virtualDirectory, { requestedSound, requestedBanner });
    REQUIRE(materialized.contains(requestedSound));
    REQUIRE(materialized.contains(requestedBanner));
    CHECK(readFile(materialized.at(requestedSound)) == sound);
    CHECK(readFile(materialized.at(requestedBanner)) == banner);
    auto materializedExtensions = QSet<QString>{};
    for (const auto& entry : std::filesystem::directory_iterator(
           materialized.at(requestedSound).parent_path())) {
        if (entry.is_regular_file()) {
            materializedExtensions.insert(
              support::pathToQString(entry.path().extension()).toLower());
        }
    }
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".bms")));
    const auto repeatedBanner =
      store.materializeRelative(virtualDirectory, { requestedBanner });
    REQUIRE(repeatedBanner.contains(requestedBanner));
    CHECK(repeatedBanner.at(requestedBanner) ==
          materialized.at(requestedBanner));

    const auto imageUrl =
      resource_managers::SongAssetStore::imageUrl(virtualChart);
    CHECK(resource_managers::SongAssetStore::pathFromUrl(imageUrl) ==
          virtualChart);
    const auto audioUrl =
      resource_managers::SongAssetStore::audioUrl(virtualChart);
    CHECK(resource_managers::SongAssetStore::isAudioUrl(audioUrl));
    CHECK(resource_managers::SongAssetStore::pathFromUrl(audioUrl) ==
          virtualChart);
}

TEST_CASE("SongAssetImageProvider loads an archived image through QML")
{
    ensureGuiApplication();
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / L"東方音弾遊戯7.zip";
    const auto virtualImage = archivePath / "song" / "stage.png";
    writeZip(archivePath,
             { { "song/stage.png", makePng() },
               { "song/video.mp4", QByteArray{ "video" } } });

    auto store = resource_managers::SongAssetStore{};
    auto engine = QQmlEngine{};
    engine.addImageProvider(
      QStringLiteral("song-assets"),
      new resource_managers::SongAssetImageProvider{ &store });
    auto component = QQmlComponent{ &engine };
    component.setData(QByteArrayLiteral("import QtQuick\n"
                                        "Image {\n"
                                        "    asynchronous: true\n"
                                        "    sourceSize.width: 2\n"
                                        "    sourceSize.height: 3\n"
                                        "}\n"),
                      QUrl{});
    REQUIRE(component.isReady());
    auto image = std::unique_ptr<QObject>{ component.create() };
    REQUIRE(image);
    image->setProperty(
      "source",
      QUrl{ resource_managers::SongAssetStore::imageUrl(virtualImage) });

    auto elapsed = QElapsedTimer{};
    elapsed.start();
    constexpr auto imageReady = 1;
    constexpr auto imageLoading = 2;
    while (image->property("status").toInt() == imageLoading &&
           elapsed.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    CHECK(image->property("status").toInt() == imageReady);

    const auto materializedVideo =
      store.materialize(archivePath / "song" / "video.mp4");
    auto materializedExtensions = QSet<QString>{};
    for (const auto& entry :
         std::filesystem::directory_iterator(materializedVideo.parent_path())) {
        if (entry.is_regular_file()) {
            materializedExtensions.insert(
              support::pathToQString(entry.path().extension()).toLower());
        }
    }
    CHECK(materializedExtensions.contains(QStringLiteral(".mp4")));
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".png")));
}

TEST_CASE("AudioPlayer loads an archived preview without a disk copy")
{
    ensureGuiApplication();
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto previewPath = findTestAssetsFolder() / "supportedSoundFormats" /
                             "audiocheck.net_sin_1000Hz_-3dBFS_0.2s_44.1k.ogg";
    auto previewFile = QFile{ support::pathToQString(previewPath) };
    REQUIRE(previewFile.open(QIODevice::ReadOnly));
    writeZip(archivePath,
             { { "song/preview.ogg", previewFile.readAll() },
               { "song/video.mp4", QByteArray{ "video" } } });

    auto store = resource_managers::SongAssetStore{};
    auto audioEngine = sounds::AudioEngine{};
    sounds::AudioPlayer::engine = &audioEngine;
    sounds::AudioPlayer::assetStore = &store;
    {
        auto player = sounds::AudioPlayer{};
        player.setSource(resource_managers::SongAssetStore::audioUrl(
          archivePath / "song" / "preview.ogg"));

        auto elapsed = QElapsedTimer{};
        elapsed.start();
        while (!player.isLoaded() && elapsed.elapsed() < 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        CHECK(player.isLoaded());
        CHECK(player.playOverlapping());

        const auto materializedVideo =
          store.materialize(archivePath / "song" / "video.mp4");
        auto materializedExtensions = QSet<QString>{};
        for (const auto& entry : std::filesystem::directory_iterator(
               materializedVideo.parent_path())) {
            if (entry.is_regular_file()) {
                materializedExtensions.insert(
                  support::pathToQString(entry.path().extension()).toLower());
            }
        }
        CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".ogg")));
    }
    sounds::AudioPlayer::assetStore = nullptr;
    sounds::AudioPlayer::engine = nullptr;
}

TEST_CASE("SongAssetStore rejects compressed nested ZIP entries")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song.zip";
    const auto outer = root / "collection.zip";
    const auto chart = QByteArray{ "#TITLE Compressed nested ZIP\n" };
    writeZip(inner, { { "song/chart.bms", chart } });
    const auto innerBytes = readFile(inner);
    writeZip(
      outer, { { "packs/song.zip", innerBytes } }, ZIP_FL_ENC_UTF_8, false);

    auto store = resource_managers::SongAssetStore{};
    CHECK_THROWS_AS(
      store.read(outer / "packs" / "song.zip" / "song" / "chart.bms"),
      std::runtime_error);
}

TEST_CASE(
  "SongAssetStore skips compressed nested ZIPs without temporary extraction")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song.zip";
    const auto outer = root / "collection.zip";
    writeZip(inner,
             { { "song/nested.bms",
                 QByteArray{ "#TITLE Compressed nested ZIP\n" } } });
    writeZip(outer,
             { { "packs/song.zip", readFile(inner) },
               { "direct.bms", QByteArray{ "#TITLE Direct\n" } },
               { "marker.mp4", QByteArray{ "marker" } } },
             ZIP_FL_ENC_UTF_8,
             false);

    auto store = resource_managers::SongAssetStore{};
    const auto marker = store.materialize(outer / "marker.mp4");
    const auto countTemporaryZips = [&marker] {
        auto count = size_t{};
        for (const auto& entry :
             std::filesystem::directory_iterator(marker.parent_path())) {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") {
                ++count;
            }
        }
        return count;
    };
    auto charts = std::vector<std::filesystem::path>{};
    store.walkArchive(
      outer,
      [](const auto& path) { return path.extension() == ".bms"; },
      [&charts](auto entry) {
          if (entry.contents) {
              charts.push_back(std::move(entry.virtualPath));
          }
      });

    REQUIRE(charts.size() == 1);
    CHECK(charts.front() == outer / "direct.bms");
    CHECK(countTemporaryZips() == 0);
}

TEST_CASE("SongAssetStore decodes legacy CP932 ZIP entry names")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "legacy.zip";
    const auto chart = QByteArray{ "#TITLE Legacy name\n" };
    const auto rawPath = QByteArray{ "\x93\x8c\x95\xfb/chart.bms", 14 };
    writeStoredZipWithRawName(archivePath, rawPath, chart);

    auto store = resource_managers::SongAssetStore{};
    auto charts = std::vector<std::filesystem::path>{};
    store.walkArchive(
      archivePath,
      [](const auto& path) { return path.extension() == ".bms"; },
      [&charts](auto entry) {
          if (entry.contents) {
              charts.push_back(std::move(entry.virtualPath));
          }
      });

    const auto expected = archivePath / L"東方" / "chart.bms";
    REQUIRE(charts.size() == 1);
    CHECK(charts.front() == expected);
    CHECK(store.read(expected) == chart);
}

TEST_CASE("SongAssetStore uses the ZIP UTF-8 flag to disambiguate entry names")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto rawPath = QByteArray{ "\xC2\xA1/chart.bms", 12 };
    const auto chart = QByteArray{ "#TITLE Ambiguous name\n" };

    const auto legacyArchive = root / "legacy-ambiguous.zip";
    writeStoredZipWithRawName(legacyArchive, rawPath, chart);
    auto store = resource_managers::SongAssetStore{};
    const auto legacyDirectory = support::qStringToPath(QStringLiteral("ﾂ｡"));
    CHECK(store.read(legacyArchive / legacyDirectory / "chart.bms") == chart);

    const auto utf8Archive = root / "utf8-ambiguous.zip";
    writeStoredZipWithRawName(utf8Archive, rawPath, chart, 0x0800);
    const auto utf8Directory = support::qStringToPath(QStringLiteral("¡"));
    CHECK(store.read(utf8Archive / utf8Directory / "chart.bms") == chart);
}

TEST_CASE("SongAssetStore keeps a large stored nested pack directly browsable")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto outer = root / "collection.zip";
    const auto stage = makePng();
    auto nestedPacks = std::vector<std::pair<std::string, QByteArray>>{};
    constexpr auto packCount = 64;
    nestedPacks.reserve(packCount);
    for (auto index = 0; index < packCount; ++index) {
        const auto name = QStringLiteral("pack-%1").arg(index, 3, 10, u'0');
        const auto inner = root / support::qStringToPath(name + ".zip");
        writeZip(inner,
                 { { "song/chart.bms", QByteArray{ "#TITLE Pack\n" } },
                   { "song/stage.png", stage },
                   { "song/preview.ogg", QByteArray{ "preview" } },
                   { "song/readme.txt", QByteArray{ "readme" } } });
        nestedPacks.emplace_back(
          (QStringLiteral("packs/") + name + QStringLiteral(".zip"))
            .toStdString(),
          readFile(inner));
    }
    writeZip(outer, nestedPacks);

    auto store = resource_managers::SongAssetStore{};
    auto chartCount = 0;
    store.walkArchive(
      outer,
      [](const auto& path) { return path.extension() == ".bms"; },
      [&chartCount](auto entry) {
          if (entry.contents) {
              ++chartCount;
          }
      });
    REQUIRE(chartCount == packCount);

    const auto selectedDirectory =
      outer / "packs" / "pack-063.zip" / "song" / "";
    const auto requestedStage = std::filesystem::path{ "stage" };
    const auto requestedPreview = std::filesystem::path{ "preview.ogg" };
    const auto assets = store.materializeRelative(
      selectedDirectory, { requestedStage, requestedPreview });
    REQUIRE(assets.contains(requestedStage));
    REQUIRE(assets.contains(requestedPreview));
    CHECK(readFile(assets.at(requestedStage)) == stage);
    CHECK(readFile(assets.at(requestedPreview)) == QByteArray{ "preview" });

    for (const auto& entry : std::filesystem::directory_iterator(
           assets.at(requestedStage).parent_path())) {
        CHECK(entry.path().extension() != ".zip");
    }
}

TEST_CASE("SongAssetStore recognizes split archive names")
{
    CHECK(
      resource_managers::SongAssetStore::isSplitArchivePath("songs.7z.001"));
    CHECK(
      resource_managers::SongAssetStore::isSplitArchivePath("songs.zip.001"));
    CHECK(resource_managers::SongAssetStore::isSplitArchivePath(
      "songs.part12.rar"));
    CHECK(resource_managers::SongAssetStore::isSplitArchivePath("songs.z01"));
    CHECK(resource_managers::SongAssetStore::isSplitArchivePath("songs.r00"));
    CHECK_FALSE(
      resource_managers::SongAssetStore::isSplitArchivePath("songs.zip"));
}

TEST_CASE("SongAssetStore supports ZIP archive containers only")
{
    CHECK(resource_managers::SongAssetStore::isArchivePath("songs.zip"));
    CHECK(
      resource_managers::SongAssetStore::isSupportedArchivePath("songs.zip"));

    for (const auto& path : { "songs.7z", "songs.rar", "songs.tar" }) {
        CHECK(resource_managers::SongAssetStore::isArchivePath(path));
        CHECK_FALSE(
          resource_managers::SongAssetStore::isSupportedArchivePath(path));
    }
}

TEST_CASE("SongAssetStore rejects non-ZIP data disguised as ZIP")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    auto store = resource_managers::SongAssetStore{};

    SECTION("with entries")
    {
        const auto archivePath = root / "disguised.zip";
        writeSevenZip(
          archivePath,
          { { "song/chart.bms", QByteArray{ "#TITLE Unsupported\n" } } });
        CHECK_THROWS(store.walkArchive(
          archivePath, [](const auto&) { return true; }, [](auto) {}));
    }

    SECTION("when empty")
    {
        const auto archivePath = root / "empty-disguised.zip";
        writeSevenZip(archivePath, {});
        CHECK_THROWS(store.walkArchive(
          archivePath, [](const auto&) { return true; }, [](auto) {}));
    }

    SECTION("without rejecting an empty ZIP")
    {
        const auto archivePath = root / "empty.zip";
        writeZip(archivePath, {});
        CHECK_NOTHROW(store.walkArchive(
          archivePath, [](const auto&) { return true; }, [](auto) {}));
    }
}

TEST_CASE("SongAssetStore skips nested non-ZIP archives")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song.7z";
    const auto outer = root / "collection.zip";
    writeZip(inner,
             { { "song/chart.bms", QByteArray{ "#TITLE Unsupported\n" } } });
    writeZip(outer, { { "set/song.7z", readFile(inner) } });

    auto store = resource_managers::SongAssetStore{};
    auto charts = std::vector<std::filesystem::path>{};
    CHECK_NOTHROW(store.walkArchive(
      outer,
      [](const std::filesystem::path& path) {
          return path.extension() == ".bms";
      },
      [&charts](auto entry) {
          if (entry.contents) {
              charts.push_back(std::move(entry.virtualPath));
          }
      }));
    CHECK(charts.empty());
}

TEST_CASE("SongAssetStore resolves a uniquely named shared archive asset")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto image = makePng();
    writeZip(archivePath, { { "shared/_title.png", image } });

    auto store = resource_managers::SongAssetStore{};
    const auto requested = archivePath / "song" / "_title.png";
    CHECK(readFile(store.materialize(requested)) == image);
}

TEST_CASE("SongAssetStore removes temporary materializations on destruction")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto virtualPath = archivePath / "song" / "preview.ogg";
    writeZip(archivePath, { { "song/preview.ogg", QByteArray{ "preview" } } });

    auto materialized = std::filesystem::path{};
    {
        auto store = resource_managers::SongAssetStore{};
        materialized = store.materialize(virtualPath);
        REQUIRE(std::filesystem::is_regular_file(materialized));
    }
    CHECK_FALSE(std::filesystem::exists(materialized));
}

TEST_CASE(
  "SongAssetStore materialization identity follows source and archive revision")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto firstArchive = root / "first.zip";
    const auto secondArchive = root / "second.zip";
    const auto relative = std::filesystem::path{ "song/banner.png" };
    const auto original = QByteArray{ "same image bytes" };

    writeZip(firstArchive, { { "song/banner.png", original } });
    writeZip(secondArchive, { { "song/banner.png", original } });

    auto store = resource_managers::SongAssetStore{};
    const auto first = store.materialize(firstArchive / relative);
    const auto repeated = store.materialize(firstArchive / relative);
    const auto sameContentsOtherArchive =
      store.materialize(secondArchive / relative);

    CHECK(repeated == first);
    CHECK(sameContentsOtherArchive != first);
    CHECK(readFile(first) == original);
    CHECK(readFile(sameContentsOtherArchive) == original);
    CHECK(support::pathToQString(first.filename()).startsWith("source-"));

    auto cancelled = std::atomic_bool{ true };
    auto cancelledRequestThrew = false;
    try {
        static_cast<void>(
          store.materialize(firstArchive / relative, &cancelled));
    } catch (const std::runtime_error&) {
        cancelledRequestThrew = true;
    }
    CHECK(cancelledRequestThrew);

    const auto replacement = QByteArray{ "replacement image bytes are newer" };
    const auto replacementArchive = root / "replacement.zip";
    writeZip(replacementArchive, { { "song/banner.png", replacement } });
    const auto replacementBytes = readFile(replacementArchive);
    auto revisedArchive = QFile{ support::pathToQString(firstArchive) };
    REQUIRE(revisedArchive.open(QIODevice::WriteOnly | QIODevice::Truncate));
    REQUIRE(revisedArchive.write(replacementBytes) == replacementBytes.size());
    revisedArchive.close();
    const auto revised = store.materialize(firstArchive / relative);

    CHECK(revised != first);
    CHECK(readFile(revised) == replacement);
}

TEST_CASE("SongAssetStore shares concurrent materialization")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto virtualPath = archivePath / "song" / "preview.ogg";
    const auto preview = QByteArray(1024 * 1024, 'p');
    writeZip(archivePath, { { "song/preview.ogg", preview } });

    auto store = resource_managers::SongAssetStore{};
    auto requests = std::vector<std::future<std::filesystem::path>>{};
    for (auto index = 0; index < 8; ++index) {
        requests.push_back(
          std::async(std::launch::async, [&store, virtualPath] {
              return store.materialize(virtualPath);
          }));
    }

    auto materialized = std::filesystem::path{};
    for (auto& request : requests) {
        const auto result = request.get();
        if (materialized.empty()) {
            materialized = result;
        }
        CHECK(result == materialized);
    }
    CHECK(readFile(materialized) == preview);

    for (const auto& entry :
         std::filesystem::directory_iterator(materialized.parent_path())) {
        CHECK_FALSE(support::pathToQString(entry.path().filename())
                      .startsWith(".extract-"));
    }
}

TEST_CASE("SongAssetStore does not impose an arbitrary nesting depth")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    auto current = root / "level6.zip";
    writeZip(current, { { "chart.bms", QByteArray{ "#TITLE Deep\n" } } });

    for (auto level = 5; level >= 0; --level) {
        const auto parent = root / ("level" + std::to_string(level) + ".zip");
        writeZip(parent,
                 { { support::pathToQString(current.filename()).toStdString(),
                     readFile(current) } });
        current = parent;
    }

    auto store = resource_managers::SongAssetStore{};
    auto charts = std::vector<std::filesystem::path>{};
    store.walkArchive(
      current,
      [](const std::filesystem::path& path) {
          return path.extension() == ".bms";
      },
      [&charts](auto entry) {
          if (entry.contents) {
              charts.push_back(std::move(entry.virtualPath));
          }
      });

    REQUIRE(charts.size() == 1);
    auto expected = current;
    for (auto level = 1; level <= 6; ++level) {
        expected /= "level" + std::to_string(level) + ".zip";
    }
    expected /= "chart.bms";
    CHECK(charts.front() == expected);
}

TEST_CASE(
  "SongDbScanner indexes charts and directory assets in nested archives")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song1.zip";
    const auto outer = root / L"東方音弾遊戯7.zip";
    const auto chart = QByteArray{ "#PLAYER 1\n"
                                   "#TITLE Nested scanner chart\n"
                                   "#ARTIST Test\n"
                                   "#BPM 120\n"
                                   "#PLAYLEVEL 1\n"
                                   "#RANK 2\n"
                                   "#TOTAL 100\n"
                                   "#WAV01 sound.wav\n"
                                   "#STAGEFILE _title.png\n"
                                   "#BANNER banner.png\n"
                                   "#BACKBMP background.bmp\n"
                                   "#00111:01\n" };
    const auto stageFile = makePng();
    writeZip(
      inner,
      { { "song/Magus Logos/song1.bms", chart },
        { "song/Magus Logos/backup/duplicate.bms", chart },
        { "song/Magus Logos/_title.png", stageFile },
        { "song/Magus Logos/background.bmp", QByteArray{ "background" } },
        { "song/Magus Logos/preview.ogg", QByteArray{ "preview" } },
        { "song/Magus Logos/readme.txt", QByteArray{ "readme" } },
        { "song/Magus Logos/unused.wav", QByteArray{ "unused" } },
        { "extras/preview.ogg", QByteArray{ "unrelated" } } });
    writeZip(outer, { { "set/song1.zip", readFile(inner) } });
    REQUIRE(std::filesystem::remove(inner));

    auto database = db::SqliteCppDb{ root / "songs.sqlite" };
    resource_managers::defineDb(database);
    auto store = resource_managers::SongAssetStore{};
    auto scanner = resource_managers::SongDbScanner{ &database, &store };
    auto stopped = std::atomic_bool{ false };
    scanner.scanDirectory(root, [](const QString&) {}, &stopped);

    auto charts =
      database
        .createStatement("SELECT path, chart_directory, title FROM charts")
        .executeAndGetAll<std::tuple<std::string, std::string, std::string>>();
    REQUIRE(charts.size() == 1);
    const auto expectedDirectory =
      support::pathToQString(outer / "set" / "song1.zip" / "song" /
                             "Magus Logos" / "")
        .toStdString();
    CHECK(std::get<0>(charts.front()) ==
          support::pathToQString(outer / "set" / "song1.zip" / "song" /
                                 "Magus Logos" / "song1.bms")
            .toStdString());
    CHECK(std::get<1>(charts.front()) == expectedDirectory);
    CHECK(std::get<2>(charts.front()) == "Nested scanner chart");

    const auto expectedListingDirectory =
      support::pathToQString(outer / "set" / "song1.zip" / "song" / "")
        .toStdString();
    auto listedIn =
      database
        .createStatement("SELECT pd.dir "
                         "FROM charts c "
                         "JOIN parent_dir pd ON pd.id = c.directory")
        .executeAndGet<std::string>();
    REQUIRE(listedIn);
    CHECK(*listedIn == expectedListingDirectory);

    auto folderFactory = qml_components::SongFolderFactory{ &database };
    const auto folder =
      folderFactory.open(QString::fromStdString(expectedListingDirectory));
    REQUIRE(folder.size() == 1);
    auto* chartData = qobject_cast<gameplay_logic::ChartData*>(
      folder.front().value<QObject*>());
    REQUIRE(chartData);
    CHECK(chartData->getStageFile() == QStringLiteral("_title.png"));
    const auto stageFileSource = chartData->getStageFileSource();
    CHECK(stageFileSource.startsWith(QStringLiteral("image://song-assets/")));
    const auto materializedStageFile = store.materialize(
      resource_managers::SongAssetStore::pathFromUrl(stageFileSource));
    auto stageFileReader =
      QImageReader{ support::pathToQString(materializedStageFile) };
    CHECK(stageFileReader.read().size() == QSize{ 2, 3 });

    auto materializedExtensions = QSet<QString>{};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
           materializedStageFile.parent_path())) {
        if (entry.is_regular_file()) {
            materializedExtensions.insert(
              support::pathToQString(entry.path().extension()).toLower());
        }
    }
    CHECK(materializedExtensions.contains(QStringLiteral(".png")));
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".zip")));
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".bmp")));
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".ogg")));
    CHECK_FALSE(materializedExtensions.contains(QStringLiteral(".txt")));
    delete chartData;

    const auto preview =
      database.createStatement("SELECT path, directory FROM preview_files")
        .executeAndGet<std::tuple<std::string, std::string>>();
    REQUIRE(preview);
    CHECK(std::get<1>(*preview) == expectedDirectory);
    CHECK(std::get<0>(*preview).ends_with("preview.ogg"));

    const auto readme =
      database.createStatement("SELECT path, directory FROM readme_files")
        .executeAndGet<std::tuple<std::string, std::string>>();
    REQUIRE(readme);
    CHECK(std::get<1>(*readme) == expectedDirectory);
    CHECK(std::get<0>(*readme).ends_with("readme.txt"));
}

TEST_CASE("SongDbScanner accepts an archive as a root song source")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto chart = QByteArray{ "#PLAYER 1\n"
                                   "#TITLE Archive root chart\n"
                                   "#ARTIST Test\n"
                                   "#BPM 120\n"
                                   "#PLAYLEVEL 1\n"
                                   "#RANK 2\n"
                                   "#TOTAL 100\n"
                                   "#WAV01 sound.wav\n"
                                   "#00111:01\n" };
    writeZip(archivePath,
             { { "song/chart.bms", chart },
               { "song/preview.ogg", QByteArray{ "preview" } },
               { "song/readme.txt", QByteArray{ "readme" } } });

    auto database = db::SqliteCppDb{ root / "songs.sqlite" };
    resource_managers::defineDb(database);
    auto store = resource_managers::SongAssetStore{};
    auto scanner = resource_managers::SongDbScanner{ &database, &store };
    auto stopped = std::atomic_bool{ false };
    scanner.scanDirectory(archivePath, [](const QString&) {}, &stopped);

    const auto expectedRoot =
      support::pathToQString(archivePath / "").toStdString();
    const auto expectedDirectory =
      support::pathToQString(archivePath / "song" / "").toStdString();
    auto rootFolder =
      database
        .createStatement("SELECT dir FROM parent_dir WHERE parent_dir IS NULL")
        .executeAndGet<std::string>();
    REQUIRE(rootFolder);
    CHECK(*rootFolder == expectedRoot);

    auto indexedChart =
      database
        .createStatement("SELECT path, chart_directory, title FROM charts")
        .executeAndGet<std::tuple<std::string, std::string, std::string>>();
    REQUIRE(indexedChart);
    CHECK(
      std::get<0>(*indexedChart) ==
      support::pathToQString(archivePath / "song" / "chart.bms").toStdString());
    CHECK(std::get<1>(*indexedChart) == expectedDirectory);
    CHECK(std::get<2>(*indexedChart) == "Archive root chart");

    auto listedIn =
      database
        .createStatement("SELECT pd.dir "
                         "FROM charts c "
                         "JOIN parent_dir pd ON pd.id = c.directory")
        .executeAndGet<std::string>();
    REQUIRE(listedIn);
    CHECK(*listedIn == expectedRoot);

    auto songFolderQuery =
      database.createStatement("SELECT COUNT(*) FROM parent_dir WHERE dir = ?");
    songFolderQuery.bind(1, expectedDirectory);
    auto songFolderCount = songFolderQuery.executeAndGet<int>();
    REQUIRE(songFolderCount);
    CHECK(*songFolderCount == 0);

    auto preview = database.createStatement("SELECT path FROM preview_files")
                     .executeAndGet<std::string>();
    REQUIRE(preview);
    CHECK(preview->ends_with("preview.ogg"));

    auto readme = database.createStatement("SELECT path FROM readme_files")
                    .executeAndGet<std::string>();
    REQUIRE(readme);
    CHECK(readme->ends_with("readme.txt"));
}

TEST_CASE("RootSongFolders accepts an archive URL from settings")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    writeZip(archivePath,
             { { "song/chart.bms", QByteArray{ "#TITLE Test\n" } } });

    auto database = db::SqliteCppDb{ root / "songs.sqlite" };
    resource_managers::defineDb(database);
    auto store = resource_managers::SongAssetStore{};
    auto scanner = resource_managers::SongDbScanner{ &database, &store };
    auto queue = qml_components::ScanningQueue{ &database, scanner };
    auto folders = qml_components::RootSongFolders{ &database, &queue };

    REQUIRE(folders.add(
      QUrl::fromLocalFile(support::pathToQString(archivePath)).toString()));
    REQUIRE(folders.rowCount() == 1);
    const auto configured =
      folders.at(0).value<qml_components::RootSongFolder*>();
    REQUIRE(configured);
    CHECK(configured->getName() ==
          QFileInfo{ support::pathToQString(archivePath) }.canonicalFilePath());
}

TEST_CASE("RootSongFolders rejects non-ZIP archives from settings")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.7z";
    writeZip(archivePath,
             { { "song/chart.bms", QByteArray{ "#TITLE Test\n" } } });

    auto database = db::SqliteCppDb{ root / "songs.sqlite" };
    resource_managers::defineDb(database);
    auto store = resource_managers::SongAssetStore{};
    auto scanner = resource_managers::SongDbScanner{ &database, &store };
    auto queue = qml_components::ScanningQueue{ &database, scanner };
    auto folders = qml_components::RootSongFolders{ &database, &queue };

    CHECK_FALSE(folders.add(
      QUrl::fromLocalFile(support::pathToQString(archivePath)).toString()));
    CHECK(folders.rowCount() == 0);
}
