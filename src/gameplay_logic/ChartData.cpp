//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>

gameplay_logic::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString subtitle,
                                     QString subartist,
                                     QString genre,
                                     int rank,
                                     double total,
                                     int playLevel,
                                     int difficulty,
                                     int noteCount,
                                     int64_t length,
                                     QString path,
                                     BmsNotes* noteData,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , subtitle(std::move(subtitle))
  , subartist(std::move(subartist))
  , genre(std::move(genre))
  , rank(rank)
  , total(total)
  , playLevel(playLevel)
  , difficulty(difficulty)
  , noteCount(noteCount)
  , length(length)
  , path(std::move(path))
  , noteData(noteData)
{
    noteData->setParent(this);
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
gameplay_logic::ChartData::getNoteData() const -> gameplay_logic::BmsNotes*
{
    return noteData;
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
      "INSERT INTO charts (title, artist, subtitle, subartist, genre, rank, "
      "total, play_level, difficulty, note_count, length, path) VALUES "
      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    query.reset();
    query.bind(1, title.toStdString());
    query.bind(2, artist.toStdString());
    query.bind(3, subtitle.toStdString());
    query.bind(4, subartist.toStdString());
    query.bind(5, genre.toStdString());
    query.bind(6, rank);
    query.bind(7, total);
    query.bind(8, playLevel);
    query.bind(9, difficulty);
    query.bind(10, noteCount);
    query.bind(11, length);
    query.bind(12, path.toStdString());

    query.execute();
}
