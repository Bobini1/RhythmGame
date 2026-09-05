#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QTemporaryDir>

#include <magic_enum/magic_enum.hpp>

namespace {
auto
makeResult(const QString& guid) -> std::unique_ptr<gameplay_logic::BmsResult>
{
    return std::make_unique<gameplay_logic::BmsResult>(
      200.0,
      100,
      100,
      0,
      0,
      0,
      0,
      QStringLiteral("HARD"),
      QList<int>(magic_enum::enum_count<gameplay_logic::Judgement>()),
      0,
      150.0,
      80,
      1234,
      10'000,
      QList<qint64>{},
      0,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      gameplay_logic::ChartData::Keymode::K7,
      guid,
      QStringLiteral("SHA256"),
      QStringLiteral("MD5"));
}

void
createScoreTables(db::SqliteCppDb& db)
{
    db.execute(
      "CREATE TABLE score ("
      "id INTEGER PRIMARY KEY, guid TEXT NOT NULL UNIQUE, "
      "sha256 TEXT NOT NULL, md5 TEXT NOT NULL, points INTEGER NOT NULL, "
      "max_points INTEGER NOT NULL, max_hits INTEGER NOT NULL, "
      "normal_note_count INTEGER NOT NULL, scratch_count INTEGER NOT NULL, "
      "ln_count INTEGER NOT NULL, bss_count INTEGER NOT NULL, "
      "mine_count INTEGER NOT NULL, max_combo INTEGER NOT NULL, "
      "poor INTEGER NOT NULL, empty_poor INTEGER NOT NULL, "
      "bad INTEGER NOT NULL, good INTEGER NOT NULL, great INTEGER NOT NULL, "
      "perfect INTEGER NOT NULL, mine_hits INTEGER NOT NULL, "
      "clear_type TEXT NOT NULL, keymode INTEGER NOT NULL, "
      "unix_timestamp INTEGER NOT NULL, length INTEGER NOT NULL, "
      "random_sequence STRING NOT NULL, random_seed INTEGER NOT NULL, "
      "note_order_algorithm INTEGER NOT NULL, "
      "note_order_algorithm_p2 INTEGER NOT NULL, dp_options INTEGER NOT NULL, "
      "game_version INTEGER NOT NULL, owner TEXT NOT NULL DEFAULT '', "
      "source INTEGER NOT NULL DEFAULT 0, "
      "ln_mode INTEGER NOT NULL DEFAULT 0);");
    db.execute(
      "CREATE TABLE replay_data (id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "score_guid TEXT NOT NULL UNIQUE, replay_data BLOB NOT NULL);");
    db.execute(
      "CREATE TABLE gauge_history (id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "score_guid TEXT NOT NULL UNIQUE, gauge_info BLOB NOT NULL);");
}

struct StoredProvenance
{
    int source{};
    int longNoteMode{};
};
}

TEST_CASE("Imported scores persist without replay-owned attachments",
          "[BmsScore]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto db = db::SqliteCppDb{ support::qStringToPath(
      directory.filePath(QStringLiteral("score.db"))) };
    createScoreTables(db);

    const auto guid = QStringLiteral("imported-guid");
    auto score = gameplay_logic::BmsScore::fromImportedResult(
      makeResult(guid),
      gameplay_logic::BmsScore::Source::Lr2,
      gameplay_logic::BmsScore::LongNoteMode::Hcn);

    REQUIRE(score->isImported());
    REQUIRE(score->getReplayData() == nullptr);
    REQUIRE(score->getGaugeHistory() == nullptr);
    REQUIRE(score->getLongNoteMode() ==
            gameplay_logic::BmsScore::LongNoteMode::Hcn);

    score->save(db);

    auto imported =
      db.createStatement("SELECT source, ln_mode FROM score WHERE guid = "
                         "'imported-guid';")
        .executeAndGet<StoredProvenance>();
    REQUIRE(imported);
    REQUIRE(imported->source ==
            static_cast<int>(gameplay_logic::BmsScore::Source::Lr2));
    REQUIRE(imported->longNoteMode ==
            static_cast<int>(gameplay_logic::BmsScore::LongNoteMode::Hcn));
    REQUIRE(db.createStatement("SELECT COUNT(*) FROM replay_data;")
              .executeAndGet<int>() == 0);
    REQUIRE(db.createStatement("SELECT COUNT(*) FROM gauge_history;")
              .executeAndGet<int>() == 0);
}

TEST_CASE("Complete score data upgrades an imported score with the same GUID",
          "[BmsScore]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto db = db::SqliteCppDb{ support::qStringToPath(
      directory.filePath(QStringLiteral("score.db"))) };
    createScoreTables(db);

    const auto guid = QStringLiteral("upgrade-guid");
    gameplay_logic::BmsScore::fromImportedResult(
      makeResult(guid),
      gameplay_logic::BmsScore::Source::Lr2,
      gameplay_logic::BmsScore::LongNoteMode::Hcn)
      ->save(db);

    auto complete = gameplay_logic::BmsScore(
      makeResult(guid),
      std::make_unique<gameplay_logic::BmsReplayData>(
        QList<gameplay_logic::HitEvent>{}, guid),
      std::make_unique<gameplay_logic::BmsGaugeHistory>(
        QList<gameplay_logic::BmsGaugeInfo>{}, guid));
    complete.save(db);

    const auto upgraded =
      db.createStatement("SELECT source, ln_mode FROM score WHERE guid = "
                         "'upgrade-guid';")
        .executeAndGet<StoredProvenance>();
    REQUIRE(upgraded);
    CHECK(upgraded->source == 0);
    CHECK(upgraded->longNoteMode == 0);
    REQUIRE(db.createStatement("SELECT COUNT(*) FROM replay_data;")
              .executeAndGet<int>() == 1);
    REQUIRE(db.createStatement("SELECT COUNT(*) FROM gauge_history;")
              .executeAndGet<int>() == 1);
}
