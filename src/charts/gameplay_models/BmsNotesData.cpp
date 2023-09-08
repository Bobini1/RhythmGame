//
// Created by bobini on 15.06.23.
//

#include <filesystem>
#include "BmsNotesData.h"
#include "sounds/OpenAlSoundBuffer.h"

using namespace std::chrono_literals;
charts::gameplay_models::BmsNotesData::BmsNotesData(
  const charts::parser_models::ParsedBmsChart& chart)
{
    if (chart.tags.randomBlocks.size() != 0) {
        throw std::runtime_error("Random blocks are not supported.");
    }
    generateMeasures(chart.tags.bpm.value_or(defaultBpm),
                     chart.tags.exBpms,
                     chart.tags.measures);
}

struct BpmChangeDef
{
    double fraction;
    double bpm;
};

auto
combineBpmChanges(const std::vector<std::string>& bpmChanges,
                  const std::vector<std::string>& exBpmChanges,
                  const std::map<std::string, double>& bpms)
  -> std::vector<BpmChangeDef>
{
    auto index = -1;
    auto combinedBpmChanges = std::vector<BpmChangeDef>{};
    for (const auto& bpmChange : bpmChanges) {
        index++;
        if (bpmChange == "00") {
            continue;
        }
        auto bpmValue = bpms.find(bpmChange);
        if (bpmValue == bpms.end()) {
            continue;
        }
        auto fraction =
          static_cast<double>(index) / static_cast<double>(bpmChanges.size());
        combinedBpmChanges.emplace_back(
          BpmChangeDef{ fraction, bpmValue->second });
    }
    index = -1;
    for (const auto& bpmChange : exBpmChanges) {
        index++;
        if (bpmChange == "00") {
            continue;
        }
        auto idx = size_t{ 0 };
        auto bpmChangeNum = std::stoi(bpmChange, &idx, 16);
        if (idx != 2) {
            spdlog::warn("Invalid bpmChange change: {}", bpmChange);
            continue;
        }
        auto fraction =
          static_cast<double>(index) / static_cast<double>(exBpmChanges.size());
        combinedBpmChanges.emplace_back(
          BpmChangeDef{ fraction, static_cast<double>(bpmChangeNum) });
    }
    std::sort(
      combinedBpmChanges.begin(),
      combinedBpmChanges.end(),
      [](const auto& a, const auto& b) { return a.fraction < b.fraction; });
    return combinedBpmChanges;
}

void
charts::gameplay_models::BmsNotesData::generateMeasures(
  double baseBpm,
  const std::map<std::string, double>& bpms,
  const std::map<uint64_t, parser_models::ParsedBmsChart::Measure>& measures)
{
    auto lastBpm = baseBpm;
    auto lastMeasure = uint64_t{ 0 };
    auto measureStart = Time{ 0ns, 0.0 };
    bpmChanges.emplace_back(measureStart, baseBpm);
    for (const auto& [measureIndex, measure] : measures) {
        auto currentMeasure = measureIndex;
        fillEmptyMeasures(lastMeasure, currentMeasure, measureStart, lastBpm);
        auto bpmChangesInMeasure = std::map<double, std::pair<double, Time>>{
            { 0.0, { lastBpm, measureStart } }
        };
        auto lastTimestamp = measureStart;
        auto lastFraction = 0.0;
        auto combinedBpmChanges =
          combineBpmChanges(measure.exBpmChanges, measure.bpmChanges, bpms);
        for (const auto& bpmChange : combinedBpmChanges) {
            auto fraction = bpmChange.fraction;
            auto bpmChangeNum = bpmChange.bpm;
            auto timestamp =
              lastTimestamp +
              Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                      (fraction - lastFraction) * defaultBeatsPerMeasure *
                      measure.meter * 60 * 1'000'000'000 / lastBpm)),
                    (fraction - lastFraction) * defaultBeatsPerMeasure *
                      measure.meter };
            bpmChanges.emplace_back(timestamp, bpmChangeNum);
            bpmChangesInMeasure[fraction] =
              std::pair{ bpmChangeNum, timestamp };
            lastTimestamp = timestamp;
            lastFraction = fraction;
            lastBpm = bpmChangeNum;
        }
        // add last bpm change
        auto timestamp =
          lastTimestamp +
          Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                  (1.0 - lastFraction) * defaultBeatsPerMeasure *
                  measure.meter * 60 * 1'000'000'000 / lastBpm)),
                (1.0 - lastFraction) * defaultBeatsPerMeasure * measure.meter };
        barLines.emplace_back(timestamp);
        for (auto i = 0; i < columnMapping.size(); i++) {
            calculateOffsetsForColumn(measure.p1VisibleNotes[columnMapping[i]],
                                      visibleNotes[i],
                                      bpmChangesInMeasure,
                                      measure.meter);
            calculateOffsetsForColumn(
              measure.p1InvisibleNotes[columnMapping[i]],
              invisibleNotes[i],
              bpmChangesInMeasure,
              measure.meter);
            calculateOffsetsForColumn(
              measure.p2VisibleNotes[i],
              visibleNotes[columnMapping[i] + columnNumber / 2],
              bpmChangesInMeasure,
              measure.meter);
            calculateOffsetsForColumn(
              measure.p2InvisibleNotes[i],
              invisibleNotes[columnMapping[i] + columnNumber / 2],
              bpmChangesInMeasure,
              measure.meter);
        }

        for (const auto& bgmNotes : measure.bgmNotes) {
            calculateOffsetsForBgm(
              bgmNotes, this->bgmNotes, bpmChangesInMeasure, measure.meter);
        }

        lastMeasure = currentMeasure;
        measureStart = timestamp;
    }
    std::sort(bgmNotes.begin(), bgmNotes.end());
}

void
charts::gameplay_models::BmsNotesData::fillEmptyMeasures(uint64_t lastMeasure,
                                                         uint64_t& measureIndex,
                                                         Time& measureStart,
                                                         double lastBpm)
{
    lastMeasure++;
    for (; lastMeasure < measureIndex; ++lastMeasure) {
        auto measureLength =
          Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                  60.0 * defaultBeatsPerMeasure * 1'000'000'000 / lastBpm)),
                defaultBeatsPerMeasure };
        auto measureEnd = measureStart + measureLength;
        barLines.push_back(measureEnd);
        measureStart = measureEnd;
    }
}
void
charts::gameplay_models::BmsNotesData::calculateOffsetsForColumn(
  const std::vector<std::string>& notes,
  std::vector<Note>& target,
  const std::map<double, std::pair<double, Time>>& bpmChangesInMeasure,
  double meter)
{
    auto index = -1;
    for (const auto& note : notes) {
        index++;
        if (note == "00") {
            continue;
        }
        auto [timestamp, soundPointer, fraction] =
          createNoteInfo(notes, bpmChangesInMeasure, index, note, meter);
        target.emplace_back(Note{ timestamp,
                                  soundPointer,
                                  { fraction * meter * defaultBeatsPerMeasure,
                                    meter * defaultBeatsPerMeasure } });
    }
}

void
charts::gameplay_models::BmsNotesData::calculateOffsetsForBgm(
  const std::vector<std::string>& notes,
  std::vector<std::pair<Time, std::string>>& target,
  const std::map<double, std::pair<double, Time>>& bpmChangesInMeasure,
  double meter)
{
    auto index = -1;
    for (const auto& note : notes) {
        index++;
        if (note == "00") {
            continue;
        }
        auto [timestamp, soundPointer, fraction] =
          createNoteInfo(notes, bpmChangesInMeasure, index, note, meter);
        target.emplace_back(timestamp.timestamp, soundPointer);
    }
}
auto
charts::gameplay_models::BmsNotesData::createNoteInfo(
  const std::vector<std::string>& notes,
  const std::map<double, std::pair<double, Time>>& bpmChangesInMeasure,
  int index,
  const std::string& note,
  double meter) -> std::tuple<Time, std::string, double>
{
    auto fraction =
      static_cast<double>(index) / static_cast<double>(notes.size());
    // https://stackoverflow.com/q/45426556
    auto lastBpmChange = bpmChangesInMeasure.upper_bound(fraction);
    auto [bpmFraction, bpmWithTimestamp] = *std::prev(lastBpmChange);
    auto [bpm, bpmTimestamp] = bpmWithTimestamp;
    auto timestamp =
      bpmTimestamp +
      Time{ std::chrono::nanoseconds(static_cast<int64_t>(
              (fraction - bpmFraction) * meter * defaultBeatsPerMeasure * 60 *
              1'000'000'000 / bpm)),
            (fraction - bpmFraction) * meter * defaultBeatsPerMeasure };
    return { timestamp, note, fraction };
}
