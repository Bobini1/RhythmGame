//
// Created by bobini on 22.07.2022.
//

#include "BmsChart.h"
charts::models::BmsChart::BmsChart(Tags tags)
  : tags(std::move(tags))
{
}
auto
charts::models::BmsChart::writeFullData(
  behaviour::SongDataWriter writer) const -> void
{
    if (tags.title) {
        writer.writeVar("getTitle", [this] { return *tags.title; });
    }
    if (tags.artist) {
        writer.writeVar("getArtist", [this] { return *tags.artist; });
    }
    if (tags.bpm) {
        writer.writeVar("getBpm", [this] { return *tags.bpm; });
    }
    if (tags.subTitle) {
        writer.writeVar("getSubTitle", [this] { return *tags.subTitle; });
    }
    if (tags.subArtist) {
        writer.writeVar("getSubArtist", [this] { return *tags.subArtist; });
    }
    if (tags.genre) {
        writer.writeVar("getGenre", [this] { return *tags.genre; });
    }
}