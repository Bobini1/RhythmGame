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
  charts::behaviour::SongDataWriter writer) const -> void
{
    if (tags.title) {
        writer.writeVar("getTitle", [this] { return *tags.title; });
    } else {
        writer.writeVar("getTitle", [] { return sol::lua_nil; });
    }
    if (tags.artist) {
        writer.writeVar("getArtist", [this] { return *tags.artist; });
    } else {
        writer.writeVar("getArtist", [] { return sol::lua_nil; });
    }
    if (tags.bpm) {
        writer.writeVar("getBpm", [this] { return *tags.bpm; });
    } else {
        writer.writeVar("getBpm", [] { return sol::lua_nil; });
    }
    if (tags.subTitle) {
        writer.writeVar("getSubTitle", [this] { return *tags.subTitle; });
    } else {
        writer.writeVar("getSubTitle", [] { return sol::lua_nil; });
    }
    if (tags.subArtist) {
        writer.writeVar("getSubArtist", [this] { return *tags.subArtist; });
    } else {
        writer.writeVar("getSubArtist", [] { return sol::lua_nil; });
    }
    if (tags.genre) {
        writer.writeVar("getGenre", [this] { return *tags.genre; });
    } else {
        writer.writeVar("getGenre", [] { return sol::lua_nil; });
    }
}