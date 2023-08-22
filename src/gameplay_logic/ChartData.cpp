//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>

gameplay_logic::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString level,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , level(std::move(level))
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
gameplay_logic::ChartData::getLevel() const -> QString
{
    return level;
}
