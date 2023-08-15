//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>

qml_components::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString level,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , level(std::move(level))
{
}
qml_components::ChartData::ChartData(QObject* parent)
  : QObject(parent)
{
}
auto
qml_components::ChartData::getTitle() const -> QString
{
    return title;
}
auto
qml_components::ChartData::getArtist() const -> QString
{
    return artist;
}
auto
qml_components::ChartData::getLevel() const -> QString
{
    return level;
}
