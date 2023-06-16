//
// Created by bobini on 15.06.23.
//

#include <filesystem>
#include "BmsChart.h"
#include "sounds/OpenAlSoundBuffer.h"

charts::gameplay_models::BmsChart::BmsChart(
  const charts::parser_models::ParsedBmsChart& chart,
  const std::string& path)
{
    if (!chart.tags.randomBlocks.empty()) {
        throw std::runtime_error("Random blocks are not supported.");
    }
    loadSounds(chart.tags.wavs, path);
    generateMeasures(chart.tags.bpm.value_or(defaultBpm),
                     chart.tags.exBpms,
                     chart.tags.measures);
}
void
charts::gameplay_models::BmsChart::loadSounds(
  const std::map<std::string, std::string>& wavs,
  const std::string& path)
{
    sounds.reserve(wavs.size());
    std::unordered_map<std::filesystem::path,
                       std::shared_ptr<const sounds::OpenALSoundBuffer>>
      buffers;
    auto rootPath = std::filesystem::path(path).parent_path();
    for (const auto& [key, value] : wavs) {
        auto filePath = rootPath / value;
        auto buffer = buffers.find(filePath);
        auto bufferPointer = [&buffer, &buffers, &filePath] {
            if (buffer != buffers.end()) {
                return buffers.emplace(filePath, buffer->second).first->second;
            }
            return buffers
              .emplace(filePath,
                       std::make_shared<const sounds::OpenALSoundBuffer>(
                         filePath.c_str()))
              .first->second;
        }();
        sounds.emplace(key, sounds::OpenALSound(bufferPointer));
    }
}
void
charts::gameplay_models::BmsChart::generateMeasures(
  double baseBpm,
  const std::map<std::string, double>& bpms,
  const std::map<uint64_t, parser_models::ParsedBmsChart::Measure>& measures)
{
    auto lastBpm = baseBpm;
    auto lastMeasure = uint64_t{ 0 };
    auto measureStart = std::chrono::nanoseconds(0);
    for (const auto& [measureIndex, measure] : measures) {
        auto currentMeasure = measureIndex;
        fillEmptyMeasures(lastMeasure, currentMeasure, measureStart, lastBpm);
        auto bpmChangesInMeasure =
          std::map<double, std::pair<double, std::chrono::nanoseconds>>{
              { 0.0, { lastBpm, measureStart } }
          };
        auto index = -1;
        auto lastTimestamp = measureStart;
        auto lastFraction = 0.0;
        for (const auto& bpmChange : measure.exBpmChanges) {
            index++;
            if (bpmChange == "00") {
                continue;
            }
            auto bpmValue = bpms.find(bpmChange);
            if (bpmValue == bpms.end()) {
                continue;
            }
            auto fraction = static_cast<double>(index) /
                            static_cast<double>(measure.exBpmChanges.size());
            auto timestamp =
              lastTimestamp +
              std::chrono::nanoseconds(static_cast<int64_t>(
                (fraction - lastFraction) * 60 * 1000 * 1000 * 1000 / lastBpm));
            bpmChanges.emplace_back(timestamp, bpmValue->second);
            bpmChangesInMeasure.emplace(
              fraction, std::pair{ bpmValue->second, timestamp });
            lastTimestamp = timestamp;
            lastFraction = fraction;
            lastBpm = bpmValue->second;
        }
        // add last bpm change
        auto timestamp =
          lastTimestamp +
          std::chrono::nanoseconds(static_cast<int64_t>(
            (1.0 - lastFraction) * 60 * 1000 * 1000 * 1000 / lastBpm));
        barLines.emplace_back(timestamp);
        for (int i = 0; i < measure.columnNumber; i++) {
            calculateOffsetsForColumn(measure.p1VisibleNotes[i],
                                      visibleNotes[i],
                                      bpmChangesInMeasure,
                                      measure.meter);
            calculateOffsetsForColumn(measure.p1InvisibleNotes[i],
                                      invisibleNotes[i],
                                      bpmChangesInMeasure,
                                      measure.meter);
            calculateOffsetsForColumn(measure.p2VisibleNotes[i],
                                      visibleNotes[i + measure.columnNumber],
                                      bpmChangesInMeasure,
                                      measure.meter);
            calculateOffsetsForColumn(measure.p2InvisibleNotes[i],
                                      invisibleNotes[i + measure.columnNumber],
                                      bpmChangesInMeasure,
                                      measure.meter);
        }

        for (const auto& bgmNotes : measure.bgmNotes) {
            calculateOffsetsForBgm(
              bgmNotes, this->bgmNotes, bpmChangesInMeasure);
        }

        lastMeasure = currentMeasure;
        measureStart = timestamp;
    }
}

void
charts::gameplay_models::BmsChart::fillEmptyMeasures(
  uint64_t lastMeasure,
  uint64_t& measureIndex,
  std::chrono::nanoseconds measureStart,
  double lastBpm)
{
    for (; lastMeasure < measureIndex; ++lastMeasure) {
        auto measureLength = std::chrono::nanoseconds(
          static_cast<int64_t>(60.0 * 4 * 1000 * 1000 * 1000 / lastBpm));
        auto measureEnd = measureStart + measureLength;
        barLines.push_back(measureEnd);
        measureStart = measureEnd;
    }
}
void
charts::gameplay_models::BmsChart::calculateOffsetsForColumn(
  const std::vector<std::string>& notes,
  std::vector<std::pair<std::chrono::nanoseconds, Note>>& target,
  const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
    bpmChangesInMeasure,
  double meter)
{
    auto index = -1;
    for (const auto& note : notes) {
        index++;
        if (note == "00") {
            continue;
        }
        auto [timestamp, soundPointer, fraction] =
          createNoteInfo(notes, bpmChangesInMeasure, index, note);
        target.emplace_back(timestamp,
                            Note{ soundPointer, { fraction, meter * 4 } });
    }
}

void
charts::gameplay_models::BmsChart::calculateOffsetsForBgm(
  const std::vector<std::string>& notes,
  std::vector<std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>>&
    target,
  const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
    bpmChangesInMeasure)
{
    auto index = -1;
    for (const auto& note : notes) {
        index++;
        if (note == "00") {
            continue;
        }
        auto [timestamp, soundPointer, fraction] =
          createNoteInfo(notes, bpmChangesInMeasure, index, note);
        target.emplace_back(timestamp, soundPointer);
    }
}
auto
charts::gameplay_models::BmsChart::createNoteInfo(
  const std::vector<std::string>& notes,
  const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
    bpmChangesInMeasure,
  int index,
  const std::string& note)
  -> std::tuple<std::chrono::nanoseconds, sounds::OpenALSound*, double>
{
    auto sound = sounds.find(note);
    auto* soundPointer = sound != sounds.end() ? &sound->second : nullptr;
    auto fraction =
      static_cast<double>(index) / static_cast<double>(notes.size());
    auto [bpmFraction, bpmWithTimestamp] =
      *bpmChangesInMeasure.lower_bound(fraction);
    auto [bpm, bpmTimestamp] = bpmWithTimestamp;
    auto timestamp =
      bpmTimestamp +
      std::chrono::nanoseconds(static_cast<int64_t>(
        (fraction - bpmFraction) * 60 * 1000 * 1000 * 1000 / bpm));
    return { timestamp, soundPointer, fraction };
}
