//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>

gameplay_logic::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString level,
                                     int noteCount,
                                     int length,
                                     QUrl directory,
                                     BmsNotes* noteData,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , level(std::move(level))
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
gameplay_logic::ChartData::getLevel() const -> QString
{
    return level;
}
auto
gameplay_logic::ChartData::createEmptyScore() const -> gameplay_logic::BmsScore*
{
    return new gameplay_logic::BmsScore(noteCount);
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
