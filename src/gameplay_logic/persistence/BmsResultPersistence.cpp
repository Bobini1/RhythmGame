#include "gameplay_logic/BmsResult.h"

#include "db/SqliteCppDb.h"
#include "support/Compress.h"

void
gameplay_logic::BmsResult::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement =
      db.createStatement("INSERT OR IGNORE INTO score ("
                         "max_points, "
                         "max_hits, "
                         "normal_note_count, "
                         "scratch_count, "
                         "ln_count, "
                         "bss_count, "
                         "mine_count, "
                         "clear_type, "
                         "points, "
                         "max_combo, "
                         "poor, "
                         "empty_poor, "
                         "bad, "
                         "good, "
                         "great, "
                         "perfect,"
                         "mine_hits,"
                         "guid,"
                         "sha256,"
                         "md5,"
                         "unix_timestamp,"
                         "length,"
                         "random_sequence,"
                         "random_seed,"
                         "note_order_algorithm,"
                         "note_order_algorithm_p2,"
                         "dp_options,"
                         "keymode,"
                         "game_version,"
                         "owner"
                         ")"
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                         "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    statement.bind(1, maxPoints);
    statement.bind(2, maxHits);
    statement.bind(3, normalNoteCount);
    statement.bind(4, scratchCount);
    statement.bind(5, lnCount);
    statement.bind(6, bssCount);
    statement.bind(7, mineCount);
    statement.bind(8, clearType.toStdString());
    statement.bind(9, points);
    statement.bind(10, maxCombo);
    statement.bind(11, judgementCounts[static_cast<int>(Judgement::Poor)]);
    statement.bind(12, judgementCounts[static_cast<int>(Judgement::EmptyPoor)]);
    statement.bind(13, judgementCounts[static_cast<int>(Judgement::Bad)]);
    statement.bind(14, judgementCounts[static_cast<int>(Judgement::Good)]);
    statement.bind(15, judgementCounts[static_cast<int>(Judgement::Great)]);
    statement.bind(16, judgementCounts[static_cast<int>(Judgement::Perfect)]);
    statement.bind(17, mineHits);
    statement.bind(18, guid.toStdString());
    statement.bind(19, sha256.toStdString());
    statement.bind(20, md5.toStdString());
    statement.bind(21, unixTimestamp);
    statement.bind(22, length);
    auto randomSequenceCompressed = support::compress(randomSequence);
    statement.bind(
      23, randomSequenceCompressed.data(), randomSequenceCompressed.size());
    statement.bind(24, static_cast<int64_t>(randomSeed));
    statement.bind(25, static_cast<int>(noteOrderAlgorithm));
    statement.bind(26, static_cast<int>(noteOrderAlgorithmP2));
    statement.bind(27, static_cast<int>(dpOptions));
    statement.bind(28, static_cast<int>(keymode));
    statement.bind(29, static_cast<int64_t>(gameVersion));
    statement.bind(30, owner.toStdString());
    statement.execute();
}
