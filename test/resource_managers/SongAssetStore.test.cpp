#include "gameplay_logic/ChartData.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/SongFolderFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongAssetImageProvider.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/SongDbScanner.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <archive.h>
#include <archive_entry.h>

#include <QBuffer>
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

struct ArchiveWriteDeleter
{
    void operator()(archive* value) const
    {
        if (value) {
            archive_write_close(value);
            archive_write_free(value);
        }
    }
};

using ArchiveWriter = std::unique_ptr<archive, ArchiveWriteDeleter>;

void
writeZip(const std::filesystem::path& target,
         const std::vector<std::pair<std::string, QByteArray>>& files)
{
    auto writer = ArchiveWriter{ archive_write_new() };
    REQUIRE(writer);
    REQUIRE(archive_write_set_format_zip(writer.get()) == ARCHIVE_OK);
#ifdef _WIN32
    REQUIRE(archive_write_open_filename_w(writer.get(), target.c_str()) ==
            ARCHIVE_OK);
#else
    REQUIRE(archive_write_open_filename(writer.get(), target.c_str()) ==
            ARCHIVE_OK);
#endif
    for (const auto& [path, contents] : files) {
        auto* entry = archive_entry_new();
        REQUIRE(entry);
        archive_entry_set_pathname(entry, path.c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, contents.size());
        REQUIRE(archive_write_header(writer.get(), entry) == ARCHIVE_OK);
        REQUIRE(archive_write_data(writer.get(),
                                   contents.constData(),
                                   contents.size()) == contents.size());
        archive_entry_free(entry);
    }
}

auto
readFile(const std::filesystem::path& path) -> QByteArray
{
    auto file = QFile{ support::pathToQString(path) };
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
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
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
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

    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
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
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / L"東方音弾遊戯7.zip";
    const auto virtualImage = archivePath / "song" / "stage.png";
    writeZip(archivePath, { { "song/stage.png", makePng() } });

    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
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

TEST_CASE("SongAssetStore resolves a uniquely named shared archive asset")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto image = makePng();
    writeZip(archivePath, { { "shared/_title.png", image } });

    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
    const auto requested = archivePath / "song" / "_title.png";
    CHECK(readFile(store.materialize(requested)) == image);
}

TEST_CASE("SongAssetStore cache identity follows source and archive revision")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto cache = support::qStringToPath(cacheDirectory.path());
    const auto firstArchive = root / "first.zip";
    const auto secondArchive = root / "second.zip";
    const auto relative = std::filesystem::path{ "song/banner.png" };
    const auto original = QByteArray{ "same image bytes" };

    writeZip(firstArchive, { { "song/banner.png", original } });
    writeZip(secondArchive, { { "song/banner.png", original } });

    auto store = resource_managers::SongAssetStore{ cache };
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
    CHECK_THROWS(store.materialize(firstArchive / relative, &cancelled));

    const auto replacement = QByteArray{ "replacement image bytes are newer" };
    writeZip(firstArchive, { { "song/banner.png", replacement } });
    const auto revised = store.materialize(firstArchive / relative);

    CHECK(revised != first);
    CHECK(readFile(revised) == replacement);
}

TEST_CASE("SongAssetStore evicts one root's archive cache on rescan")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto firstRoot = root / "first-root";
    const auto secondRoot = root / "second-root";
    REQUIRE(std::filesystem::create_directories(firstRoot));
    REQUIRE(std::filesystem::create_directories(secondRoot));
    const auto firstArchive = firstRoot / "songs.zip";
    const auto secondArchive = secondRoot / "songs.zip";
    const auto relative = std::filesystem::path{ "song/banner.png" };
    writeZip(firstArchive, { { "song/banner.png", QByteArray{ "first" } } });
    writeZip(secondArchive, { { "song/banner.png", QByteArray{ "second" } } });

    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
    const auto firstCached = store.materialize(firstArchive / relative);
    const auto secondCached = store.materialize(secondArchive / relative);
    CAPTURE(firstCached, secondCached);
    REQUIRE(std::filesystem::is_regular_file(firstCached));
    REQUIRE(std::filesystem::is_regular_file(secondCached));

    store.beginRescan(firstRoot);
    store.evictArchiveForRescan(firstRoot, firstArchive);
    CHECK_FALSE(std::filesystem::exists(firstCached));
    CHECK(std::filesystem::is_regular_file(secondCached));

    const auto recreated = store.materialize(firstArchive / relative);
    REQUIRE(std::filesystem::is_regular_file(recreated));
    store.beginRescan(firstRoot);
    CHECK_FALSE(std::filesystem::exists(recreated));
    CHECK(std::filesystem::is_regular_file(secondCached));
}

TEST_CASE("SongAssetStore shares concurrent materialization")
{
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    const auto virtualPath = archivePath / "song" / "preview.ogg";
    const auto preview = QByteArray(1024 * 1024, 'p');
    writeZip(archivePath, { { "song/preview.ogg", preview } });

    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
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

    for (const auto& entry : std::filesystem::directory_iterator(
           support::qStringToPath(cacheDirectory.path()))) {
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

    auto store = resource_managers::SongAssetStore{ root / "asset-cache" };
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
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto inner = root / "song1.zip";
    const auto outer = root / L"東方音弾遊戯7.7z";
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
    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
    auto scanner = resource_managers::SongDbScanner{ &database, &store };
    auto stopped = std::atomic_bool{ false };
    scanner.scanDirectory(root, [](const QString&) {}, &stopped);

    const auto unusedCached = store.materialize(
      outer / "set" / "song1.zip" / "song" / "Magus Logos" / "unused.wav");
    REQUIRE(std::filesystem::is_regular_file(unusedCached));
    scanner.scanDirectory(root, [](const QString&) {}, &stopped);
    CHECK_FALSE(std::filesystem::exists(unusedCached));

    auto cachedExtensions = QSet<QString>{};
    auto cachedOggFiles = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
           support::qStringToPath(cacheDirectory.path()))) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto extension =
          support::pathToQString(entry.path().extension()).toLower();
        cachedExtensions.insert(extension);
        if (extension == QStringLiteral(".ogg")) {
            ++cachedOggFiles;
        }
    }
    CHECK(cachedExtensions.contains(QStringLiteral(".png")));
    CHECK(cachedExtensions.contains(QStringLiteral(".bmp")));
    CHECK(cachedExtensions.contains(QStringLiteral(".ogg")));
    CHECK(cachedOggFiles == 1);
    CHECK(cachedExtensions.contains(QStringLiteral(".txt")));

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
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
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
    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
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
    auto cacheDirectory = QTemporaryDir{};
    REQUIRE(cacheDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto archivePath = root / "songs.zip";
    writeZip(archivePath,
             { { "song/chart.bms", QByteArray{ "#TITLE Test\n" } } });

    auto database = db::SqliteCppDb{ root / "songs.sqlite" };
    resource_managers::defineDb(database);
    auto store = resource_managers::SongAssetStore{ support::qStringToPath(
      cacheDirectory.path()) };
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
