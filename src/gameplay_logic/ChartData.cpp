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
                                     int length,
                                     QUrl directory,
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
  , directory(std::move(directory))
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
gameplay_logic::ChartData::getLength() const -> int
{
    return length;
}
auto
gameplay_logic::ChartData::getDirectory() const -> QUrl
{
    return directory;
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
