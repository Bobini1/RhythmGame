#include "gameplay_logic/ChartData.h"

#include "db/SqliteCppDb.h"
#include "support/Compress.h"

auto
gameplay_logic::ChartData::save(db::SqliteCppDb& db) const -> void
{
    auto query = db.createStatement(
      "INSERT OR REPLACE INTO charts (title, artist, subtitle, subartist, "
      "genre, stage_file, banner, back_bmp, rank, total, play_level, "
      "difficulty, is_random, random_sequence, normal_note_count, "
      "scratch_count, ln_count, "
      "bss_count, mine_count, length, initial_bpm, max_bpm, "
      "min_bpm, main_bpm, avg_bpm, peak_density, avg_density, end_density, "
      "path, chart_directory, directory, sha256, "
      "md5, keymode, game_version) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
      "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    query.bind(1, title.toStdString());
    query.bind(2, artist.toStdString());
    query.bind(3, subtitle.toStdString());
    query.bind(4, subartist.toStdString());
    query.bind(5, genre.toStdString());
    query.bind(6, stageFile.toStdString());
    query.bind(7, banner.toStdString());
    query.bind(8, backBmp.toStdString());
    query.bind(9, rank);
    query.bind(10, total);
    query.bind(11, playLevel);
    query.bind(12, difficulty);
    query.bind(13, isRandom);
    auto compressed = support::compress(randomSequence);
    query.bind(14, compressed.data(), compressed.size());
    query.bind(15, normalNoteCount);
    query.bind(16, scratchCount);
    query.bind(17, lnCount);
    query.bind(18, bssCount);
    query.bind(19, mineCount);
    query.bind(20, length);
    query.bind(21, initialBpm);
    query.bind(22, maxBpm);
    query.bind(23, minBpm);
    query.bind(24, mainBpm);
    query.bind(25, avgBpm);
    query.bind(26, peakDensity);
    query.bind(27, avgDensity);
    query.bind(28, endDensity);
    query.bind(29, path.toStdString());
    query.bind(30, getChartDirectory().toStdString());
    if (directory == -1) {
        query.bind(31);
    } else {
        query.bind(31, directory);
    }
    query.bind(32, sha256.toStdString());
    query.bind(33, md5.toStdString());
    query.bind(34, static_cast<int>(keymode));
    query.bind(35, static_cast<int64_t>(gameVersion));
    auto id = query.execute();
    auto query2 =
      db.createStatement("INSERT OR REPLACE INTO histogram_data "
                         "(histogram_data, bpms, chart_id) VALUES (?, ?, ?);");

    auto compressedHistogram = support::compress(histogramData);
    query2.bind(1, compressedHistogram.data(), compressedHistogram.size());
    auto compressedBpmChanges = support::compress(bpmChanges);
    query2.bind(2, compressedBpmChanges.data(), compressedBpmChanges.size());
    query2.bind(3, id);
    query2.execute();
}
