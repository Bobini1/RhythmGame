#include "db/SqliteCppDb.h"
#include "gameplay_logic/ChartData.h"
#include "qml_components/SongFolderFactory.h"
#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/Tables.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include "support/FolderName.h"

#ifdef RHYTHMGAME_USE_BACKBEAT
#include "arena/QtArenaRoundLoader.h"
#include "resource_managers/BackbeatCatalog.h"
#include <backbeat.h>
#include <sqlite3.h>
#endif

#include <QFile>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QThread>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <functional>

namespace {

void
ensureCoreApplication()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "Backbeat.test";
        static char* argv[] = { name, nullptr };
        static QCoreApplication application(argc, argv);
    }
}

auto
waitUntil(const std::function<bool()>& predicate, int timeout = 6000) -> bool
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeout) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    return predicate();
}

const QByteArray chartBytes =
  "#PLAYER 1\n#TITLE Backbeat fixture\n#ARTIST Test\n#BPM 120\n"
  "#WAV01 sound.wav\n#STAGEFILE stage.png\n#00111:0100\n";

struct Library
{
    QTemporaryDir temporary;
    std::filesystem::path path =
      support::qStringToPath(temporary.filePath("songs.sqlite"));
    db::SqliteCppDb database{ path };

    Library()
    {
        ensureCoreApplication();
        resource_managers::defineDb(database);
    }

    auto save(const QString& chartPath, const QByteArray& bytes = chartBytes)
      -> std::unique_ptr<gameplay_logic::ChartData>
    {
        const resource_managers::ChartDataFactory factory;
        auto components = factory.loadChartData(
          std::string_view(bytes.constData(),
                           static_cast<size_t>(bytes.size())),
          support::qStringToPath(chartPath),
          [](auto) { return 1; },
          -1);
        components.chartData->save(database);
        return std::move(components.chartData);
    }

    auto count() -> int
    {
        return database.createStatement("SELECT count(*) FROM charts")
          .executeAndGet<int>()
          .value();
    }
};

void
deleteCharts(const QVariantList& charts)
{
    for (const auto& item : charts) {
        if (item.canConvert<gameplay_logic::ChartData*>()) {
            delete item.value<gameplay_logic::ChartData*>();
        }
    }
}

#ifdef RHYTHMGAME_USE_BACKBEAT
class Store : public resource_managers::BackbeatSource
{
  public:
    std::atomic<qint64> currentRevision = 1;
    mutable int reads = 0;
    mutable int searches = 0;
    mutable std::atomic_int revisionChecks = 0;
    QHash<QString, Bundle> installed;
    QList<Pack> installedPacks;
    QHash<QString, Asset> assets;
    bool unavailable = false;
    mutable bool changesDuringRead = false;
    std::function<void()> onBundleRead;

    auto revision() const -> qint64 override
    {
        ++revisionChecks;
        return currentRevision.load() +
               (changesDuringRead && reads > 0 ? 1 : 0);
    }
    auto bundles(const std::atomic_bool&) const -> QStringList override
    {
        ++searches;
        if (unavailable) {
            throw std::runtime_error("Store unavailable");
        }
        auto ids = installed.keys();
        ids.sort();
        return ids;
    }
    auto bundle(const QString& id) const -> Bundle override
    {
        ++reads;
        if (onBundleRead) {
            onBundleRead();
        }
        return installed.value(id);
    }
    auto resolve(const std::filesystem::path& path) const
      -> std::optional<Asset> override
    {
        const auto key = support::pathToQString(path);
        if (!assets.contains(key)) {
            return std::nullopt;
        }
        return assets.value(key);
    }
    auto packs() const -> QList<Pack> override { return installedPacks; }
    auto collections(db::SqliteCppDb*,
                     const QHash<QString, IndexedBundle>&) const
      -> QList<resource_managers::Table> override
    {
        return {};
    }
};
#endif

} // namespace

TEST_CASE("Chart and histogram replacement rolls back together",
          "[library][ChartData]")
{
    Library library;
    const auto path = library.temporary.filePath("chart.bms");
    const auto original = library.save(path);
    library.database.execute(
      "CREATE TRIGGER reject_histogram BEFORE INSERT ON histogram_data "
      "BEGIN SELECT RAISE(ABORT, 'injected histogram failure'); END");
    REQUIRE_THROWS_AS(
      library.save(path, chartBytes + "#SUBTITLE Replacement\n"),
      SQLite::Exception);
    CHECK(library.count() == 1);
    CHECK(library.database.createStatement("SELECT md5 FROM charts")
            .executeAndGet<std::string>() == original->getMd5().toStdString());
    CHECK(library.database
            .createStatement("SELECT count(*) FROM charts c "
                             "JOIN histogram_data h ON h.chart_id = c.id")
            .executeAndGet<int>() == 1);
}

TEST_CASE(
  "Collections preserve course order, repeated charts and SHA-256 references",
  "[library][collections]")
{
    Library library;
    const auto first = library.save(library.temporary.filePath("first.bms"));
    const auto second = library.save(library.temporary.filePath("second.bms"),
                                     chartBytes + "#SUBTITLE Second\n");
    resource_managers::Course course{ &library.database };
    course.md5s = { second->getMd5(), first->getMd5(), second->getMd5() };
    const auto charts = course.loadCharts();
    REQUIRE(charts.size() == 3);
    REQUIRE(charts[0].value<gameplay_logic::ChartData*>()->getPath() ==
            second->getPath());
    REQUIRE(charts[1].value<gameplay_logic::ChartData*>()->getPath() ==
            first->getPath());
    REQUIRE(charts[2].value<gameplay_logic::ChartData*>()->getPath() ==
            second->getPath());
    deleteCharts(charts);

    course.md5s = { QString{}, QString{} };
    course.sha256s = { second->getSha256(), QString(64, 'f') };
    const auto identifier = course.getIdentifier();
    REQUIRE(course.chartPath(0) == second->getPath());
    REQUIRE(course.chartPath(1).isEmpty());
    const auto withMissing = course.loadCharts();
    REQUIRE(withMissing[0].canConvert<gameplay_logic::ChartData*>());
    REQUIRE(withMissing[1].toString() == "sha256/" + QString(64, 'f'));
    deleteCharts(withMissing);
    course.md5s[0] = second->getMd5();
    REQUIRE(course.getIdentifier() == identifier);
}

TEST_CASE("Collection entries can require an exact path or fall back by hash",
          "[library][collections]")
{
    Library library;
    const auto native = library.save(library.temporary.filePath("native.bms"));
    resource_managers::Entry entry;
    entry.md5 = native->getMd5();
    entry.path = "backbeat:/Backbeat/missing/chart.bms";
    entry.exactPath = true;
    resource_managers::Level level{ &library.database, "Pack", { entry } };
    auto charts = level.loadCharts();
    REQUIRE(charts[0].canConvert<resource_managers::Entry>());
    level.entries[0].exactPath = false;
    charts = level.loadCharts();
    REQUIRE(charts[0].value<gameplay_logic::ChartData*>()->getPath() ==
            native->getPath());
    deleteCharts(charts);

    level.entries = {};
    entry.path = native->getPath();
    for (int i = 0; i < 270; ++i) {
        level.entries.append(entry);
    }
    charts = level.loadCharts();
    REQUIRE(charts.size() == 270);
    for (const auto& chart : charts) {
        REQUIRE(chart.value<gameplay_logic::ChartData*>()->getPath() ==
                native->getPath());
    }
    deleteCharts(charts);
}

TEST_CASE("Collection browsing and reload retain the owning provider",
          "[library][collections]")
{
    ensureCoreApplication();
    Library library;
    const auto url =
      QUrl::fromLocalFile(library.temporary.filePath("header.json"));
    QFile cache(library.temporary.filePath("tables.json"));
    REQUIRE(cache.open(QIODevice::WriteOnly));
    cache.write(
      QJsonDocument(
        QJsonArray{ QJsonObject{
          { "url", url.toString() },
          { "header",
            QJsonObject{ { "name", "Native" },
                         { "level_order", QJsonArray{ "7" } } } },
          { "data",
            QJsonArray{ QJsonObject{ { "level", "7" },
                                     { "title", "Original chart" },
                                     { "md5", QString(32, 'a') } } } } } })
        .toJson());
    cache.close();

    class Network : public QNetworkAccessManager
    {
      public:
        int requests = 0;
        auto createRequest(Operation operation,
                           const QNetworkRequest& request,
                           QIODevice* data) -> QNetworkReply* override
        {
            ++requests;
            return QNetworkAccessManager::createRequest(
              operation, request, data);
        }
    } network;
    resource_managers::Tables tables(
      &network, QDir(library.temporary.path()), &library.database);
    const auto native = tables.getList()[0].value<resource_managers::Table>();
    REQUIRE(native.levels.size() == 1);
    REQUIRE(native.levels[0].entries.size() == 1);
    auto external = native;
    external.name = "External";
    external.managedExternally = true;
    tables.setExternalTables({ external });
    REQUIRE(native.getIdentifier() != external.getIdentifier());
    REQUIRE(
      tables.resolveTable(native).value<resource_managers::Table>().name ==
      "Native");
    REQUIRE(
      tables.resolveTable(external).value<resource_managers::Table>().name ==
      "External");

    const auto retainedLevel = external.levels[0];
    auto updated = external;
    updated.name = "Updated";
    updated.levels[0].entries[0].title = "New chart";
    tables.setExternalTables({ updated });
    REQUIRE(
      tables.resolveTable(external).value<resource_managers::Table>().name ==
      "Updated");
    REQUIRE(tables.resolveLevel(external, retainedLevel)
              .value<resource_managers::Level>()
              .entries[0]
              .title == "New chart");
    REQUIRE(tables.resolveLevel(native, retainedLevel)
              .value<resource_managers::Level>()
              .entries[0]
              .title == "Original chart");

    int externalRequests = 0;
    QObject::connect(&tables,
                     &resource_managers::Tables::externalReloadRequested,
                     [&] { ++externalRequests; });
    REQUIRE(tables.reloadTable(external));
    REQUIRE(externalRequests == 1);
    REQUIRE(network.requests == 0);
    REQUIRE(tables.reloadTable(native));
    REQUIRE(externalRequests == 1);
    REQUIRE(network.requests == 1);

    updated.levels.clear();
    tables.setExternalTables({ updated });
    REQUIRE_FALSE(tables.resolveLevel(external, retainedLevel).isValid());
    tables.setExternalTables({});
    REQUIRE_FALSE(tables.resolveTable(external).isValid());
    REQUIRE_FALSE(tables.reloadTable(external));
    REQUIRE(tables.resolveTable(native).isValid());
}

#ifdef RHYTHMGAME_USE_BACKBEAT

TEST_CASE(
  "Native folder cleanup during Backbeat parsing cannot orphan its charts",
  "[backbeat]")
{
    Library library;
    auto source = std::make_shared<Store>();
    source->installed.insert(QString(64, 'a'), { "chart.bms", chartBytes, {} });
    source->onBundleRead = [&] {
        library.database.execute(
          "DELETE FROM parent_dir WHERE id NOT IN "
          "(SELECT directory FROM charts WHERE directory IS NOT NULL)");
    };
    resource_managers::BackbeatCatalog catalog(
      source, library.path, &library.database);
    REQUIRE(catalog.synchronize().added == 1);
    CHECK(library.database
            .createStatement("SELECT count(*) FROM charts c "
                             "JOIN parent_dir p ON p.id = c.directory")
            .executeAndGet<int>() == 1);
    qml_components::SongFolderFactory folders(&library.database);
    const auto charts =
      folders.open(resource_managers::BackbeatSource::rootPath());
    CHECK(charts.size() == 1);
    deleteCharts(charts);
}

TEST_CASE("Backbeat polls revisions without a QML engine or selection screen",
          "[backbeat]")
{
    ensureCoreApplication();
    Library library;
    auto source = std::make_shared<Store>();
    resource_managers::BackbeatCatalog catalog(
      source, library.path, &library.database);
    int updates = 0;
    int completed = 0;
    QObject::connect(&catalog,
                     &resource_managers::BackbeatCatalog::updated,
                     [&](const auto&) { ++updates; });
    QObject::connect(&catalog,
                     &resource_managers::BackbeatCatalog::busyChanged,
                     [&](bool busy) { completed += !busy; });
    REQUIRE(source->revisionChecks == 0);
    REQUIRE(waitUntil([&] { return updates == 1; }));
    catalog.refresh();
    REQUIRE(waitUntil([&] { return completed == 2; }));
    REQUIRE(updates == 1);
    REQUIRE(source->searches == 1);
    ++source->currentRevision;
    REQUIRE(waitUntil([&] { return updates == 2; }));
    REQUIRE(source->searches == 2);
}

TEST_CASE("Backbeat and the application use the same SQLite", "[backbeat]")
{
    REQUIRE(bkb_sqlite_version_number() == sqlite3_libversion_number());
    REQUIRE(sqlite3_libversion_number() >= BKB_SQLITE_MIN_VERSION_NUMBER);
    REQUIRE(sqlite3_threadsafe() != 0);
    REQUIRE(sqlite3_compileoption_used("ENABLE_FTS5"));
}

TEST_CASE(
  "Backbeat indexing is incremental and deletion preserves native charts",
  "[backbeat]")
{
    Library library;
    auto native = library.save(library.temporary.filePath("native.bms"));
    auto source = std::make_shared<Store>();
    const auto id = QString(64, 'a');
    source->installed.insert(
      id, { "chart.bms", chartBytes, { "preview.ogg", "readme.txt" } });
    resource_managers::BackbeatCatalog catalog(
      source, library.path, &library.database);
    const auto first = catalog.synchronize();
    REQUIRE(first.added == 1);
    REQUIRE(first.error.isEmpty());
    REQUIRE(source->reads == 1);
    REQUIRE(library.count() == 2);
    qml_components::SongFolderFactory folders(&library.database);
    auto charts = folders.open(resource_managers::BackbeatSource::rootPath());
    REQUIRE(charts.size() == 1);
    const auto* chart = charts[0].value<gameplay_logic::ChartData*>();
    REQUIRE(chart->getMd5() == native->getMd5());
    REQUIRE(chart->getChartDirectory() == "backbeat:/Backbeat/" + id + '/');
    REQUIRE(chart->getStageFileSource().startsWith("image://song-assets/"));
    deleteCharts(charts);
    REQUIRE_FALSE(catalog.synchronize(first.revision).revision);
    REQUIRE(source->searches == 1);
    ++source->currentRevision;
    REQUIRE(catalog.synchronize(first.revision).added == 0);
    REQUIRE(source->reads == 1);
    // Restarting still reuses the persistent parsed chart cache.
    REQUIRE(catalog.synchronize().added == 0);
    REQUIRE(source->reads == 1);

    source->installed.clear();
    ++source->currentRevision;
    REQUIRE(catalog.synchronize().removed == 1);
    REQUIRE(library.count() == 1);
    REQUIRE(library.database.createStatement("SELECT path FROM charts")
              .executeAndGet<std::string>() == native->getPath().toStdString());
    REQUIRE(
      library.database.createStatement("SELECT count(*) FROM preview_files")
        .executeAndGet<int>() == 0);
    REQUIRE(
      library.database.createStatement("SELECT count(*) FROM histogram_data")
        .executeAndGet<int>() == 1);
}

TEST_CASE("Backbeat refresh failures and concurrent imports do not discard "
          "cached charts",
          "[backbeat]")
{
    Library library;
    auto source = std::make_shared<Store>();
    source->installed.insert(QString(64, 'a'), { "one.bms", chartBytes, {} });
    resource_managers::BackbeatCatalog catalog(
      source, library.path, &library.database);
    REQUIRE(catalog.synchronize().added == 1);
    source->unavailable = true;
    REQUIRE_THROWS(catalog.synchronize());
    REQUIRE(library.count() == 1);
    source->unavailable = false;
    source->installed.clear();
    source->installed.insert(QString(64, 'b'), { "two.bms", chartBytes, {} });
    source->reads = 0;
    source->changesDuringRead = true;
    REQUIRE_FALSE(catalog.synchronize().revision);
    REQUIRE(library.count() == 1);
    source->changesDuringRead = false;
    const auto updated = catalog.synchronize();
    REQUIRE(updated.added == 1);
    REQUIRE(updated.removed == 1);
    REQUIRE(library.count() == 1);
}

TEST_CASE("Backbeat packs are ordinary folders with independent membership",
          "[backbeat][folders]")
{
    Library library;
    const auto native = library.save(library.temporary.filePath("native.bms"));
    auto source = std::make_shared<Store>();
    const auto firstId = QString(64, 'a');
    const auto secondId = QString(64, 'b');
    const auto looseId = QString(64, 'c');
    const auto missingId = QString(64, 'd');
    for (const auto& id : { firstId, secondId, looseId }) {
        source->installed.insert(id, { "chart.bms", chartBytes, {} });
    }
    const auto name = QString::fromUtf8("Pack / 100% #1 \\ 音楽");
    source->installedPacks = {
        { "https://example.test/one", name, { firstId, missingId, firstId } },
        { "https://example.test/two", name, { firstId, secondId } },
        { "https://example.test/empty", "Empty pack", { missingId } }
    };
    resource_managers::BackbeatCatalog catalog(
      source, library.path, &library.database);
    const auto first = catalog.synchronize();
    REQUIRE(first.added == 3);
    REQUIRE(first.collections.isEmpty());

    qml_components::SongFolderFactory folders(&library.database);
    const auto root = resource_managers::BackbeatSource::rootPath();
    const auto packFolders = [&] {
        QStringList paths;
        const auto items = folders.open("");
        for (const auto& item : items) {
            if (item.typeId() == QMetaType::QString &&
                item.toString() != root) {
                paths.append(item.toString());
            }
        }
        deleteCharts(items);
        return paths;
    };
    const auto paths = packFolders();
    REQUIRE(paths.size() == 2);
    CHECK(paths[0] != paths[1]);
    QString single;
    QString shared;
    for (const auto& path : paths) {
        CHECK(support::folderName(path) == name);
        CHECK(folders.parentFolder(path).isEmpty());
        const auto charts = folders.open(path);
        CHECK(folders.folderSize(path) == charts.size());
        REQUIRE_FALSE(charts.isEmpty());
        for (const auto& item : charts) {
            const auto* chart = item.value<gameplay_logic::ChartData*>();
            REQUIRE(chart);
            CHECK(chart->getPath() != native->getPath());
            CHECK(chart->getChartDirectory().startsWith(root));
        }
        if (charts.size() == 1) {
            single = path;
        } else {
            REQUIRE(charts.size() == 2);
            shared = path;
        }
        const auto recursive = folders.openRecursive(path.chopped(1));
        CHECK(recursive.size() == charts.size());
        deleteCharts(recursive);
        deleteCharts(charts);
    }
    REQUIRE_FALSE(single.isEmpty());
    REQUIRE_FALSE(shared.isEmpty());
    const auto all = folders.openRecursive("");
    CHECK(all.size() == 4);
    deleteCharts(all);
    const auto allBackbeat = folders.open(root);
    CHECK(allBackbeat.size() == 3);
    deleteCharts(allBackbeat);

    library.database.execute(
      "CREATE TRIGGER reject_pack BEFORE INSERT ON folder_charts "
      "BEGIN SELECT RAISE(ABORT, 'injected pack publication failure'); END");
    REQUIRE_THROWS(catalog.synchronize());
    CHECK(folders.folderSize(single) == 1);
    CHECK(folders.folderSize(shared) == 2);
    CHECK(library.count() == 4);
    library.database.execute("DROP TRIGGER reject_pack");

    source->installedPacks[0].name = "Renamed";
    source->installedPacks[1].bundles = { secondId };
    ++source->currentRevision;
    const auto changed = catalog.synchronize(first.revision);
    CHECK(changed.added == 0);
    CHECK(changed.removed == 0);
    CHECK(source->reads == 3);
    const auto removedFolder = folders.open(single);
    CHECK(removedFolder.isEmpty());
    CHECK(folders.folderSize(single) == 0);
    deleteCharts(removedFolder);
    CHECK(folders.folderSize(shared) == 1);

    source->installed.remove(secondId);
    ++source->currentRevision;
    CHECK(catalog.synchronize(changed.revision).removed == 1);
    const auto remaining = packFolders();
    REQUIRE(remaining.size() == 1);
    CHECK(support::folderName(remaining[0]) == "Renamed");
    CHECK(folders.folderSize(shared) == 0);

    source->installedPacks.clear();
    ++source->currentRevision;
    REQUIRE(catalog.synchronize().revision.has_value());
    CHECK(packFolders().isEmpty());
    CHECK(library.count() == 3);
    CHECK(library.database.createStatement("SELECT count(*) FROM folder_charts")
            .executeAndGet<int>() == 0);
}

TEST_CASE("Backbeat assets use memory or existing files and round-trip through "
          "image URLs",
          "[backbeat]")
{
    QTemporaryDir temporary;
    auto source = std::make_shared<Store>();
    const auto path = resource_managers::BackbeatSource::chartPath(
      QString(64, 'c'), "chart.bms");
    const auto image =
      path.parent_path() /
      support::qStringToPath(QStringLiteral("日本語 # image.png"));
    source->assets.insert(support::pathToQString(path), chartBytes);
    source->assets.insert(support::pathToQString(image),
                          QByteArray("inline image"));
    const auto local = temporary.filePath("music.ogg");
    QFile file(local);
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write("file sound");
    file.close();
    const auto sound = path.parent_path() / "sound.wav";
    source->assets.insert(
      support::pathToQString(path.parent_path() / "sound.ogg"),
      support::qStringToPath(local));
    resource_managers::SongAssetStore assets;
    assets.setBackbeatSource(source);
    REQUIRE(assets.isVirtual(path));
    REQUIRE_FALSE(assets.isArchived(path));
    REQUIRE(assets.read(path) == chartBytes);
    REQUIRE(assets.read(sound) == "file sound");
    REQUIRE(assets.materialize(sound) == support::qStringToPath(local));
    REQUIRE(resource_managers::SongAssetStore::pathFromUrl(
              assets.imageUrl(image)) == image);
    const auto sibling = path.parent_path() / "../common/banner.png";
    source->assets.insert(support::pathToQString(sibling),
                          QByteArray("shared image"));
    REQUIRE(assets.read(sibling) == "shared image");
    REQUIRE(assets.read(path.parent_path() / "..\\common\\banner.png") ==
            "shared image");
    REQUIRE(resource_managers::SongAssetStore::pathFromUrl(
              assets.imageUrl(support::pathToQString(path.parent_path()),
                              "../common/banner.png")) == sibling);
    const auto materialized = assets.materialize(image);
    REQUIRE(assets.read(materialized) == "inline image");
    REQUIRE(assets.materialize(image) == materialized);
    const auto batch = assets.materializeRelative(
      path.parent_path(), { "sound.wav", "missing.wav" });
    REQUIRE(batch.size() == 1);
    REQUIRE(batch.at("sound.wav") == support::qStringToPath(local));
    REQUIRE(assets.containingFolder(support::pathToQString(path)).isEmpty());
    REQUIRE_THROWS(assets.read(path.parent_path() / "missing"));
    std::atomic_bool cancelled = true;
    REQUIRE_THROWS(assets.read(sound, &cancelled));
}

TEST_CASE("Arena verifies Backbeat chart bytes without a filesystem chart",
          "[backbeat][arena]")
{
    ensureCoreApplication();
    Library library;
    auto source = std::make_shared<Store>();
    const auto path = resource_managers::BackbeatSource::chartPath(
      QString(64, 'd'), "chart.bms");
    const auto chart = library.save(support::pathToQString(path));
    source->assets.insert(support::pathToQString(path), chartBytes);
    resource_managers::SongAssetStore assets;
    assets.setBackbeatSource(source);
    arena::QtArenaRoundLoader loader(
      nullptr, &library.database, nullptr, &assets);
    std::optional<arena::ArenaProbeResult> result;
    QObject::connect(&loader,
                     &arena::ArenaRoundLoader::probeFinished,
                     [&result](quint64, const arena::ArenaProbeResult& value) {
                         result = value;
                     });
    const auto digest =
      QCryptographicHash::hash(chartBytes, QCryptographicHash::Sha256);
    loader.probe(1, digest);
    QElapsedTimer timer;
    timer.start();
    while (!result && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE(result.has_value());
    REQUIRE(result->failure == arena::ArenaProbeFailure::None);
    REQUIRE(result->observedSha256 == digest);
}

#else

TEST_CASE("A build without Backbeat removes only its cached catalog",
          "[library][backbeat-disabled]")
{
    Library library;
    const auto native = library.save(library.temporary.filePath("native.bms"));
    library.save("backbeat:/Backbeat/" + QString(64, 'a') + "/chart.bms");
    library.database.execute(
      "INSERT INTO parent_dir(dir) VALUES ('backbeat:/packs/old/Pack/')");
    library.database.execute("INSERT INTO folder_charts(directory, chart_id) "
                             "SELECT p.id, c.id FROM parent_dir p, charts c "
                             "WHERE p.dir = 'backbeat:/packs/old/Pack/' "
                             "AND c.path GLOB 'backbeat:/*'");
    REQUIRE(library.count() == 2);
    resource_managers::defineDb(library.database);
    REQUIRE(library.count() == 1);
    REQUIRE(library.database.createStatement("SELECT path FROM charts")
              .executeAndGet<std::string>() == native->getPath().toStdString());
    CHECK(library.database.createStatement("SELECT count(*) FROM folder_charts")
            .executeAndGet<int>() == 0);
    CHECK(library.database
            .createStatement("SELECT count(*) FROM parent_dir "
                             "WHERE dir GLOB 'backbeat:/*'")
            .executeAndGet<int>() == 0);
}

#endif
