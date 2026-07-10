#include "arena/SqliteArenaInventorySource.h"
#include "arena/ArenaBinaryProtocol.h"
#include "db/SqliteCppDb.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SongDbScanner.h"
#include "support/QStringToPath.h"
#include "FakeArenaInventorySource.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <QTemporaryDir>
#include <QThread>

#include <functional>
#include <memory>
#include <optional>

namespace {

void
ensureApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "RhythmGameArenaInventoryTest";
    static char* argv[] = { applicationName, nullptr };
    static auto application = std::make_unique<QCoreApplication>(argc, argv);
}

auto
spinUntil(const std::function<bool()>& predicate, int timeoutMs = 20'000)
  -> bool
{
    ensureApplication();
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return predicate();
}

auto
databasePath(QTemporaryDir& directory) -> std::filesystem::path
{
    return support::qStringToPath(
      directory.filePath(QStringLiteral("songs.sqlite")));
}

void
createInventoryTable(db::SqliteCppDb& db)
{
    db.execute("CREATE TABLE charts (sha256 TEXT NOT NULL)");
}

auto
hexRecord(QString value) -> QByteArray
{
    return QByteArray::fromHex(value.toLatin1());
}

auto
rootPath(QTemporaryDir& directory, const QString& name) -> QString
{
    const auto path = directory.filePath(name);
    REQUIRE(QDir{}.mkpath(path));
    return QDir(path).canonicalPath() + QStringLiteral("/");
}

} // namespace

TEST_CASE("SqliteArenaInventorySource publishes sorted distinct raw hashes",
          "[arena][ArenaInventorySource]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    createInventoryTable(db);
    const auto zero = QString(64, QLatin1Char('0'));
    const auto upperF = QString(64, QLatin1Char('F'));
    auto insert = db.createStatement("INSERT INTO charts(sha256) VALUES (?)");
    for (const auto& value : { upperF, zero, upperF }) {
        insert.reset();
        insert.bind(1, value.toStdString());
        insert.execute();
    }

    arena::SqliteArenaInventorySource source(path);
    std::optional<arena::ArenaInventorySnapshot> snapshot;
    quint64 responseId = 0;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotReady,
      [&](quint64 requestId, arena::ArenaInventorySnapshot value) {
          responseId = requestId;
          snapshot = std::move(value);
      });
    source.requestSnapshot(41);

    REQUIRE(spinUntil([&] { return snapshot.has_value(); }));
    CHECK(responseId == 41);
    CHECK(snapshot->libraryGeneration == 1);
    CHECK(snapshot->packedSha256 ==
          hexRecord(zero) + hexRecord(upperF.toLower()));
}

TEST_CASE("SqliteArenaInventorySource rejects invalid hashes",
          "[arena][ArenaInventorySource]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    createInventoryTable(db);
    db.execute(
      "INSERT INTO charts(sha256) VALUES "
      "('gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg')");

    arena::SqliteArenaInventorySource source(path);
    std::optional<arena::ArenaInventoryFailure> failure;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotFailed,
      [&](quint64 requestId, arena::ArenaInventoryFailure value) {
          CHECK(requestId == 7);
          failure = value;
      });
    source.requestSnapshot(7);

    REQUIRE(spinUntil([&] { return failure.has_value(); }));
    CHECK(*failure == arena::ArenaInventoryFailure::InvalidHash);
}

TEST_CASE("SqliteArenaInventorySource enforces the exact inventory ceiling",
          "[arena][ArenaInventorySource]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    createInventoryTable(db);
    db.execute(
      "WITH RECURSIVE sequence(value) AS ("
      " SELECT 1 UNION ALL SELECT value + 1 FROM sequence WHERE value < 250000"
      ") INSERT INTO charts(sha256) SELECT printf('%064x', value) FROM "
      "sequence");

    arena::SqliteArenaInventorySource source(path);
    std::optional<arena::ArenaInventorySnapshot> snapshot;
    std::optional<arena::ArenaInventoryFailure> failure;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotReady,
      [&](quint64 requestId, arena::ArenaInventorySnapshot value) {
          if (requestId == 1) {
              snapshot = std::move(value);
          }
      });
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotFailed,
      [&](quint64 requestId, arena::ArenaInventoryFailure value) {
          if (requestId == 2) {
              failure = value;
          }
      });

    source.requestSnapshot(1);
    REQUIRE(spinUntil([&] { return snapshot.has_value(); }, 60'000));
    CHECK(snapshot->packedSha256.size() == arena::ArenaMaxInventoryBytes);

    db.execute("INSERT INTO charts(sha256) VALUES (printf('%064x', 250001))");
    source.commitLibraryMutation();
    source.requestSnapshot(2);
    REQUIRE(spinUntil([&] { return failure.has_value(); }, 60'000));
    CHECK(*failure == arena::ArenaInventoryFailure::TooManyCharts);
}

TEST_CASE(
  "SqliteArenaInventorySource restarts races for only the newest generation",
  "[arena][ArenaInventorySource]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    createInventoryTable(db);
    db.execute("INSERT INTO charts(sha256) VALUES (printf('%064x', 1))");

    arena::SqliteArenaInventorySource source(path);
    QVector<qint64> generations;
    QVector<quint64> responseIds;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::generationChanged,
      [&](qint64 generation) { generations.push_back(generation); });
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotReady,
      [&](quint64 requestId, const arena::ArenaInventorySnapshot& snapshot) {
          responseIds.push_back(requestId);
          generations.push_back(snapshot.libraryGeneration);
      });

    source.requestSnapshot(10);
    source.commitLibraryMutation();
    source.commitLibraryMutation();
    source.requestSnapshot(11);

    REQUIRE(spinUntil([&] { return !responseIds.isEmpty(); }));
    CHECK(source.generation() == 3);
    CHECK(responseIds == QVector<quint64>{ 11 });
    CHECK(generations == QVector<qint64>{ 2, 3, 3 });
}

TEST_CASE("SqliteArenaInventorySource cancellation suppresses stale completion",
          "[arena][ArenaInventorySource]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    createInventoryTable(db);
    db.execute("INSERT INTO charts(sha256) VALUES (printf('%064x', 1))");

    arena::SqliteArenaInventorySource source(path);
    QVector<quint64> responseIds;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotReady,
      [&](quint64 requestId, const arena::ArenaInventorySnapshot&) {
          responseIds.push_back(requestId);
      });
    source.requestSnapshot(1);
    source.cancel(1);
    source.commitLibraryMutation();
    CHECK_FALSE(spinUntil([&] { return !responseIds.isEmpty(); }, 500));
    source.requestSnapshot(2);

    REQUIRE(spinUntil([&] { return responseIds.contains(2); }));
    CHECK(responseIds == QVector<quint64>{ 2 });
}

TEST_CASE("ArenaInventorySource commits one generation after scan queue drain",
          "[arena][ArenaInventorySource][SongDbScanner]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    resource_managers::defineDb(db);
    resource_managers::SongDbScanner scanner(&db);
    qml_components::ScanningQueue queue(&db, scanner);
    arena::SqliteArenaInventorySource source(path);
    QObject::connect(&queue,
                     &qml_components::ScanningQueue::queueDrained,
                     &source,
                     &arena::SqliteArenaInventorySource::commitLibraryMutation);
    int drains = 0;
    QObject::connect(
      &queue, &qml_components::ScanningQueue::queueDrained, [&] { ++drains; });

    auto first = QSharedPointer<qml_components::RootSongFolder>::create(
      rootPath(directory, QStringLiteral("first")),
      qml_components::RootSongFolder::NotScanned);
    auto second = QSharedPointer<qml_components::RootSongFolder>::create(
      rootPath(directory, QStringLiteral("second")),
      qml_components::RootSongFolder::NotScanned);
    REQUIRE(queue.scan(first.get()));
    REQUIRE(queue.scan(second.get()));

    REQUIRE(spinUntil([&] { return queue.rowCount() == 0; }, 60'000));
    CHECK(drains == 1);
    CHECK(source.generation() == 2);
}

TEST_CASE("ArenaInventorySource scan stop reaches idle and commits once",
          "[arena][ArenaInventorySource][SongDbScanner]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    db::SqliteCppDb db(path);
    resource_managers::defineDb(db);
    resource_managers::SongDbScanner scanner(&db);
    qml_components::ScanningQueue queue(&db, scanner);
    arena::SqliteArenaInventorySource source(path);
    int drains = 0;
    QObject::connect(&queue,
                     &qml_components::ScanningQueue::queueDrained,
                     &source,
                     &arena::SqliteArenaInventorySource::commitLibraryMutation);
    QObject::connect(
      &queue, &qml_components::ScanningQueue::queueDrained, [&] { ++drains; });

    auto root = QSharedPointer<qml_components::RootSongFolder>::create(
      rootPath(directory, QStringLiteral("cancelled")),
      qml_components::RootSongFolder::NotScanned);
    REQUIRE(queue.scan(root.get()));
    queue.remove(0);

    REQUIRE(spinUntil([&] { return queue.rowCount() == 0; }, 60'000));
    CHECK(drains == 1);
    CHECK(source.generation() == 2);
}

TEST_CASE("ArenaInventorySource root removal commits after database cleanup",
          "[arena][ArenaInventorySource][SongDbScanner]")
{
    ensureApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = databasePath(directory);
    const auto root = rootPath(directory, QStringLiteral("removed"));
    db::SqliteCppDb db(path);
    db.execute("CREATE TABLE root_dir (path TEXT NOT NULL UNIQUE, status "
               "INTEGER NOT NULL)");
    db.execute(
      "CREATE TABLE charts (id INTEGER PRIMARY KEY, path TEXT NOT NULL, "
      "directory INTEGER, chart_directory TEXT, sha256 TEXT NOT NULL)");
    db.execute("CREATE TABLE parent_dir (id INTEGER PRIMARY KEY, parent_dir "
               "INTEGER, dir TEXT NOT NULL)");
    db.execute("CREATE TABLE note_data (sha256 TEXT NOT NULL)");
    db.execute("CREATE TABLE histogram_data (chart_id INTEGER)");
    db.execute("CREATE TABLE preview_files (directory TEXT)");
    db.execute("CREATE TABLE readme_files (directory TEXT)");
    auto insertRoot =
      db.createStatement("INSERT INTO root_dir(path, status) VALUES (?, 2)");
    insertRoot.bind(1, root.toStdString());
    insertRoot.execute();
    auto insertChart = db.createStatement(
      "INSERT INTO charts(id, path, sha256) VALUES (1, ?, printf('%064x', 1))");
    insertChart.bind(1, (root + QStringLiteral("chart.bms")).toStdString());
    insertChart.execute();

    resource_managers::SongDbScanner scanner(&db);
    qml_components::ScanningQueue queue(&db, scanner);
    qml_components::RootSongFolders folders(&db, &queue);
    arena::SqliteArenaInventorySource source(path);
    bool cleanedBeforeGenerationSignal = false;
    QObject::connect(
      &folders,
      &qml_components::RootSongFolders::chartSetMutationCommitted,
      &source,
      &arena::SqliteArenaInventorySource::commitLibraryMutation);
    QObject::connect(
      &source, &arena::ArenaInventorySource::generationChanged, [&] {
          auto count = db.createStatement("SELECT count(*) FROM charts")
                         .executeAndGet<int>();
          cleanedBeforeGenerationSignal = count && *count == 0;
      });

    folders.remove(0);
    CHECK(cleanedBeforeGenerationSignal);
    CHECK(source.generation() == 2);
}

TEST_CASE("FakeArenaInventorySource preserves request correlation",
          "[arena][ArenaInventorySource]")
{
    arena::test::FakeArenaInventorySource source;
    std::optional<arena::ArenaInventorySnapshot> snapshot;
    QObject::connect(
      &source,
      &arena::ArenaInventorySource::snapshotReady,
      [&](quint64 requestId, arena::ArenaInventorySnapshot value) {
          CHECK(requestId == 9);
          snapshot = std::move(value);
      });
    source.requestSnapshot(9);
    source.succeed(9, QByteArray(32, '\x01'));
    REQUIRE(snapshot);
    CHECK(source.requests == QVector<quint64>{ 9 });
    CHECK(snapshot->libraryGeneration == 1);
}
