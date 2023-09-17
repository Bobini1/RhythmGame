//
// Created by bobini on 22.07.2022.
//

#include "ParsedBmsChart.h"
auto
charts::parser_models::ParsedBmsChart::mergeTags(
  charts::parser_models::ParsedBmsChart::Tags& first,
  charts::parser_models::ParsedBmsChart::Tags second) -> void
{
    if (second.title.has_value()) {
        first.title = std::move(second.title);
    }
    if (second.artist.has_value()) {
        first.artist = std::move(second.artist);
    }
    if (second.subTitle.has_value()) {
        first.subTitle = std::move(second.subTitle);
    }
    if (second.subArtist.has_value()) {
        first.subArtist = std::move(second.subArtist);
    }
    if (second.genre.has_value()) {
        first.genre = std::move(second.genre);
    }
    if (second.bpm.has_value()) {
        first.bpm = std::move(second.bpm);
    }
    if (second.total.has_value()) {
        first.total = std::move(second.total);
    }
    if (second.rank.has_value()) {
        first.rank = std::move(second.rank);
    }
    if (second.playLevel.has_value()) {
        first.playLevel = std::move(second.playLevel);
    }
    if (second.difficulty.has_value()) {
        first.difficulty = std::move(second.difficulty);
    }
    for (auto& [key, value] : second.exBpms) {
        first.exBpms[key] = value;
    }
    for (auto& [key, value] : second.wavs) {
        first.wavs[key] = std::move(value);
    }
    for (auto& [key, value] : second.measures) {
        first.measures[key] = std::move(value);
    }
}
