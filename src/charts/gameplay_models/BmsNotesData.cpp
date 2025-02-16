//
// Created by bobini on 15.06.23.
//

#include <unordered_set>
#include <numeric>
#include "BmsNotesData.h"
#include "sounds/OpenAlSoundBuffer.h"

#include <span>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace charts::gameplay_models {
namespace {

struct BpmChangeDef
{
    double fraction;
    bool isStop;
    std::pair<int, int> fractionDec;
    double bpm;
};

enum class BpmChangeType
{
    Normal = 0,
    Stop = 1,
    AfterStop = 2
};

auto
combineBpmChanges(std::span<const uint16_t> exBpmChanges,
                  std::span<const uint16_t> bpmChanges,
                  std::span<const uint16_t> stops,
                  const std::unordered_map<uint16_t, double>& bpms,
                  const std::unordered_map<uint16_t, double>& stopDefs)
  -> std::vector<BpmChangeDef>
{
    auto index = -1;
    auto combinedBpmChanges = std::vector<BpmChangeDef>{};
    for (const auto& bpmChange : exBpmChanges) {
        index++;
        if (bpmChange == 0) {
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
          static_cast<double>(index) / static_cast<double>(exBpmChanges.size());
        auto gcd = std::gcd(index, static_cast<int>(exBpmChanges.size()));
        auto fractionDec =
          std::pair{ index / gcd, static_cast<int>(exBpmChanges.size()) / gcd };
        combinedBpmChanges.emplace_back(
          BpmChangeDef{ fraction, false, fractionDec, bpmValue->second });
    }
    index = -1;
    for (const auto& bpmChange : bpmChanges) {
        index++;
        if (bpmChange == 0) {
            continue;
        }
        auto fraction =
          static_cast<double>(index) / static_cast<double>(bpmChanges.size());
        auto gcd = std::gcd(index, static_cast<int>(bpmChanges.size()));
        auto fractionDec =
          std::pair{ index / gcd, static_cast<int>(bpmChanges.size()) / gcd };
        combinedBpmChanges.emplace_back(BpmChangeDef{
          fraction, false, fractionDec, static_cast<double>(bpmChange) });
    }
    index = -1;
    for (const auto& stop : stops) {
        index++;
        if (stop == 0) {
            continue;
        }
        auto stopValue = stopDefs.find(stop);
        if (stopValue == stopDefs.end()) {
            continue;
        }
        if (stopValue->second <= 0.0) {
            spdlog::warn("Stop must be positive, was: {}",
                         std::to_string(stopValue->second));
            continue;
        }
        auto fraction =
          static_cast<double>(index) / static_cast<double>(stops.size());
        auto gcd = std::gcd(index, static_cast<int>(stops.size()));
        auto fractionDec =
          std::pair{ index / gcd, static_cast<int>(stops.size()) / gcd };
        combinedBpmChanges.emplace_back(
          BpmChangeDef{ fraction, true, fractionDec, stopValue->second });
    }
    std::sort(combinedBpmChanges.begin(),
              combinedBpmChanges.end(),
              [](const auto& a, const auto& b) {
                  if (a.fractionDec == b.fractionDec) {
                      return a.isStop < b.isStop;
                  }
                  return a.fraction < b.fraction;
              });
    return combinedBpmChanges;
}

auto
createNoteInfo(
  const std::vector<uint16_t>& notes,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  int index,
  double meter) -> std::tuple<BmsNotesData::Time, double>
{
    auto fraction =
      static_cast<double>(index) / static_cast<double>(notes.size());
    // https://stackoverflow.com/q/45426556
    auto lastBpmChange =
      bpmChangesInMeasure.upper_bound({ fraction, BpmChangeType::Normal });
    auto iter = std::prev(lastBpmChange);
    auto [bpmFractionAndType, bpmWithTimestamp] = *iter;
    auto [bpmFraction, bpmChangeType] = bpmFractionAndType;
    auto [bpm, bpmTimestamp] = bpmWithTimestamp;
    auto timestamp = bpmTimestamp + BmsNotesData::Time{
        std::chrono::nanoseconds(static_cast<int64_t>(
          (fraction - bpmFraction) * meter *
          BmsNotesData::defaultBeatsPerMeasure * 60 * 1'000'000'000 / bpm)),
        (fraction - bpmFraction) * meter * BmsNotesData::defaultBeatsPerMeasure
    };
    return { timestamp, fraction };
}

auto
getLastNote(std::vector<BmsNotesData::Note>& target)
{
    auto last = target.rbegin();
    while (last != target.rend() &&
           (last->noteType == BmsNotesData::NoteType::Landmine ||
            last->noteType == BmsNotesData::NoteType::Invisible)) {
        ++last;
    }
    return last;
}

void
calculateOffsetsForColumn(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<BmsNotesData::Note>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter,
  const BmsNotesData::NoteType noteType,
  std::optional<uint16_t> lnObj)
{
    if (notes.empty()) {
        return;
    }
    if (notes.size() == 1) {
        auto index = -1;
        for (const auto& note : notes.at(0)) {
            index++;
            if (note == 0) {
                continue;
            }
            auto [timestamp, fraction] =
              createNoteInfo(notes.at(0), bpmChangesInMeasure, index, meter);
            auto thisNoteType = noteType;
            if (noteType == BmsNotesData::NoteType::Normal &&
                lnObj.has_value() && note == lnObj.value()) {
                // we don't ever want two ln ends in a row
                if (auto lastNote = getLastNote(target);
                    lastNote != target.rend() &&
                    lastNote->noteType != BmsNotesData::NoteType::LongNoteEnd) {
                    thisNoteType = BmsNotesData::NoteType::LongNoteEnd;
                    lastNote->noteType = BmsNotesData::NoteType::LongNoteBegin;
                } else {
                    spdlog::trace("Two LN endings in a row");
                }
            }
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure },
              thisNoteType,
              note });
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
            if (note == 0) {
                continue;
            }
            auto gcd = std::gcd(index, static_cast<int>(definition.size()));
            auto [timestamp, fraction] =
              createNoteInfo(definition, bpmChangesInMeasure, index, meter);
            notesMap[{ index / gcd,
                       static_cast<int>(definition.size()) / gcd }] =
              BmsNotesData::Note{
                  timestamp,
                  { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                    meter * BmsNotesData::defaultBeatsPerMeasure },
                  noteType,
                  note
              };
        }
        std::vector<BmsNotesData::Note> notesVector;
        for (const auto& [fractionDec, note] : notesMap) {
            notesVector.push_back(note);
        }
        // sort by timestamp
        std::sort(notesVector.begin(), notesVector.end(), [](const auto& a, const auto& b) {
            return a.time.timestamp < b.time.timestamp;
        });
        // add to target
        for (const auto& note : notesVector) {
            target.push_back(note);
        }
    }
    for (auto& [timestamp, note] : notesMap) {
        if (noteType == BmsNotesData::NoteType::Normal && lnObj.has_value() &&
            note.sound == lnObj.value()) {
            // we don't ever want two ln ends in a row
            if (auto lastNote = getLastNote(target);
                lastNote != target.rend() &&
                lastNote->noteType != BmsNotesData::NoteType::LongNoteEnd) {
                note.noteType = BmsNotesData::NoteType::LongNoteEnd;
                lastNote->noteType = BmsNotesData::NoteType::LongNoteBegin;
            } else {
                spdlog::trace("Two LN endings in a row");
            }
        }
        target.push_back(note);
    }
}

void
calculateOffsetsForLnRdm(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<BmsNotesData::Note>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter,
  bool& insideLn,
  std::optional<size_t>& lastInsertedRdmNote)
{
    if (notes.empty()) {
        return;
    }
    if (notes.size() == 1) {
        auto index = -1;
        for (const auto& note : notes.at(0)) {
            index++;
            if (note == 0) {
                continue;
            }
            auto [timestamp, fraction] =
              createNoteInfo(notes.at(0), bpmChangesInMeasure, index, meter);
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure },
              insideLn ? BmsNotesData::NoteType::LongNoteEnd
                       : BmsNotesData::NoteType::LongNoteBegin,
              note });
            insideLn = !insideLn;
            lastInsertedRdmNote = target.size() - 1;
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
            if (note == 0) {
                continue;
            }
            auto gcd = std::gcd(index, static_cast<int>(definition.size()));
            auto [timestamp, fraction] =
              createNoteInfo(definition, bpmChangesInMeasure, index, meter);
            notesMap[{ index / gcd,
                       static_cast<int>(definition.size()) / gcd }] =
              BmsNotesData::Note{
                  timestamp,
                  { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                    meter * BmsNotesData::defaultBeatsPerMeasure },
                  {},
                  note
              };
        }
        std::vector<BmsNotesData::Note> notesVector;
        for (const auto& [fractionDec, note] : notesMap) {
            notesVector.push_back(note);
        }
        // sort by timestamp
        std::sort(notesVector.begin(),
                  notesVector.end(),
                  [](const auto& a, const auto& b) {
                      return a.time.timestamp < b.time.timestamp;
                  });
        // add to target
        for (auto& note : notesVector) {
            note.noteType = insideLn ? BmsNotesData::NoteType::LongNoteEnd
                                     : BmsNotesData::NoteType::LongNoteBegin;
            insideLn = !insideLn;
            target.push_back(note);
        }
    }
    for (const auto& [timestamp, note] : notesMap) {
        target.push_back(note);
    }
}

void
addLnEndsMgq(
  std::vector<BmsNotesData::Note>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter,
  bool& insideLn)
{
    if (insideLn) {
        auto [timestamp, fraction] =
          createNoteInfo({ 0 }, bpmChangesInMeasure, 0, meter);
        target.emplace_back(BmsNotesData::Note{
          timestamp,
          { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
            meter * BmsNotesData::defaultBeatsPerMeasure },
          BmsNotesData::NoteType::LongNoteEnd });
        insideLn = false;
    }
}

void
calculateOffsetsForLnMgq(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<BmsNotesData::Note>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter,
  bool& insideLn)
{
    if (notes.empty()) {
        return;
    }
    if (notes.size() != 1) {
        spdlog::warn("MGQ type LN multiple definitions compounding is not "
                     "supported. Picking the last valid definition.");
    }

    auto index = -1;
    for (const auto& note : notes.back()) {
        index++;
        if (note == 0 && insideLn) {
            auto [timestamp, fraction] =
              createNoteInfo(notes.back(), bpmChangesInMeasure, index, meter);
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure },
              BmsNotesData::NoteType::LongNoteEnd,
              note });
            insideLn = false;
        } else if (note != 0 && !insideLn) {
            auto [timestamp, fraction] =
              createNoteInfo(notes.at(0), bpmChangesInMeasure, index, meter);
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure },
              BmsNotesData::NoteType::LongNoteBegin,
              note });
            insideLn = true;
        }
    }
}

void
calculateOffsetsForLandmine(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<BmsNotesData::Note>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter)
{
    if (notes.empty()) {
        return;
    }
    if (notes.size() == 1) {
        auto index = -1;
        for (const auto& note : notes.at(0)) {
            index++;
            if (note == 0) {
                continue;
            }
            auto [timestamp, fraction] =
              createNoteInfo(notes.at(0), bpmChangesInMeasure, index, meter);
            target.emplace_back(BmsNotesData::Note{
              timestamp,
              { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                meter * BmsNotesData::defaultBeatsPerMeasure },
              BmsNotesData::NoteType::Landmine,
              note });
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
            if (note == 0) {
                continue;
            }
            auto gcd = std::gcd(index, static_cast<int>(definition.size()));
            auto [timestamp, fraction] =
              createNoteInfo(definition, bpmChangesInMeasure, index, meter);
            notesMap[{ index / gcd,
                       static_cast<int>(definition.size()) / gcd }] =
              BmsNotesData::Note{
                  timestamp,
                  { fraction * meter * BmsNotesData::defaultBeatsPerMeasure,
                    meter * BmsNotesData::defaultBeatsPerMeasure },
                  BmsNotesData::NoteType::Landmine,
                  note
              };
        }
        std::vector<BmsNotesData::Note> notesVector;
        for (const auto& [fractionDec, note] : notesMap) {
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
calculateOffsetsForBgm(
  const std::vector<uint16_t>& notes,
  std::vector<std::pair<BmsNotesData::Time, uint16_t>>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter)
{
    auto index = -1;
    for (const auto& note : notes) {
        index++;
        if (note == 0) {
            continue;
        }
        auto [timestamp, fraction] =
          createNoteInfo(notes, bpmChangesInMeasure, index, meter);
        target.emplace_back(timestamp.timestamp, note);
    }
}

void
calculateOffsetsForBga(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<std::pair<BmsNotesData::Time, uint16_t>>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 std::pair<double, BmsNotesData::Time>>& bpmChangesInMeasure,
  double meter)
{
    if (notes.empty()) {
        return;
    }
    if (notes.size() == 1) {
        auto index = -1;
        for (const auto& note : notes.at(0)) {
            index++;
            if (note == 0) {
                continue;
            }
            auto [timestamp, fraction] =
              createNoteInfo(notes.at(0), bpmChangesInMeasure, index, meter);
            target.emplace_back(timestamp.timestamp, note);
        }
        return;
    }
    auto notesMap =
      std::map<std::pair<int, int>, std::pair<uint16_t, BmsNotesData::Time>>{};
    for (const auto& definition : notes) {
        auto index = -1;
        for (const auto& note : definition) {
            index++;
            if (note == 0) {
                continue;
            }
            auto gcd = std::gcd(index, static_cast<int>(definition.size()));
            auto [timestamp, fraction] =
              createNoteInfo(definition, bpmChangesInMeasure, index, meter);
            notesMap[{ index / gcd,
                       static_cast<int>(definition.size()) / gcd }] = {
                note, timestamp
            };
        }
        std::vector<std::pair<BmsNotesData::Time, uint16_t>> notesVector;
        for (const auto& [fractionDec, note] : notesMap) {
            notesVector.emplace_back(note.second, note.first);
        }
        // sort by timestamp
        std::sort(notesVector.begin(),
                  notesVector.end(),
                  [](const auto& a, const auto& b) {
                      return a.first.timestamp < b.first.timestamp;
                  });
        // add to target
        for (const auto& note : notesVector) {
            target.push_back(note);
        }
    }
}

void
removeInvalidNotes(
  std::array<std::vector<BmsNotesData::Note>, BmsNotesData::columnNumber> notes)
{
    for (auto columnIndex = 0; columnIndex < notes.size(); columnIndex++) {
        auto& column = notes.at(columnIndex);
        auto insideLn = false;
        std::erase_if(column, [&insideLn](const auto& note) {
            auto valid =
              (note.noteType == BmsNotesData::NoteType::LongNoteEnd) ||
              !insideLn;
            if (valid &&
                note.noteType == BmsNotesData::NoteType::LongNoteBegin) {
                insideLn = true;
            } else if (valid &&
                       note.noteType == BmsNotesData::NoteType::LongNoteEnd) {
                insideLn = false;
            } else if (!valid) {
                spdlog::trace("Invalid: note inside LN");
            }
            return !valid;
        });
    }
}

} // namespace

BmsNotesData::BmsNotesData(const charts::parser_models::ParsedBmsChart& chart)
{
    auto lnType = defaultLnType;
    if (chart.tags.lnType.has_value()) {
        lnType = static_cast<LnType>(chart.tags.lnType.value());
    }
    generateMeasures(chart.tags.bpm.value_or(defaultBpm),
                     chart.tags.exBpms,
                     chart.tags.stops,
                     chart.tags.measures,
                     lnType,
                     chart.tags.lnObj);
}
void
BmsNotesData::generateMeasures(
  double baseBpm,
  const std::unordered_map<uint16_t, double>& bpms,
  const std::unordered_map<uint16_t, double>& stops,
  const std::map<int64_t, parser_models::ParsedBmsChart::Measure>& measures,
  LnType lnType,
  std::optional<uint16_t> lnObj)
{
    auto lastBpm = baseBpm;
    auto lastMeasure = int64_t{ -1 };
    auto measureStart = Time{ 0ns, 0.0 };
    bpmChanges.emplace_back(measureStart, baseBpm);
    auto insideLnP1 =
      std::array<bool, parser_models::ParsedBmsChart::Measure::columnNumber>{};
    auto insideLnP2 =
      std::array<bool, parser_models::ParsedBmsChart::Measure::columnNumber>{};
    auto lastInsertedRdmNoteP1 =
      std::array<std::optional<size_t>,
                 parser_models::ParsedBmsChart::Measure::columnNumber>{};
    auto lastInsertedRdmNoteP2 =
      std::array<std::optional<size_t>,
                 parser_models::ParsedBmsChart::Measure::columnNumber>{};
    for (const auto& [measureIndex, measure] : measures) {
        auto currentMeasure = measureIndex;
        if (lnType == LnType::MGQ && currentMeasure > lastMeasure + 1) {
            adjustMgqLnEnds(lastBpm, measureStart, insideLnP1, insideLnP2);
        }
        fillEmptyMeasures(lastMeasure, currentMeasure, measureStart, lastBpm);
        auto bpmChangesInMeasure =
          std::map<std::pair<double, BpmChangeType>, std::pair<double, Time>>{
              { { 0.0, BpmChangeType::Normal }, { lastBpm, measureStart } }
          };
        auto lastTimestamp = measureStart;
        auto lastFraction = 0.0;
        auto combinedBpmChanges = combineBpmChanges(
          measure.exBpmChanges, measure.bpmChanges, measure.stops, bpms, stops);
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
            bpmChanges.emplace_back(timestamp,
                                    bpmChange.isStop ? 0 : bpmChangeNum);
            bpmChangesInMeasure[{ fraction,
                                  bpmChange.isStop ? BpmChangeType::Stop
                                                   : BpmChangeType::Normal }] =
              std::pair{ bpmChangeNum, timestamp };
            if (bpmChange.isStop) {
                // add another bpm change at the end of the stop
                timestamp =
                  timestamp +
                  Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                          (bpmChangeNum / 192) * defaultBeatsPerMeasure * 60 *
                          1'000'000'000 / lastBpm)),
                        0 };
                bpmChanges.emplace_back(timestamp, lastBpm);
                bpmChangesInMeasure[{ fraction, BpmChangeType::AfterStop }] =
                  std::pair{ lastBpm, timestamp };
            } else {
                lastBpm = bpmChangeNum;
            }
            lastTimestamp = timestamp;
            lastFraction = fraction;
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
            calculateOffsetsForColumn(
              measure.p1VisibleNotes.at(columnMapping.at(i)),
              notes.at(i),
              bpmChangesInMeasure,
              meter,
              NoteType::Normal,
              lnObj);
            calculateOffsetsForColumn(
              measure.p1InvisibleNotes.at(columnMapping.at(i)),
              notes.at(i),
              bpmChangesInMeasure,
              meter,
              NoteType::Invisible,
              std::nullopt);
            calculateOffsetsForColumn(
              measure.p2VisibleNotes.at(columnMapping.at(i)),
              notes.at(i + columnMapping.size()),
              bpmChangesInMeasure,
              meter,
              NoteType::Normal,
              lnObj);
            calculateOffsetsForColumn(
              measure.p2InvisibleNotes.at(columnMapping.at(i)),
              notes.at(i + columnMapping.size()),
              bpmChangesInMeasure,
              meter,
              NoteType::Invisible,
              std::nullopt);
            if (lnType == LnType::RDM) {
                calculateOffsetsForLnRdm(
                  measure.p1LongNotes.at(columnMapping.at(i)),
                  notes.at(i),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP1.at(columnMapping.at(i)),
                  lastInsertedRdmNoteP1.at(columnMapping.at(i)));
                calculateOffsetsForLnRdm(
                  measure.p2LongNotes.at(columnMapping.at(i)),
                  notes.at(i + columnMapping.size()),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP2.at(columnMapping.at(i)),
                  lastInsertedRdmNoteP2.at(columnMapping.at(i)));
            } else if (lnType == LnType::MGQ) {
                calculateOffsetsForLnMgq(
                  measure.p1LongNotes.at(columnMapping.at(i)),
                  notes.at(i),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP1.at(columnMapping.at(i)));
                calculateOffsetsForLnMgq(
                  measure.p2LongNotes.at(columnMapping.at(i)),
                  notes.at(i + columnMapping.size()),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP2.at(columnMapping.at(i)));
            }
            calculateOffsetsForLandmine(
              measure.p1Landmines.at(columnMapping.at(i)),
              notes.at(i),
              bpmChangesInMeasure,
              meter);
            calculateOffsetsForLandmine(
              measure.p2Landmines.at(columnMapping.at(i)),
              notes.at(i + columnMapping.size()),
              bpmChangesInMeasure,
              meter);
        }

        for (const auto& bgmNotes : measure.bgmNotes) {
            calculateOffsetsForBgm(
              bgmNotes, this->bgmNotes, bpmChangesInMeasure, meter);
        }
        calculateOffsetsForBga(
          measure.bgaBase, this->bgaBase, bpmChangesInMeasure, meter);
        calculateOffsetsForBga(
          measure.bgaLayer, this->bgaLayer, bpmChangesInMeasure, meter);
        calculateOffsetsForBga(
          measure.bgaLayer2, this->bgaLayer2, bpmChangesInMeasure, meter);
        calculateOffsetsForBga(
          measure.bgaPoor, this->bgaPoor, bpmChangesInMeasure, meter);

        lastMeasure = currentMeasure;
        measureStart = timestamp;
    }
    std::sort(bgmNotes.begin(), bgmNotes.end());
    if (lnType == LnType::RDM) {
        adjustRdmLnEnds(lastInsertedRdmNoteP1, lastInsertedRdmNoteP2);
    } else {
        adjustMgqLnEnds(lastBpm, measureStart, insideLnP1, insideLnP2);
    }
    for (auto& column : notes) {
        std::sort(
          column.begin(), column.end(), [](const auto& a, const auto& b) {
              if (a.time.timestamp == b.time.timestamp) {
                  return a.noteType < b.noteType;
              }
              return a.time.timestamp < b.time.timestamp;
          });
    }
    removeInvalidNotes(notes);
}
void
BmsNotesData::adjustMgqLnEnds(
  double lastBpm,
  BmsNotesData::Time measureStart,
  std::array<bool, parser_models::ParsedBmsChart::Measure::columnNumber>&
    insideLnP1,
  std::array<bool, parser_models::ParsedBmsChart::Measure::columnNumber>&
    insideLnP2)
{
    auto bpmChangesInMeasureTemp =
      std::map<std::pair<double, BpmChangeType>, std::pair<double, Time>>{
          { { 0.0, BpmChangeType::Normal }, { lastBpm, measureStart } }
      };
    for (auto i = 0; i < columnMapping.size(); i++) {
        addLnEndsMgq(this->notes.at(i),
                     bpmChangesInMeasureTemp,
                     1,
                     insideLnP1.at(columnMapping.at(i)));
        addLnEndsMgq(this->notes.at(i + columnMapping.size()),
                     bpmChangesInMeasureTemp,
                     1,
                     insideLnP2.at(columnMapping.at(i)));
    }
}
void
BmsNotesData::adjustRdmLnEnds(
  const std::array<std::optional<size_t>,
                   parser_models::ParsedBmsChart::Measure::columnNumber>&
    lastInsertedRdmNoteP1,
  const std::array<std::optional<size_t>,
                   parser_models::ParsedBmsChart::Measure::columnNumber>&
    lastInsertedRdmNoteP2)
{
    for (int i = 0; i < columnMapping.size(); i++) {
        auto lastNote = lastInsertedRdmNoteP1.at(columnMapping.at(i));
        if (!lastNote.has_value()) {
            continue;
        }
        if (notes.at(i).at(*lastNote).noteType == NoteType::LongNoteBegin) {
            notes.at(i).at(*lastNote).noteType = NoteType::Normal;
        }
        lastNote = lastInsertedRdmNoteP2.at(columnMapping.at(i));
        if (!lastNote.has_value()) {
            continue;
        }
        if (notes.at(i + columnMapping.size()).at(*lastNote).noteType ==
            NoteType::LongNoteBegin) {
            notes.at(i + columnMapping.size()).at(*lastNote).noteType =
              NoteType::Normal;
        }
    }
}

void
BmsNotesData::fillEmptyMeasures(int64_t lastMeasure,
                                int64_t measureIndex,
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