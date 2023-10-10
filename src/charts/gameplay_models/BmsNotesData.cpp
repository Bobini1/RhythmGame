//
// Created by bobini on 15.06.23.
//

#include <unordered_set>
#include <numeric>
#include "BmsNotesData.h"
#include "sounds/OpenAlSoundBuffer.h"

using namespace std::chrono_literals;

namespace charts::gameplay_models {
namespace {

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
        if (bpmValue->second <= 0.0) {
            throw std::runtime_error{ "Bpm must be positive, was: " +
                                      std::to_string(bpmValue->second) };
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
            spdlog::warn("Invalid bpm change: {}", bpmChange);
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

auto
createNoteInfo(const std::vector<std::string>& notes,
               const std::map<double, std::pair<double, BmsNotesData::Time>>&
                 bpmChangesInMeasure,
               int index,
               const std::string& note,
               double meter)
  -> std::tuple<BmsNotesData::Time, std::string, double>
{
    auto fraction =
      static_cast<double>(index) / static_cast<double>(notes.size());
    // https://stackoverflow.com/q/45426556
    auto lastBpmChange = bpmChangesInMeasure.upper_bound(fraction);
    auto [bpmFraction, bpmWithTimestamp] = *std::prev(lastBpmChange);
    auto [bpm, bpmTimestamp] = bpmWithTimestamp;
    auto timestamp = bpmTimestamp + BmsNotesData::Time{
        std::chrono::nanoseconds(static_cast<int64_t>(
          (fraction - bpmFraction) * meter *
          BmsNotesData::defaultBeatsPerMeasure * 60 * 1'000'000'000 / bpm)),
        (fraction - bpmFraction) * meter * BmsNotesData::defaultBeatsPerMeasure
    };
    return { timestamp, note, fraction };
}

void
calculateOffsetsForColumn(
  std::span<const std::vector<std::string>> notes,
  std::vector<BmsNotesData::Note>& target,
  const std::map<double, std::pair<double, BmsNotesData::Time>>&
    bpmChangesInMeasure,
  double meter)
{
    if (notes.size() == 0) {
        return;
    }
    if (notes.size() == 1) {
        auto index = -1;
        for (const auto& note : notes[0]) {
            index++;
            if (note == "00") {
                continue;
            }
            auto [timestamp, soundPointer, fraction] =
              createNoteInfo(notes[0], bpmChangesInMeasure, index, note, meter);
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              soundPointer,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure } });
        }
        return;
    }

    // When the same channel in the same <measure> duplicates, both are
    // compounded.
    // Priority is given to a side with a large line number. But 00 does not
    // overwrite an old place.
    // https://hitkey.nekokan.dyndns.info/cmds.htm#BEHAVIOR-IN-GENERAL-IMPLEMENTATION
    auto notesMap = std::map<std::pair<int, int>, BmsNotesData::Note>{};
    for (const auto& definition : notes) {
        auto index = -1;
        for (const auto& note : definition) {
            index++;
            if (note == "00") {
                continue;
            }
            auto gcd = std::gcd(index, static_cast<int>(definition.size()));
            auto [timestamp, soundPointer, fraction] = createNoteInfo(
              definition, bpmChangesInMeasure, index, note, meter);
            notesMap[{ index / gcd,
                       static_cast<int>(definition.size()) / gcd }] =
              BmsNotesData::Note{
                  timestamp,
                  soundPointer,
                  { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                    meter * BmsNotesData::defaultBeatsPerMeasure }
              };
        }
        std::vector<BmsNotesData::Note> notesVector;
        for (const auto& [timestamp, note] : notesMap) {
            notesVector.push_back(note);
        }
        // sort by timestamp
        std::sort(notesVector.begin(),
                  notesVector.end(),
                  [](const auto& a, const auto& b) {
                      return a.time.timestamp < b.time.timestamp;
                  });
        // add to target
        for (const auto& note : notesVector) {
            target.push_back(note);
        }
    }
    for (const auto& [timestamp, note] : notesMap) {
        target.push_back(note);
    }
}

void
calculateOffsetsForBgmOrBga(
  const std::vector<std::string>& notes,
  std::vector<std::pair<BmsNotesData::Time, std::string>>& target,
  const std::map<double, std::pair<double, BmsNotesData::Time>>&
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
          createNoteInfo(notes, bpmChangesInMeasure, index, note, meter);
        target.emplace_back(timestamp.timestamp, soundPointer);
    }
}

} // namespace

BmsNotesData::BmsNotesData(const charts::parser_models::ParsedBmsChart& chart)
{
    generateMeasures(chart.tags.bpm.value_or(defaultBpm),
                     chart.tags.exBpms,
                     chart.tags.measures);
}
void
BmsNotesData::generateMeasures(
  double baseBpm,
  const std::map<std::string, double>& bpms,
  const std::map<int64_t, parser_models::ParsedBmsChart::Measure>& measures)
{
    auto lastBpm = baseBpm;
    auto lastMeasure = int64_t{ -1 };
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
        auto meter = measure.meter.value_or(
          charts::parser_models::ParsedBmsChart::Measure::defaultMeter);
        for (const auto& bpmChange : combinedBpmChanges) {
            auto fraction = bpmChange.fraction;
            auto bpmChangeNum = bpmChange.bpm;
            auto timestamp = lastTimestamp + Time{
                std::chrono::nanoseconds(static_cast<int64_t>(
                  (fraction - lastFraction) * defaultBeatsPerMeasure * meter *
                  60 * 1'000'000'000 / lastBpm)),
                (fraction - lastFraction) * defaultBeatsPerMeasure * meter
            };
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
                  (1.0 - lastFraction) * defaultBeatsPerMeasure * meter * 60 *
                  1'000'000'000 / lastBpm)),
                (1.0 - lastFraction) * defaultBeatsPerMeasure * meter };
        barLines.emplace_back(timestamp);
        for (auto i = 0; i < columnMapping.size(); i++) {
            calculateOffsetsForColumn(measure.p1VisibleNotes[columnMapping[i]],
                                      visibleNotes[i],
                                      bpmChangesInMeasure,
                                      meter);
            calculateOffsetsForColumn(
              measure.p1InvisibleNotes[columnMapping[i]],
              invisibleNotes[i],
              bpmChangesInMeasure,
              meter);
            calculateOffsetsForColumn(
              measure.p2VisibleNotes[i],
              visibleNotes[columnMapping[i] + columnMapping.size() / 2],
              bpmChangesInMeasure,
              meter);
            calculateOffsetsForColumn(
              measure.p2InvisibleNotes[i],
              invisibleNotes[columnMapping[i] + columnMapping.size() / 2],
              bpmChangesInMeasure,
              meter);
        }

        for (const auto& bgmNotes : measure.bgmNotes) {
            calculateOffsetsForBgmOrBga(
              bgmNotes, this->bgmNotes, bpmChangesInMeasure, meter);
        }
        for (const auto& bgaBase : measure.bgaBase) {
            calculateOffsetsForBgmOrBga(
              bgaBase, this->bgaBase, bpmChangesInMeasure, meter);
        }
        for (const auto& bgaPoor : measure.bgaPoor) {
            calculateOffsetsForBgmOrBga(
              bgaPoor, this->bgaPoor, bpmChangesInMeasure, meter);
        }
        for (const auto& bgaLayer : measure.bgaLayer) {
            calculateOffsetsForBgmOrBga(
              bgaLayer, this->bgaLayer, bpmChangesInMeasure, meter);
        }
        for (const auto& bgaLayer2 : measure.bgaLayer2) {
            calculateOffsetsForBgmOrBga(
              bgaLayer2, this->bgaLayer2, bpmChangesInMeasure, meter);
        }

        lastMeasure = currentMeasure;
        measureStart = timestamp;
    }
    std::sort(bgmNotes.begin(), bgmNotes.end());
}

void
BmsNotesData::fillEmptyMeasures(int64_t lastMeasure,
                                int64_t& measureIndex,
                                BmsNotesData::Time& measureStart,
                                double lastBpm)
{
    lastMeasure++;
    for (; lastMeasure < measureIndex; ++lastMeasure) {
        auto measureLength =
          BmsNotesData::Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                                60.0 * BmsNotesData::defaultBeatsPerMeasure *
                                1'000'000'000 / lastBpm)),
                              BmsNotesData::defaultBeatsPerMeasure };
        auto measureEnd = measureStart + measureLength;
        barLines.push_back(measureEnd);
        measureStart = measureEnd;
    }
}
} // namespace charts::gameplay_models