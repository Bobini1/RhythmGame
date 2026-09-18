#include "db/SqliteCppDb.h"
#include "gameplay_logic/ChartData.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/SongFolderFactory.h"
#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongAssetStore.h"
#include "support/QStringToPath.h"
#include "support/Version.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

namespace {
struct Catalog
{
    QTemporaryDir directory;
    db::SqliteCppDb database{ support::qStringToPath(
      directory.filePath("songs.sqlite")) };

    Catalog()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char name[] = "Database.test";
            static char* argv[] = { name, nullptr };
            static QCoreApplication application(argc, argv);
        }
        resource_managers::defineDb(database);
    }

    void save(const QString& path, int directoryId)
    {
        const resource_managers::ChartDataFactory factory;
        auto components = factory.loadChartData(
          "#PLAYER 1\n#TITLE Test\n#ARTIST Test\n#BPM 120\n#00111:0100\n",
          support::qStringToPath(path),
          [](auto) { return 1; },
          directoryId);
        components.chartData->save(database);
    }
};

void
deleteCharts(const QVariantList& charts)
{
    for (const auto& chart : charts) {
        if (chart.canConvert<gameplay_logic::ChartData*>())
            delete chart.value<gameplay_logic::ChartData*>();
    }
}
}

TEST_CASE("Catalog migration rolls back schema data and version together",
          "[Database]")
{
    Catalog catalog;
    auto& db = catalog.database;
    catalog.save("C:/songs/chart.bms", -1);
    db.execute("UPDATE charts SET rank = 2");
    auto version = db.createStatement(
      "UPDATE properties SET value = ? WHERE key = 'version'");
    version.bind(1, static_cast<int64_t>(support::packVersion(1, 3, 5)));
    version.execute();
    db.execute("CREATE TRIGGER reject_version BEFORE INSERT ON properties "
               "WHEN new.key = 'version' BEGIN "
               "SELECT RAISE(ABORT, 'injected migration failure'); END");

    REQUIRE_THROWS(resource_managers::defineDb(db));
    CHECK(db.createStatement("SELECT rank FROM charts").executeAndGet<int>() ==
          2);
    CHECK(db.createStatement("SELECT count(*) FROM pragma_table_info('charts') "
                             "WHERE name = 'rank_old'")
            .executeAndGet<int>() == 0);
    CHECK(
      db.createStatement("SELECT value FROM properties WHERE key = 'version'")
        .executeAndGet<int64_t>() == support::packVersion(1, 3, 5));

    db.execute("DROP TRIGGER reject_version");
    REQUIRE_NOTHROW(resource_managers::defineDb(db));
    CHECK(db.createStatement("SELECT rank FROM charts").executeAndGet<int>() ==
          75);
    CHECK(
      db.createStatement("SELECT value FROM properties WHERE key = 'version'")
        .executeAndGet<int64_t>() == support::currentVersion);
}

TEST_CASE("Clearing a root preserves another scan's unpublished directories",
          "[Database]")
{
    Catalog catalog;
    auto& db = catalog.database;
    db.execute("INSERT INTO parent_dir(id, dir) VALUES "
               "(1, 'C:/songs_100%/'), (2, 'C:/songsX100more/'), "
               "(3, 'C:/scanning/')");
    for (const auto* table : { "preview_files", "readme_files" }) {
        db.execute(std::string("INSERT INTO ") + table +
                   "(path, directory) VALUES "
                   "('remove', 'C:/songs_100%/'), "
                   "('keep', 'C:/scanning/')");
    }
    catalog.save("C:/songs_100%/remove.bms", 1);
    catalog.save("C:/songsX100more/keep.bms", 2);
    resource_managers::SongAssetStore assets;
    qml_components::ScanningQueue queue{
        &db, resource_managers::SongDbScanner{ &db, &assets }
    };
    queue.clear("C:/songs_100%");

    CHECK(
      db.createStatement("SELECT count(*) FROM charts").executeAndGet<int>() ==
      1);
    CHECK(db.createStatement("SELECT count(*) FROM parent_dir")
            .executeAndGet<int>() == 2);
    for (const auto* table : { "preview_files", "readme_files" }) {
        CHECK(db.createStatement(std::string("SELECT path FROM ") + table)
                .executeAndGet<std::string>() == "keep");
    }
    catalog.save("C:/scanning/late.bms", 3);
    CHECK(db.createStatement("SELECT count(*) FROM charts c "
                             "JOIN parent_dir p ON c.directory = p.id")
            .executeAndGet<int>() == 2);
}

TEST_CASE(
  "Folder counts and recursive queries include every matching directory",
  "[Database]")
{
    Catalog catalog;
    auto& db = catalog.database;
    db.execute("INSERT INTO parent_dir(id, parent_dir, dir) VALUES "
               "(1, NULL, 'C:/songs_100%/'), "
               "(2, 'C:/songs_100%/', 'C:/songs_100%/child/'), "
               "(3, NULL, 'C:/songsX100more/')");
    catalog.save("C:/songs_100%/one.bms", 1);
    catalog.save("C:/songs_100%/child/two.bms", 2);
    catalog.save("C:/songsX100more/three.bms", 3);
    qml_components::SongFolderFactory folders(&db);

    CHECK(folders.folderSize("C:/songs_100%/") == 2);
    CHECK(folders.folderSize("C:/empty/") == 0);
    CHECK(folders.folderSize("") == 2);
    const auto descendants = folders.openRecursive("C:/songs_100%");
    CHECK(descendants.size() == 2);
    deleteCharts(descendants);
    const auto all = folders.openRecursive("");
    CHECK(all.size() == 3);
    deleteCharts(all);
}
