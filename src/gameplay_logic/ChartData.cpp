//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>
#include <spdlog/spdlog.h>
#include <zstd.h>

gameplay_logic::ChartData::ChartData(QString title,
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
                                     int noteCount,
                                     int64_t length,
                                     QString path,
                                     QString directoryInDb,
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
  , noteCount(noteCount)
  , length(length)
  , path(std::move(path))
  , directoryInDb(std::move(directoryInDb))
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
gameplay_logic::ChartData::getNoteCount() const -> int
{
    return noteCount;
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
    static thread_local auto query = db.createStatement(
      "INSERT OR REPLACE INTO charts (title, artist, subtitle, subartist, "
      "genre, stage_file, banner, back_bmp, rank, total, play_level, "
      "difficulty, is_random, note_count, length, path, directory_in_db, "
      "sha256, keymode) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
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
    query.bind(14, noteCount);
    query.bind(15, length);
    query.bind(16, path.toStdString());
    query.bind(17, directoryInDb.toStdString());
    query.bind(18, sha256.toStdString());
    query.bind(19, static_cast<int>(keymode));
    query.execute();
}
auto
gameplay_logic::ChartData::getSha256() const -> QString
{
    return sha256;
}
auto
gameplay_logic::ChartData::getDirectoryInDb() const -> QString
{
    return directoryInDb;
}

auto
gameplay_logic::ChartData::load(
  const gameplay_logic::ChartData::DTO& chartDataDto)
  -> std::unique_ptr<gameplay_logic::ChartData>
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
      chartDataDto.noteCount,
      chartDataDto.length,
      QString::fromStdString(chartDataDto.path),
      QString::fromStdString(chartDataDto.directoryInDb),
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
