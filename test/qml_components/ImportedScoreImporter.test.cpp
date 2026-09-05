#include "qml_components/ImportedScoreImporter.h"
#include "support/QStringToPath.h"

#include <QTemporaryDir>
#include <SQLiteCpp/SQLiteCpp.h>
#include <catch2/catch_test_macros.hpp>

namespace {

void
createTargetSchema(db::SqliteCppDb& database, const QString& songDatabasePath)
{
    auto escapedPath = songDatabasePath;
    escapedPath.replace(QStringLiteral("'"), QStringLiteral("''"));
    database.execute(QStringLiteral("ATTACH DATABASE '%1' AS song_db;")
                       .arg(escapedPath)
                       .toStdString());
    database.execute(
      "CREATE TABLE song_db.charts (sha256 TEXT, md5 TEXT, "
      "normal_note_count INTEGER, scratch_count INTEGER, ln_count INTEGER, "
      "bss_count INTEGER, mine_count INTEGER, length INTEGER, "
      "keymode INTEGER);");
    database.execute(
      "CREATE TABLE score (id INTEGER PRIMARY KEY, guid TEXT NOT NULL UNIQUE, "
      "sha256 TEXT NOT NULL, md5 TEXT NOT NULL, points INTEGER NOT NULL, "
      "max_points INTEGER NOT NULL, max_hits INTEGER NOT NULL, "
      "normal_note_count INTEGER NOT NULL, scratch_count INTEGER NOT NULL, "
      "ln_count INTEGER NOT NULL, bss_count INTEGER NOT NULL, "
      "mine_count INTEGER NOT NULL, max_combo INTEGER NOT NULL, "
      "poor INTEGER NOT NULL, empty_poor INTEGER NOT NULL, bad INTEGER NOT "
      "NULL, good INTEGER NOT NULL, great INTEGER NOT NULL, perfect INTEGER "
      "NOT NULL, mine_hits INTEGER NOT NULL, clear_type TEXT NOT NULL, "
      "keymode INTEGER NOT NULL, unix_timestamp INTEGER NOT NULL, "
      "length INTEGER NOT NULL, random_sequence STRING NOT NULL, "
      "random_seed INTEGER NOT NULL, note_order_algorithm INTEGER NOT NULL, "
      "note_order_algorithm_p2 INTEGER NOT NULL, dp_options INTEGER NOT NULL, "
      "game_version INTEGER NOT NULL, owner TEXT NOT NULL DEFAULT '', "
      "source INTEGER NOT NULL DEFAULT 0, "
      "ln_mode INTEGER NOT NULL DEFAULT 0);");
}

struct ImportedRow
{
    std::string clearType;
    int source{};
    int perfect{};
    int great{};
    int poor{};
    int emptyPoor{};
    int longNoteMode{};
};

} // namespace

TEST_CASE("LR2 score databases import with LR2 provenance",
          "[ImportedScoreImporter]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto targetPath = directory.filePath(QStringLiteral("target.db"));
    const auto songPath = directory.filePath(QStringLiteral("songs.db"));
    const auto sourcePath = directory.filePath(QStringLiteral("score.db"));
    auto target = db::SqliteCppDb{ support::qStringToPath(targetPath) };
    createTargetSchema(target, songPath);
    target.execute("INSERT INTO song_db.charts VALUES "
                   "('SHA', 'MD5', 100, 0, 0, 0, 0, 1000000, 7);");

    {
        auto source =
          SQLite::Database(sourcePath.toStdString(),
                           SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        source.exec(
          "CREATE TABLE score (hash TEXT, clear INTEGER, perfect INTEGER, "
          "great INTEGER, good INTEGER, bad INTEGER, poor INTEGER, "
          "totalnotes INTEGER, maxcombo INTEGER);");
        source.exec("INSERT INTO score VALUES "
                    "('MD5', 4, 40, 20, 10, 5, 2, 100, 80);");
    }

    auto imported = 0;
    qml_components::importLocalScoreDatabase(
      target,
      sourcePath,
      qml_components::ScoreImportCallbacks{
        .started = [](int total) { REQUIRE(total == 1); },
        .imported = [&imported] { ++imported; },
        .skipped = [] {},
        .failed = [](const QString&) { FAIL("Import unexpectedly failed"); },
      });

    REQUIRE(imported == 1);
    const auto row =
      target
        .createStatement("SELECT clear_type, source, perfect, great, poor, "
                         "empty_poor, ln_mode FROM score;")
        .executeAndGet<ImportedRow>();
    REQUIRE(row);
    CHECK(row->clearType == "HARD");
    CHECK(row->source == 1);
    CHECK(row->perfect == 40);
    CHECK(row->great == 20);
    CHECK(row->poor == 2);
    CHECK(row->longNoteMode == 0);
}

TEST_CASE("beatoraja light assist imports as a failed score",
          "[ImportedScoreImporter]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto targetPath = directory.filePath(QStringLiteral("target.db"));
    const auto songPath = directory.filePath(QStringLiteral("songs.db"));
    const auto sourcePath = directory.filePath(QStringLiteral("score.db"));
    auto target = db::SqliteCppDb{ support::qStringToPath(targetPath) };
    createTargetSchema(target, songPath);
    target.execute("INSERT INTO song_db.charts VALUES "
                   "('SHA', 'MD5', 100, 1, 2, 3, 0, 1000000, 7);");

    {
        auto source =
          SQLite::Database(sourcePath.toStdString(),
                           SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        source.exec(
          "CREATE TABLE score (sha256 TEXT, mode INTEGER, clear INTEGER, "
          "epg INTEGER, lpg INTEGER, egr INTEGER, lgr INTEGER, egd INTEGER, "
          "lgd INTEGER, ebd INTEGER, lbd INTEGER, epr INTEGER, lpr INTEGER, "
          "ems INTEGER, lms INTEGER, notes INTEGER, combo INTEGER, date "
          "INTEGER);");
        source.exec("INSERT INTO score VALUES "
                    "('SHA', 0, 3, 40, 10, 20, 5, 2, 1, 1, 1, 3, 2, 4, 1, 106, "
                    "75, 1234);");
    }

    auto total = 0;
    auto imported = 0;
    qml_components::importLocalScoreDatabase(
      target,
      sourcePath,
      qml_components::ScoreImportCallbacks{
        .started = [&total](int value) { total = value; },
        .imported = [&imported] { ++imported; },
        .skipped = [] {},
        .failed = [](const QString&) { FAIL("Import unexpectedly failed"); },
      });

    REQUIRE(total == 1);
    REQUIRE(imported == 1);
    const auto row =
      target
        .createStatement("SELECT clear_type, source, perfect, great, poor, "
                         "empty_poor, ln_mode FROM score;")
        .executeAndGet<ImportedRow>();
    REQUIRE(row);
    CHECK(row->clearType == "FAILED");
    CHECK(row->source == 2);
    CHECK(row->perfect == 50);
    CHECK(row->great == 25);
    CHECK(row->poor == 5);
    CHECK(row->emptyPoor == 5);
    CHECK(row->longNoteMode == 0);
}

TEST_CASE("beatoraja keeps separate best scores for LN, CN, and HCN",
          "[ImportedScoreImporter]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto targetPath = directory.filePath(QStringLiteral("target.db"));
    const auto songPath = directory.filePath(QStringLiteral("songs.db"));
    const auto sourcePath = directory.filePath(QStringLiteral("score.db"));
    auto target = db::SqliteCppDb{ support::qStringToPath(targetPath) };
    createTargetSchema(target, songPath);
    target.execute("INSERT INTO song_db.charts VALUES "
                   "('SHA', 'MD5', 100, 0, 10, 0, 0, 1000000, 7);");

    {
        auto source =
          SQLite::Database(sourcePath.toStdString(),
                           SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        source.exec(
          "CREATE TABLE score (sha256 TEXT, mode INTEGER, clear INTEGER, "
          "epg INTEGER, lpg INTEGER, egr INTEGER, lgr INTEGER, egd INTEGER, "
          "lgd INTEGER, ebd INTEGER, lbd INTEGER, epr INTEGER, lpr INTEGER, "
          "ems INTEGER, lms INTEGER, notes INTEGER, combo INTEGER, date "
          "INTEGER);");
        source.exec("INSERT INTO score VALUES "
                    "('SHA', 0, 5, 50, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, "
                    "110, 100, 1000), "
                    "('SHA', 1, 5, 51, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, "
                    "110, 101, 1001), "
                    "('SHA', 2, 5, 52, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, "
                    "110, 102, 1002);");
    }

    auto total = 0;
    auto imported = 0;
    const auto callbacks = qml_components::ScoreImportCallbacks{
        .started = [&total](int value) { total = value; },
        .imported = [&imported] { ++imported; },
        .skipped = [] {},
        .failed = [](const QString&) { FAIL("Import unexpectedly failed"); },
    };
    qml_components::importLocalScoreDatabase(target, sourcePath, callbacks);
    qml_components::importLocalScoreDatabase(target, sourcePath, callbacks);

    CHECK(total == 3);
    CHECK(imported == 6);
    struct LongNoteModeRow
    {
        int longNoteMode{};
    };
    const auto modes =
      target.createStatement("SELECT ln_mode FROM score ORDER BY ln_mode;")
        .executeAndGetAll<LongNoteModeRow>();
    REQUIRE(modes.size() == 3);
    CHECK(modes[0].longNoteMode == 0);
    CHECK(modes[1].longNoteMode == 1);
    CHECK(modes[2].longNoteMode == 2);
}

TEST_CASE("Bokutachi PBs import with Bokutachi provenance",
          "[ImportedScoreImporter]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto targetPath = directory.filePath(QStringLiteral("target.db"));
    const auto songPath = directory.filePath(QStringLiteral("songs.db"));
    auto target = db::SqliteCppDb{ support::qStringToPath(targetPath) };
    createTargetSchema(target, songPath);
    target.execute("INSERT INTO song_db.charts VALUES "
                   "('SHA', 'MD5', 100, 0, 0, 0, 0, 1000000, 7);");

    const auto response = QByteArrayLiteral(R"json({
      "body": {
        "charts": [{"chartID":"chart", "data": {
          "hashSHA256":"SHA", "hashMD5":"MD5"}}],
        "pbs": [{
          "game":"bms-7k", "chartID":"chart", "timeAchieved":1234000,
          "scoreData": {
            "enumIndexes":{"lamp":7}, "score":200,
            "judgements":{"pgreat":100,"great":0,"good":0,"bad":0,"poor":0},
            "optional":{"maxCombo":100}
          }
        }]
      }
    })json");

    REQUIRE(qml_components::importBokutachiPersonalBests(target, response) ==
            1);
    const auto row =
      target
        .createStatement("SELECT clear_type, source, perfect, great, poor, "
                         "empty_poor, ln_mode FROM score;")
        .executeAndGet<ImportedRow>();
    REQUIRE(row);
    CHECK(row->clearType == "MAX");
    CHECK(row->source == 3);
    CHECK(row->perfect == 100);
    CHECK(row->longNoteMode == 0);
}
