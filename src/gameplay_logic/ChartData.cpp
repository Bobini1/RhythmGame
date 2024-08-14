//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"
#include "support/LehmerCode.h"
#include "support/Compress.h"
#include <QFileInfo>

#include <memory>
#include <utility>
#include <spdlog/spdlog.h>
#include <zstd.h>

gameplay_logic::ChartData::
ChartData(QString title,
          QString artist,
          QString subtitle,
          QString subartist,
          QString genre,
          QString stageFile,
          QString banner,
          QString backBmp,
          int rank,
          double total,
          int playLevel,
          int difficulty,
          bool isRandom,
          QList<int64_t> randomSequence,
          int normalNoteCount,
          int lnCount,
          int mineCount,
          int64_t length,
          double bpm,
          double maxBpm,
          double minBpm,
          QString path,
          int64_t directory,
          QString sha256,
          Keymode keymode,
          QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , subtitle(std::move(subtitle))
  , subartist(std::move(subartist))
  , genre(std::move(genre))
  , stageFile(std::move(stageFile))
  , banner(std::move(banner))
  , backBmp(std::move(backBmp))
  , rank(rank)
  , total(total)
  , playLevel(playLevel)
  , difficulty(difficulty)
  , isRandom(isRandom)
  , randomSequence(std::move(randomSequence))
  , normalNoteCount(normalNoteCount)
  , lnCount(lnCount)
  , mineCount(mineCount)
  , length(length)
  , initialBpm(bpm)
  , maxBpm(maxBpm)
  , minBpm(minBpm)
  , path(std::move(path))
  , directory(directory)
  , sha256(std::move(sha256))
  , keymode(keymode)
{
}
auto
gameplay_logic::ChartData::getTitle() const -> QString
{
    return title;
}
auto
gameplay_logic::ChartData::getArtist() const -> QString
{
    return artist;
}
auto
gameplay_logic::ChartData::getNormalNoteCount() const -> int
{
    return normalNoteCount;
}
auto
gameplay_logic::ChartData::getLength() const -> int64_t
{
    return length;
}
auto
gameplay_logic::ChartData::getPath() const -> QString
{
    return path;
}
auto
gameplay_logic::ChartData::getRank() const -> int
{
    return rank;
}
auto
gameplay_logic::ChartData::getTotal() const -> double
{
    return total;
}
auto
gameplay_logic::ChartData::getPlayLevel() const -> int
{
    return playLevel;
}
auto
gameplay_logic::ChartData::getDifficulty() const -> int
{
    return difficulty;
}
auto
gameplay_logic::ChartData::getSubtitle() const -> QString
{
    return subtitle;
}
auto
gameplay_logic::ChartData::getSubartist() const -> QString
{
    return subartist;
}
auto
gameplay_logic::ChartData::getGenre() const -> QString
{
    return genre;
}
auto
gameplay_logic::ChartData::save(db::SqliteCppDb& db) const -> void
{
    auto query = db.createStatement(
      "INSERT OR REPLACE INTO charts (title, artist, subtitle, subartist, "
      "genre, stage_file, banner, back_bmp, rank, total, play_level, "
      "difficulty, is_random, random_sequence, normal_note_count, ln_count, "
      "mine_count, length, initial_bpm, max_bpm, "
      "min_bpm, path, chart_directory, directory, sha256, keymode) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
      "?, ?, ?, ?, ?);");
    query.reset();
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
    query.bind(16, lnCount);
    query.bind(17, mineCount);
    query.bind(18, length);
    query.bind(19, initialBpm);
    query.bind(20, maxBpm);
    query.bind(21, minBpm);
    query.bind(22, path.toStdString());
    query.bind(23, getChartDirectory().toStdString());
    if (directory == 0) {
        query.bind(24);
    } else {
        query.bind(24, directory);
    }
    query.bind(25, sha256.toStdString());
    query.bind(26, static_cast<int>(keymode));
    query.execute();
}
auto
gameplay_logic::ChartData::getSha256() const -> QString
{
    return sha256;
}

auto
gameplay_logic::ChartData::load(const DTO& chartDataDto)
  -> std::unique_ptr<ChartData>
{
    return std::make_unique<ChartData>(
      QString::fromStdString(chartDataDto.title),
      QString::fromStdString(chartDataDto.artist),
      QString::fromStdString(chartDataDto.subtitle),
      QString::fromStdString(chartDataDto.subartist),
      QString::fromStdString(chartDataDto.genre),
      QString::fromStdString(chartDataDto.stageFile),
      QString::fromStdString(chartDataDto.banner),
      QString::fromStdString(chartDataDto.backBmp),
      chartDataDto.rank,
      chartDataDto.total,
      chartDataDto.playLevel,
      chartDataDto.difficulty,
      static_cast<bool>(chartDataDto.isRandom),
      support::decompress<QList<int64_t>>(QByteArray::fromStdString(chartDataDto.randomSequence)),
      chartDataDto.normalNoteCount,
      chartDataDto.lnCount,
      chartDataDto.mineCount,
      chartDataDto.length,
      chartDataDto.initialBpm,
      chartDataDto.maxBpm,
      chartDataDto.minBpm,
      QString::fromStdString(chartDataDto.path),
      chartDataDto.directory,
      QString::fromStdString(chartDataDto.sha256),
      static_cast<Keymode>(chartDataDto.keymode));
}
auto
gameplay_logic::ChartData::getIsRandom() const -> bool
{
    return isRandom;
}
auto
gameplay_logic::ChartData::getKeymode() const
  -> gameplay_logic::ChartData::Keymode
{
    return keymode;
}
auto
gameplay_logic::ChartData::getStageFile() const -> QString
{
    return stageFile;
}
auto
gameplay_logic::ChartData::getBanner() const -> QString
{
    return banner;
}
auto
gameplay_logic::ChartData::getBackBmp() const -> QString
{
    return backBmp;
}
auto
gameplay_logic::ChartData::getDirectory() const -> QString
{
    if (directory == 0) {
        return "";
    }
    auto dir = getChartDirectory();
    dir = dir.left(dir.size() - 1);
    return dir.left(dir.lastIndexOf('/') + 1);
}
auto
gameplay_logic::ChartData::getChartDirectory() const -> QString
{
    return QFileInfo{path}.absolutePath() + '/';
}
auto
gameplay_logic::ChartData::getInitialBpm() const -> double
{
    return initialBpm;
}
auto
gameplay_logic::ChartData::getMaxBpm() const -> double
{
    return maxBpm;
}
auto
gameplay_logic::ChartData::getMinBpm() const -> double
{
    return minBpm;
}
auto
gameplay_logic::ChartData::getLnCount() const -> int
{
    return lnCount;
}
auto
gameplay_logic::ChartData::getMineCount() const -> int
{
    return mineCount;
}
