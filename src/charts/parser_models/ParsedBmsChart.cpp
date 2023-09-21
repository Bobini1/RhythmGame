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
        first.bpm = second.bpm;
    }
    if (second.total.has_value()) {
        first.total = second.total;
    }
    if (second.rank.has_value()) {
        first.rank = second.rank;
    }
    if (second.playLevel.has_value()) {
        first.playLevel = second.playLevel;
    }
    if (second.difficulty.has_value()) {
        first.difficulty = second.difficulty;
    }
    for (auto& [key, value] : second.exBpms) {
        first.exBpms[key] = value;
    }
    for (auto& [key, value] : second.wavs) {
        first.wavs[key] = std::move(value);
    }
    for (auto& [key, measure] : second.measures) {
        auto& firstMeasure = first.measures[key];
        for (auto column = 0; column < measure.p1VisibleNotes.size();
             ++column) {
            for (auto& definition : measure.p1VisibleNotes[column]) {
                firstMeasure.p1VisibleNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto column = 0; column < measure.p2VisibleNotes.size();
             ++column) {
            for (auto& definition : measure.p2VisibleNotes[column]) {
                firstMeasure.p2VisibleNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto column = 0; column < measure.p1InvisibleNotes.size();
             ++column) {
            for (auto& definition : measure.p1InvisibleNotes[column]) {
                firstMeasure.p1InvisibleNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto column = 0; column < measure.p2InvisibleNotes.size();
             ++column) {
            for (auto& definition : measure.p2InvisibleNotes[column]) {
                firstMeasure.p2InvisibleNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto column = 0; column < measure.p1LongNotes.size(); ++column) {
            for (auto& definition : measure.p1LongNotes[column]) {
                firstMeasure.p1LongNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto column = 0; column < measure.p2LongNotes.size(); ++column) {
            for (auto& definition : measure.p2LongNotes[column]) {
                firstMeasure.p2LongNotes[column].push_back(
                  std::move(definition));
            }
        }
        for (auto& definition : measure.bgmNotes) {
            firstMeasure.bgmNotes.push_back(std::move(definition));
        }
        for (auto& definition : measure.bpmChanges) {
            firstMeasure.bpmChanges.push_back(std::move(definition));
        }
        for (auto& definition : measure.exBpmChanges) {
            firstMeasure.exBpmChanges.push_back(std::move(definition));
        }
        if (measure.meter.has_value()) {
            firstMeasure.meter = measure.meter;
        }
    }
}
