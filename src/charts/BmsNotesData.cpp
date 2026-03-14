//
// Created by bobini on 15.06.23.
//

#include <algorithm>
#include <numeric>
#include <set>
#include <span>
#include "BmsNotesData.h"

#include "Base62.h"
#include "sounds/SoundBuffer.h"

#include <ranges>
#include <spdlog/spdlog.h>
#include <QJsonArray>

using namespace std::chrono_literals;

namespace charts {
namespace {

struct BpmChangeDef
{
    enum class Type
    {
        BpmChange,
        Stop,
        Scroll
    };
    double fraction;
    Type type;
    std::pair<int, int> fractionDec;
    double value;
};

enum class BpmChangeType
{
    NormalOrScroll = 0,
    Stop = 1,
    AfterStop = 2,
};

auto
combineBpmChanges(std::span<const std::vector<uint16_t>> exBpmChanges,
                  std::span<const std::vector<uint16_t>> bpmChanges,
                  std::span<const std::vector<uint16_t>> stops,
                  std::span<const std::vector<uint16_t>> scrolls,
                  const std::unordered_map<uint16_t, double>& bpms,
                  const std::unordered_map<uint16_t, double>& stopDefs,
                  const std::unordered_map<uint16_t, double>& scrollDefs)
  -> std::vector<BpmChangeDef>
{
    auto combinedBpmChanges = std::vector<BpmChangeDef>{};
    for (const auto& exBpmChangeDefs : exBpmChanges) {
        auto index = -1;
        for (const auto& bpmChange : exBpmChangeDefs) {
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
            auto fraction = static_cast<double>(index) /
                            static_cast<double>(exBpmChangeDefs.size());
            auto gcd =
              std::gcd(index, static_cast<int>(exBpmChangeDefs.size()));
            auto fractionDec =
              std::pair{ index / gcd,
                         static_cast<int>(exBpmChangeDefs.size()) / gcd };
            combinedBpmChanges.emplace_back(
              BpmChangeDef{ fraction,
                            BpmChangeDef::Type::BpmChange,
                            fractionDec,
                            bpmValue->second });
        }
    }
    for (const auto& bpmChangeDefs : bpmChanges) {
        auto index = -1;
        for (const auto& bpmChange : bpmChangeDefs) {
            index++;
            if (bpmChange == 0) {
                continue;
            }
            auto fraction = static_cast<double>(index) /
                            static_cast<double>(bpmChangeDefs.size());
            auto gcd = std::gcd(index, static_cast<int>(bpmChangeDefs.size()));
            auto fractionDec =
              std::pair{ index / gcd,
                         static_cast<int>(bpmChangeDefs.size()) / gcd };
            combinedBpmChanges.emplace_back(
              BpmChangeDef{ fraction,
                            BpmChangeDef::Type::BpmChange,
                            fractionDec,
                            static_cast<double>(bpmChange) });
        }
    }
    for (const auto& stopDefsVec : stops) {
        auto index = -1;
        for (const auto& stop : stopDefsVec) {
            index++;
            if (stop == 0) {
                continue;
            }
            auto stopValue = stopDefs.find(stop);
            if (stopValue == stopDefs.end()) {
                continue;
            }
            if (stopValue->second <= 0.0) {
                spdlog::debug("Stop must be positive, was: {}",
                              std::to_string(stopValue->second));
                continue;
            }
            auto fraction = static_cast<double>(index) /
                            static_cast<double>(stopDefsVec.size());
            auto gcd = std::gcd(index, static_cast<int>(stopDefsVec.size()));
            auto fractionDec =
              std::pair{ index / gcd,
                         static_cast<int>(stopDefsVec.size()) / gcd };
            combinedBpmChanges.emplace_back(
              BpmChangeDef{ fraction,
                            BpmChangeDef::Type::Stop,
                            fractionDec,
                            stopValue->second });
        }
    }
    for (const auto& scrollDefsVec : scrolls) {
        auto index = -1;
        for (const auto& scroll : scrollDefsVec) {
            index++;
            if (scroll == 0) {
                continue;
            }
            auto scrollValue = scrollDefs.find(scroll);
            if (scrollValue == scrollDefs.end()) {
                continue;
            }
            auto fraction = static_cast<double>(index) /
                            static_cast<double>(scrollDefsVec.size());
            auto gcd = std::gcd(index, static_cast<int>(scrollDefsVec.size()));
            auto fractionDec =
              std::pair{ index / gcd,
                         static_cast<int>(scrollDefsVec.size()) / gcd };
            combinedBpmChanges.emplace_back(
              BpmChangeDef{ fraction,
                            BpmChangeDef::Type::Scroll,
                            fractionDec,
                            scrollValue->second });
        }
    }
    std::ranges::stable_sort(combinedBpmChanges,
                             [](const auto& a, const auto& b) {
                                 if (a.fractionDec == b.fractionDec) {
                                     return a.type < b.type;
                                 }
                                 return a.fraction < b.fraction;
                             });
    return combinedBpmChanges;
}

auto
createNoteInfo(
  const std::vector<uint16_t>& notes,
  const std::map<std::pair<double, BpmChangeType>,
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
  int index,
  double meter) -> std::tuple<BmsNotesData::Time, double>
{
    auto fraction =
      static_cast<double>(index) / static_cast<double>(notes.size());
    // https://stackoverflow.com/q/45426556
    auto lastBpmChange = bpmChangesInMeasure.upper_bound(
      { fraction, BpmChangeType::NormalOrScroll });
    auto iter = std::prev(lastBpmChange);
    auto [bpmFractionAndType, bpmWithTimestamp] = *iter;
    auto [bpmFraction, bpmChangeType] = bpmFractionAndType;
    auto [bpm, scroll, bpmTimestamp] = bpmWithTimestamp;
    auto timestamp = bpmTimestamp + BmsNotesData::Time{
        std::chrono::nanoseconds(static_cast<int64_t>(
          (fraction - bpmFraction) * meter *
          BmsNotesData::defaultBeatsPerMeasure * 60 * 1'000'000'000 / bpm)),
        (fraction - bpmFraction) * meter * BmsNotesData::defaultBeatsPerMeasure,
        (fraction - bpmFraction) * meter *
          BmsNotesData::defaultBeatsPerMeasure * scroll
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
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
    }
    std::vector<BmsNotesData::Note> notesVector;
    for (const auto& note : notesMap | std::views::values) {
        notesVector.push_back(note);
    }
    // sort by timestamp
    std::ranges::sort(notesVector, [](const auto& a, const auto& b) {
        return a.time.timestamp < b.time.timestamp;
    });
    for (auto& note : notesVector) {
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
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
        for (const auto& note : notesMap | std::views::values) {
            notesVector.push_back(note);
        }
        // sort by timestamp
        std::ranges::sort(notesVector, [](const auto& a, const auto& b) {
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
addLnEndsMgq(std::vector<BmsNotesData::Note>& target,
             const std::map<std::pair<double, BpmChangeType>,
                            BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
    }
    std::vector<BmsNotesData::Note> notesVector;
    for (const auto& [fractionDec, note] : notesMap) {
        notesVector.push_back(note);
    }
    // sort by timestamp
    std::ranges::sort(notesVector, [](const auto& a, const auto& b) {
        return a.time.timestamp < b.time.timestamp;
    });
    // add to target
    for (const auto& note : notesVector) {
        target.push_back(note);
    }
}

void
calculateOffsetsForBgm(
  const std::vector<uint16_t>& notes,
  std::vector<std::pair<BmsNotesData::Time, uint64_t>>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
        target.emplace_back(timestamp, note);
    }
}

void
calculateOffsetsForBga(
  const std::vector<std::vector<uint16_t>>& notes,
  std::vector<std::pair<BmsNotesData::Time, uint64_t>>& target,
  const std::map<std::pair<double, BpmChangeType>,
                 BmsNotesData::BpmChangeValues>& bpmChangesInMeasure,
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
            target.emplace_back(timestamp, note);
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
        for (const auto& note : notesMap | std::views::values) {
            notesVector.emplace_back(note.second, note.first);
        }
        // sort by timestamp
        std::ranges::stable_sort(notesVector, [](const auto& a, const auto& b) {
            return a.first.timestamp < b.first.timestamp;
        });
        // add to target
        for (const auto& note : notesVector) {
            target.push_back(note);
        }
    }
}

void
removeInvalidNotes(std::array<std::vector<BmsNotesData::Note>,
                              BmsNotesData::columnNumber>& notes)
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

template<typename T>
auto
resolveBase(int base, const std::vector<std::pair<uint16_t, T>>& values)
{
    auto map = std::unordered_map<uint16_t, T>{};
    for (const auto& [identifier, value] : values) {
        if (base == 36) {
            auto resolvedIdentifier = charts::base62ToBase36(identifier);
            map[resolvedIdentifier] = value;
        } else {
            map[identifier] = value;
        }
    }
    return map;
}

constexpr auto convertData = [](auto& data) {
    std::ranges::for_each(data | std::ranges::views::join,
                          [](auto& id) { id = charts::base62ToBase36(id); });
};
constexpr auto convertNestedData = [](auto& data) {
    std::ranges::for_each(data | std::ranges::views::join |
                            std::ranges::views::join,
                          [](auto& id) { id = charts::base62ToBase36(id); });
};
void
convertMeasuresBaseFrom62To36(
  std::map<int64_t, ParsedBmsChart::Measure>& measures)
{
    for (auto& measureData : measures | std::views::values) {
        convertData(measureData.bgaBase);
        convertData(measureData.bgaPoor);
        convertData(measureData.bgaLayer);
        convertData(measureData.bgaLayer2);
        convertData(measureData.bgmNotes);
        convertData(measureData.exBpmChanges);
        convertData(measureData.stops);
        convertData(measureData.scrolls);
        convertData(measureData.speeds);
        convertNestedData(measureData.p1VisibleNotes);
        convertNestedData(measureData.p1InvisibleNotes);
        convertNestedData(measureData.p1LongNotes);
        convertNestedData(measureData.p1Landmines);
        convertNestedData(measureData.p2VisibleNotes);
        convertNestedData(measureData.p2InvisibleNotes);
        convertNestedData(measureData.p2LongNotes);
        convertNestedData(measureData.p2Landmines);
    }
}
} // namespace

auto
BmsNotesData::fromParsedChart(const ParsedBmsChart& chart) -> BmsNotesData
{
    const auto base = chart.tags.base.value_or(defaultBase);
    auto data = BmsNotesData{};
    auto lnType = defaultLnType;
    if (chart.tags.lnType.has_value()) {
        lnType = static_cast<LnType>(chart.tags.lnType.value());
    }
    const auto bpm = chart.tags.bpm.value_or(defaultBpm);
    if (bpm <= 0.0) {
        throw std::runtime_error{ "Initial bpm must be positive, was: " +
                                  std::to_string(bpm) };
    }
    auto exBpmsMap = resolveBase(base, chart.tags.exBpms);
    auto stopsMap = resolveBase(base, chart.tags.stops);
    auto scrollsMap = resolveBase(base, chart.tags.scrolls);
    auto speedsMap = resolveBase(base, chart.tags.speeds);
    auto lnObj =
      base == 36 ? chart.tags.lnObj.transform(charts::base62ToBase36<uint16_t>)
                 : chart.tags.lnObj;
    auto measuresCopy = chart.tags.measures;
    if (base == 36) {
        convertMeasuresBaseFrom62To36(measuresCopy);
    }
    data.generateMeasures(bpm,
                          exBpmsMap,
                          stopsMap,
                          scrollsMap,
                          speedsMap,
                          measuresCopy,
                          lnType,
                          lnObj);
    return data;
}
BmsNotesData
BmsNotesData::fromBmson(const QJsonObject& bmson)
{
    auto data = BmsNotesData{};

    // Reject if version is not 1.0.0
    const auto version = bmson["version"].toString();
    if (version != "1.0.0") {
        throw std::runtime_error{ "Unsupported bmson version: " +
                                  version.toStdString() };
    }

    // ── 1. Read basic info ──────────────────────────────────────────────
    const auto info = bmson["info"].toObject();

    auto hint = info["mode_hint"].toString();
    if (hint != "beat-5k" && hint != "beat-7k" && hint != "beat-10k" &&
        hint != "beat-14k" && hint != "") {
        throw std::runtime_error{ "Unsupported bmson mode: " +
                                  hint.toStdString() };
    }
    const auto initBpm = info["init_bpm"].toDouble(defaultBpm);
    if (initBpm <= 0.0) {
        throw std::runtime_error{ "Initial bpm must be positive, was: " +
                                  std::to_string(initBpm) };
    }
    // Resolution: pulses per quarter note (default 240)
    const auto resolution = info["resolution"].toInteger(240);
    if (resolution <= 0) {
        throw std::runtime_error{ "Resolution must be positive, was: " +
                                  std::to_string(resolution) };
    }

    // ── 2. Collect BPM change events ────────────────────────────────────
    struct BpmEvent
    {
        int64_t pulse;
        double bpm;
    };
    auto bpmEvents = std::vector<BpmEvent>{};
    bpmEvents.push_back({ 0, initBpm });
    for (const auto& ev : bmson["bpm_events"].toArray()) {
        auto obj = ev.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        auto bpm = obj["bpm"].toDouble(0);
        if (bpm <= 0.0) {
            spdlog::debug("Skipping non-positive BPM event: {}",
                          std::to_string(bpm));
            continue;
        }
        bpmEvents.push_back({ pulse, bpm });
    }
    std::ranges::sort(bpmEvents, [](const auto& a, const auto& b) {
        return a.pulse < b.pulse;
    });
    // Deduplicate: if multiple events at the same pulse, keep the last one
    bpmEvents.erase(std::ranges::unique(bpmEvents,
                                        [](const auto& a, const auto& b) {
                                            return a.pulse == b.pulse;
                                        })
                      .begin(),
                    bpmEvents.end());

    // ── 3. Collect stop events ──────────────────────────────────────────
    struct StopEvent
    {
        int64_t pulse;
        int64_t duration; // in pulses
    };
    auto stopEvents = std::vector<StopEvent>{};
    for (const auto& ev : bmson["stop_events"].toArray()) {
        auto obj = ev.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        auto dur = static_cast<int64_t>(obj["duration"].toDouble(0));
        if (dur <= 0) {
            spdlog::debug("Skipping non-positive stop: {}", dur);
            continue;
        }
        stopEvents.push_back({ pulse, dur });
    }
    std::ranges::sort(stopEvents, [](const auto& a, const auto& b) {
        return a.pulse < b.pulse;
    });

    // ── 4. Build a timeline for pulse→Time conversion ───────────────────
    // A timeline entry records the cumulative Time at a given pulse, plus
    // the BPM in effect from that point forward.
    struct TimelineEntry
    {
        int64_t pulse;
        Time time;
        double bpm;
    };
    auto timeline = std::vector<TimelineEntry>{};

    // Merge BPM changes and stops into one sorted list of pulse positions.
    auto allPulses = std::set<int64_t>{};
    for (const auto& ev : bpmEvents) {
        allPulses.insert(ev.pulse);
    }
    for (const auto& ev : stopEvents) {
        allPulses.insert(ev.pulse);
    }

    // Walk through the timeline, accumulating time
    auto currentBpm = initBpm;
    auto currentTime = Time{ 0ns, 0.0, 0.0 };
    auto lastPulse = int64_t{ 0 };
    auto bpmIt = bpmEvents.begin();
    auto stopIt = stopEvents.begin();

    // Helper: advance time from lastPulse to targetPulse at currentBpm
    auto advanceTime = [&](int64_t targetPulse) {
        if (targetPulse > lastPulse) {
            auto deltaPulses = targetPulse - lastPulse;
            auto deltaNs = static_cast<int64_t>(
              static_cast<double>(deltaPulses) / resolution * 60.0 *
              1'000'000'000 / currentBpm);
            auto deltaPos = static_cast<double>(deltaPulses) / resolution;
            currentTime =
              currentTime +
              Time{ std::chrono::nanoseconds(deltaNs), deltaPos, deltaPos };
            lastPulse = targetPulse;
        }
    };

    // Process timeline
    timeline.push_back({ 0, currentTime, currentBpm });
    data.bpmChanges.emplace_back(currentBpm, defaultScroll, currentTime);

    for (auto pulse : allPulses) {
        // Advance time to this pulse
        advanceTime(pulse);

        // Apply BPM change if one exists at this pulse
        while (bpmIt != bpmEvents.end() && bpmIt->pulse == pulse) {
            currentBpm = bpmIt->bpm;
            if (pulse > 0 || bpmIt != bpmEvents.begin()) {
                // Don't double-add the initial BPM
                data.bpmChanges.emplace_back(
                  currentBpm, defaultScroll, currentTime);
            }
            ++bpmIt;
        }

        timeline.push_back({ pulse, currentTime, currentBpm });

        // Apply stops at this pulse
        while (stopIt != stopEvents.end() && stopIt->pulse == pulse) {
            // A stop adds time but no position change
            data.bpmChanges.emplace_back(0.0, defaultScroll, currentTime);
            auto stopNs = static_cast<int64_t>(
              static_cast<double>(stopIt->duration) / resolution * 60.0 *
              1'000'000'000 / currentBpm);
            currentTime =
              currentTime + Time{ std::chrono::nanoseconds(stopNs), 0.0, 0.0 };
            data.bpmChanges.emplace_back(
              currentBpm, defaultScroll, currentTime);
            // Update the timeline entry with the post-stop time
            timeline.back() = { pulse, currentTime, currentBpm };
            ++stopIt;
        }
    }

    // ── 5. pulseToTime conversion function ──────────────────────────────
    auto pulseToTime = [&timeline, resolution](int64_t pulse) -> Time {
        // Binary search for the last timeline entry at or before this pulse
        auto it = std::upper_bound(timeline.begin(),
                                   timeline.end(),
                                   pulse,
                                   [](int64_t p, const TimelineEntry& entry) {
                                       return p < entry.pulse;
                                   });
        if (it != timeline.begin()) {
            --it;
        }
        auto deltaPulses = pulse - it->pulse;
        auto deltaNs =
          static_cast<int64_t>(static_cast<double>(deltaPulses) / resolution *
                               60.0 * 1'000'000'000 / it->bpm);
        auto deltaPos = static_cast<double>(deltaPulses) / resolution;
        return it->time +
               Time{ std::chrono::nanoseconds(deltaNs), deltaPos, deltaPos };
    };

    auto pulseToSnap = [resolution](int64_t pulse) -> Snap {
        // Express the pulse position within a 4-beat measure
        auto pulsesPerMeasure = resolution * defaultBeatsPerMeasure;
        auto posInMeasure = pulse % pulsesPerMeasure;
        if (posInMeasure < 0) {
            posInMeasure += pulsesPerMeasure;
        }
        auto gcd =
          std::gcd(posInMeasure, static_cast<int64_t>(pulsesPerMeasure));
        return { static_cast<double>(posInMeasure / gcd),
                 static_cast<double>(pulsesPerMeasure / gcd) };
    };

    // ── 6. Column mapping from bmson x values ───────────────────────────
    // bmson x: 1=scratch, 2-8=keys 1-7 for P1; 9-16 for P2; 0=BGM
    // Internal columns: 0-6=keys 1-7, 7=scratch for P1
    //                   8-14=keys 1-7, 15=scratch for P2
    auto bmsonXToColumn = [](int x) -> std::optional<int> {
        if (x == 0) {
            return std::nullopt; // BGM
        }

        if (x > 0 && x <= 16) {
            return x - 1;
        }
        throw std::runtime_error{
            "Unsupported keymode, encountered column index " + std::to_string(x)
        };
    };

    // ── 7. Process sound channels (with bmson slicing) ────────────────
    // For each channel:
    //   1. Gather all unique pulse values from notes, sorted.
    //   2. Convert pulses to metric seconds for slicing.
    //   3. Track restart points (c=false) to compute audio-file offsets.
    //   4. Assign a unique sound ID per slice.
    //   5. Map each note to its slice's sound ID.
    const auto soundChannels = bmson["sound_channels"].toArray();
    uint16_t nextSoundId = 1; // 0 is reserved for "no sound"

    for (uint16_t channelIdx = 0;
         channelIdx < static_cast<uint16_t>(soundChannels.size());
         ++channelIdx) {
        auto channelObj = soundChannels[channelIdx].toObject();
        const auto notesArr = channelObj["notes"].toArray();
        if (notesArr.isEmpty()) {
            continue;
        }

        // ── 7a. Parse all notes in this channel ─────────────────────────
        struct ChannelNote
        {
            qint64 x;
            int64_t y; // pulse
            int64_t l; // length in pulses (0 = normal)
            bool c;    // continuation flag
        };
        auto channelNotes = std::vector<ChannelNote>{};
        channelNotes.reserve(notesArr.size());
        for (const auto& noteVal : notesArr) {
            auto noteObj = noteVal.toObject();
            channelNotes.push_back({
              noteObj["x"].toInteger(0),
              static_cast<int64_t>(noteObj["y"].toDouble(0)),
              static_cast<int64_t>(noteObj["l"].toDouble(0)),
              noteObj["c"].toBool(false),
            });
        }

        // ── 7b. Gather unique pulse positions, sorted ───────────────────
        auto uniquePulses = std::vector<int64_t>{};
        for (const auto& note : channelNotes) {
            uniquePulses.push_back(note.y);
        }
        std::ranges::sort(uniquePulses);
        uniquePulses.erase(std::ranges::unique(uniquePulses).begin(),
                           uniquePulses.end());

        // ── 7c. For each unique pulse, determine if it restarts audio ───
        // A pulse restarts if ANY note at that pulse has c=false.
        auto pulseRestarts = std::unordered_map<int64_t, bool>{};
        for (const auto& pulse : uniquePulses) {
            pulseRestarts[pulse] = false;
        }
        for (const auto& note : channelNotes) {
            if (!note.c) {
                pulseRestarts[note.y] = true;
            }
        }

        // ── 7d. Convert pulses to seconds and compute slices ────────────
        // Walk through unique pulses in order. Track the "audio cursor"
        // which resets to 0 on restart and advances by the time delta
        // between consecutive pulses otherwise.
        auto pulseToSeconds = [&pulseToTime](int64_t pulse) -> double {
            auto time = pulseToTime(pulse);
            return std::chrono::duration<double>(time.timestamp).count();
        };

        struct SliceDesc
        {
            uint16_t soundId;
            double audioStart; // in seconds within the audio file
            double audioEnd;   // -1 = end of file
        };
        auto slices = std::vector<SliceDesc>{};
        // Map from pulse → soundId for this channel
        auto pulseToSoundId = std::unordered_map<int64_t, uint16_t>{};

        double audioCursor = 0.0;
        for (size_t i = 0; i < uniquePulses.size(); ++i) {
            auto pulse = uniquePulses[i];
            bool restarts = pulseRestarts[pulse];

            if (restarts) {
                audioCursor = 0.0;
            }

            auto sliceSoundId = nextSoundId++;
            pulseToSoundId[pulse] = sliceSoundId;

            double sliceStart = audioCursor;
            double sliceEnd = -1.0; // end of file by default

            // Advance audio cursor to next pulse's position
            if (i + 1 < uniquePulses.size()) {
                auto nextPulse = uniquePulses[i + 1];
                auto deltaSec =
                  pulseToSeconds(nextPulse) - pulseToSeconds(pulse);
                // If next pulse restarts, this slice extends to the delta
                // (audio cursor will reset anyway)
                sliceEnd = audioCursor + deltaSec;
                if (!pulseRestarts[nextPulse]) {
                    audioCursor += deltaSec;
                }
            }

            slices.push_back({ sliceSoundId, sliceStart, sliceEnd });
            data.bmsonSlices.push_back({
              sliceSoundId,
              channelIdx,
              sliceStart,
              sliceEnd,
            });
        }

        // ── 7e. Create note data using per-slice sound IDs ──────────────
        for (const auto& note : channelNotes) {
            auto time = pulseToTime(note.y);
            auto snap = pulseToSnap(note.y);

            auto sliceSoundId = pulseToSoundId[note.y];

            auto column = bmsonXToColumn(note.x);
            if (!column.has_value()) {
                // BGM note
                data.bgmNotes.emplace_back(time, sliceSoundId);
                continue;
            }

            auto col = column.value();
            if (col < 0 || col >= static_cast<int>(columnNumber)) {
                spdlog::debug(
                  "bmson lane x={} mapped to invalid column {}", note.x, col);
                continue;
            }

            if (note.l > 0) {
                // Long note
                data.notes.at(col).emplace_back(
                  Note{ time, snap, NoteType::LongNoteBegin, sliceSoundId });
                auto endTime = pulseToTime(note.y + note.l);
                auto endSnap = pulseToSnap(note.y + note.l);
                data.notes.at(col).emplace_back(Note{
                  endTime, endSnap, NoteType::LongNoteEnd, sliceSoundId });
            } else {
                // Normal note
                data.notes.at(col).emplace_back(
                  Note{ time, snap, NoteType::Normal, sliceSoundId });
            }
        }
    }

    // ── 7f. Fuse notes from different channels at the same position ─────
    // For visible notes: group by (column, timestamp, noteType). If
    // multiple notes land on the same spot, keep one note and record a
    // fusion entry mapping a new fused sound ID to all the individual
    // slice sound IDs.
    for (auto& column : data.notes) {
        // Sort first so duplicates are adjacent
        std::ranges::sort(column, [](const auto& a, const auto& b) {
            if (a.time.timestamp != b.time.timestamp) {
                return a.time.timestamp < b.time.timestamp;
            }
            return a.noteType < b.noteType;
        });
        auto fusedColumn = std::vector<Note>{};
        fusedColumn.reserve(column.size());
        for (size_t i = 0; i < column.size();) {
            // Find the run of notes at the same (timestamp, noteType)
            size_t j = i + 1;
            while (j < column.size() &&
                   column[j].time.timestamp == column[i].time.timestamp &&
                   column[j].noteType == column[i].noteType) {
                ++j;
            }
            if (j - i == 1) {
                // Single note, no fusion needed
                fusedColumn.push_back(column[i]);
            } else {
                // Multiple notes at the same position — fuse them
                auto fusedSoundId = nextSoundId++;
                auto sliceIds = std::vector<uint64_t>{};
                for (size_t k = i; k < j; ++k) {
                    sliceIds.push_back(column[k].sound);
                }
                data.bmsonFusions[fusedSoundId] = std::move(sliceIds);
                auto fusedNote = column[i];
                fusedNote.sound = fusedSoundId;
                fusedColumn.push_back(fusedNote);
            }
            i = j;
        }
        column = std::move(fusedColumn);
    }
    // Do the same for BGM notes
    {
        std::ranges::sort(data.bgmNotes);
        auto fusedBgm = std::vector<std::pair<Time, uint64_t>>{};
        fusedBgm.reserve(data.bgmNotes.size());
        for (size_t i = 0; i < data.bgmNotes.size();) {
            size_t j = i + 1;
            while (j < data.bgmNotes.size() &&
                   data.bgmNotes[j].first.timestamp ==
                     data.bgmNotes[i].first.timestamp) {
                ++j;
            }
            if (j - i == 1) {
                fusedBgm.push_back(data.bgmNotes[i]);
            } else {
                auto fusedSoundId = nextSoundId++;
                auto sliceIds = std::vector<uint64_t>{};
                for (size_t k = i; k < j; ++k) {
                    sliceIds.push_back(data.bgmNotes[k].second);
                }
                data.bmsonFusions[fusedSoundId] = std::move(sliceIds);
                fusedBgm.emplace_back(data.bgmNotes[i].first, fusedSoundId);
            }
            i = j;
        }
        data.bgmNotes = std::move(fusedBgm);
    }

    // ── 8. Bar lines ────────────────────────────────────────────────────
    for (const auto& line : bmson["lines"].toArray()) {
        auto obj = line.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        data.barLines.push_back(pulseToTime(pulse));
    }

    // ── 9. BGA events ───────────────────────────────────────────────────
    const auto bgaObj = bmson["bga"].toObject();

    for (const auto& ev : bgaObj["bga_events"].toArray()) {
        auto obj = ev.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        auto id = static_cast<uint16_t>(obj["id"].toInt(0));
        data.bgaBase.emplace_back(pulseToTime(pulse), id);
    }
    for (const auto& ev : bgaObj["layer_events"].toArray()) {
        auto obj = ev.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        auto id = static_cast<uint16_t>(obj["id"].toInt(0));
        data.bgaLayer.emplace_back(pulseToTime(pulse), id);
    }
    for (const auto& ev : bgaObj["poor_events"].toArray()) {
        auto obj = ev.toObject();
        auto pulse = static_cast<int64_t>(obj["y"].toDouble(0));
        auto id = static_cast<uint16_t>(obj["id"].toInt(0));
        data.bgaPoor.emplace_back(pulseToTime(pulse), id);
    }

    // ── 10. Sort all output arrays ──────────────────────────────────────
    for (auto& column : data.notes) {
        std::ranges::sort(column, [](const auto& a, const auto& b) {
            if (a.time.timestamp == b.time.timestamp) {
                return a.noteType < b.noteType;
            }
            return a.time.timestamp < b.time.timestamp;
        });
    }
    std::ranges::sort(data.bgmNotes);
    std::ranges::sort(data.bgaBase);
    std::ranges::sort(data.bgaLayer);
    std::ranges::sort(data.bgaPoor);
    std::ranges::sort(data.barLines);

    removeInvalidNotes(data.notes);

    return data;
}
void
BmsNotesData::generateMeasures(
  double baseBpm,
  const std::unordered_map<uint16_t, double>& bpms,
  const std::unordered_map<uint16_t, double>& stops,
  const std::unordered_map<uint16_t, double>& scrolls,
  const std::unordered_map<uint16_t, double>& speeds,
  const std::map<int64_t, ParsedBmsChart::Measure>& measures,
  LnType lnType,
  std::optional<uint16_t> lnObj)
{
    auto lastBpm = baseBpm;
    auto lastScroll = 1.0;
    auto lastMeasure = int64_t{ -1 };
    auto measureStart = Time{ 0ns, 0.0, 0.0 };
    bpmChanges.emplace_back(baseBpm, defaultScroll, measureStart);
    auto insideLnP1 = std::array<bool, ParsedBmsChart::Measure::columnNumber>{};
    auto insideLnP2 = std::array<bool, ParsedBmsChart::Measure::columnNumber>{};
    auto lastInsertedRdmNoteP1 =
      std::array<std::optional<size_t>,
                 ParsedBmsChart::Measure::columnNumber>{};
    auto lastInsertedRdmNoteP2 =
      std::array<std::optional<size_t>,
                 ParsedBmsChart::Measure::columnNumber>{};
    auto visibleNotes = std::array<std::vector<Note>, columnNumber>{};
    auto invisibleNotes = std::array<std::vector<Note>, columnNumber>{};
    auto lnNotes = std::array<std::vector<Note>, columnNumber>{};
    auto landmineNotes = std::array<std::vector<Note>, columnNumber>{};
    for (const auto& [measureIndex, measure] : measures) {
        auto currentMeasure = measureIndex;
        if (lnType == LnType::MGQ && currentMeasure > lastMeasure + 1) {
            adjustMgqLnEnds(lastBpm,
                            lastScroll,
                            measureStart,
                            insideLnP1,
                            insideLnP2,
                            lnNotes);
        }
        fillEmptyMeasures(
          lastMeasure, currentMeasure, measureStart, lastBpm, lastScroll);
        auto bpmChangesInMeasure =
          std::map<std::pair<double, BpmChangeType>, BpmChangeValues>{
              { { 0.0, BpmChangeType::NormalOrScroll },
                { lastBpm, lastScroll, measureStart } }
          };
        auto lastTimestamp = measureStart;
        auto lastFraction = 0.0;
        auto combinedBpmChanges = combineBpmChanges(measure.exBpmChanges,
                                                    measure.bpmChanges,
                                                    measure.stops,
                                                    measure.scrolls,
                                                    bpms,
                                                    stops,
                                                    scrolls);
        auto meter =
          measure.meter.value_or(ParsedBmsChart::Measure::defaultMeter);
        for (const auto& bpmChange : combinedBpmChanges) {
            auto fraction = bpmChange.fraction;
            auto timestamp =
              lastTimestamp +
              Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                      (fraction - lastFraction) * defaultBeatsPerMeasure *
                      meter * 60 * 1'000'000'000 / lastBpm)),
                    (fraction - lastFraction) * defaultBeatsPerMeasure * meter,
                    (fraction - lastFraction) * defaultBeatsPerMeasure * meter *
                      lastScroll };
            auto changeType = [&] {
                switch (bpmChange.type) {
                    case BpmChangeDef::Type::BpmChange:
                    case BpmChangeDef::Type::Scroll:
                        return BpmChangeType::NormalOrScroll;
                    case BpmChangeDef::Type::Stop:
                        return BpmChangeType::Stop;
                }
                throw std::logic_error{ "Unknown BPM change type" };
            }();
            if (bpmChange.type == BpmChangeDef::Type::BpmChange) {
                lastBpm = bpmChange.value;
            } else if (bpmChange.type == BpmChangeDef::Type::Scroll) {
                lastScroll = bpmChange.value;
            }
            bpmChanges.emplace_back(
              bpmChange.type == BpmChangeDef::Type::Stop ? 0.0 : lastBpm,
              lastScroll,
              timestamp);
            bpmChangesInMeasure[{ fraction, changeType }] = BpmChangeValues{
                bpmChange.type == BpmChangeDef::Type::Stop ? 0.0 : lastBpm,
                lastScroll,
                timestamp
            };
            if (bpmChange.type == BpmChangeDef::Type::Stop) {
                // add another bpm change at the end of the stop
                timestamp =
                  timestamp +
                  Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                          bpmChange.value / 192 * defaultBeatsPerMeasure * 60 *
                          1'000'000'000 / lastBpm)),
                        0,
                        0 };
                bpmChanges.emplace_back(lastBpm, lastScroll, timestamp);
                bpmChangesInMeasure[{ fraction, BpmChangeType::AfterStop }] =
                  BpmChangeValues{ lastBpm, lastScroll, timestamp };
            }
            lastTimestamp = timestamp;
            lastFraction = fraction;
        }
        // add last bpm change
        auto timestamp = lastTimestamp + Time{
            std::chrono::nanoseconds(static_cast<int64_t>(
              (1.0 - lastFraction) * defaultBeatsPerMeasure * meter * 60 *
              1'000'000'000 / lastBpm)),
            (1.0 - lastFraction) * defaultBeatsPerMeasure * meter,
            (1.0 - lastFraction) * defaultBeatsPerMeasure * meter * lastScroll
        };
        barLines.emplace_back(timestamp);
        for (auto i = 0; i < columnMapping.size(); i++) {
            calculateOffsetsForColumn(
              measure.p1VisibleNotes.at(columnMapping.at(i)),
              visibleNotes.at(i),
              bpmChangesInMeasure,
              meter,
              NoteType::Normal,
              lnObj);
            calculateOffsetsForColumn(
              measure.p1InvisibleNotes.at(columnMapping.at(i)),
              invisibleNotes.at(i),
              bpmChangesInMeasure,
              meter,
              NoteType::Invisible,
              std::nullopt);
            calculateOffsetsForColumn(
              measure.p2VisibleNotes.at(columnMapping.at(i)),
              visibleNotes.at(i + columnMapping.size()),
              bpmChangesInMeasure,
              meter,
              NoteType::Normal,
              lnObj);
            calculateOffsetsForColumn(
              measure.p2InvisibleNotes.at(columnMapping.at(i)),
              invisibleNotes.at(i + columnMapping.size()),
              bpmChangesInMeasure,
              meter,
              NoteType::Invisible,
              std::nullopt);
            if (lnType == LnType::RDM) {
                calculateOffsetsForLnRdm(
                  measure.p1LongNotes.at(columnMapping.at(i)),
                  lnNotes.at(i),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP1.at(columnMapping.at(i)),
                  lastInsertedRdmNoteP1.at(columnMapping.at(i)));
                calculateOffsetsForLnRdm(
                  measure.p2LongNotes.at(columnMapping.at(i)),
                  lnNotes.at(i + columnMapping.size()),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP2.at(columnMapping.at(i)),
                  lastInsertedRdmNoteP2.at(columnMapping.at(i)));
            } else if (lnType == LnType::MGQ) {
                calculateOffsetsForLnMgq(
                  measure.p1LongNotes.at(columnMapping.at(i)),
                  lnNotes.at(i),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP1.at(columnMapping.at(i)));
                calculateOffsetsForLnMgq(
                  measure.p2LongNotes.at(columnMapping.at(i)),
                  lnNotes.at(i + columnMapping.size()),
                  bpmChangesInMeasure,
                  meter,
                  insideLnP2.at(columnMapping.at(i)));
            }
            calculateOffsetsForLandmine(
              measure.p1Landmines.at(columnMapping.at(i)),
              landmineNotes.at(i),
              bpmChangesInMeasure,
              meter);
            calculateOffsetsForLandmine(
              measure.p2Landmines.at(columnMapping.at(i)),
              landmineNotes.at(i + columnMapping.size()),
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
    std::ranges::sort(bgmNotes);
    if (lnType == LnType::RDM) {
        adjustRdmLnEnds(lastInsertedRdmNoteP1, lastInsertedRdmNoteP2, lnNotes);
    } else {
        adjustMgqLnEnds(
          lastBpm, lastScroll, measureStart, insideLnP1, insideLnP2, lnNotes);
    }
    notes = invisibleNotes;
    for (auto i = 0; i < visibleNotes.size(); i++) {
        for (const auto& note : visibleNotes.at(i)) {
            notes.at(i).push_back(note);
        }
        for (const auto& note : lnNotes.at(i)) {
            notes.at(i).push_back(note);
        }
        for (const auto& note : landmineNotes.at(i)) {
            notes.at(i).push_back(note);
        }
    }
    for (auto& column : notes) {
        std::ranges::sort(column, [](const auto& a, const auto& b) {
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
  double lastScroll,
  Time measureStart,
  std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP1,
  std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP2,
  std::span<std::vector<Note>> target)

{
    auto bpmChangesInMeasureTemp =
      std::map<std::pair<double, BpmChangeType>, BpmChangeValues>{
          { { 0.0, BpmChangeType::NormalOrScroll },
            { lastBpm, lastScroll, measureStart } }
      };
    for (auto i = 0; i < columnMapping.size(); i++) {
        addLnEndsMgq(target[i],
                     bpmChangesInMeasureTemp,
                     1,
                     insideLnP1.at(columnMapping.at(i)));
        addLnEndsMgq(target[i + columnMapping.size()],
                     bpmChangesInMeasureTemp,
                     1,
                     insideLnP2.at(columnMapping.at(i)));
    }
}
void
BmsNotesData::adjustRdmLnEnds(
  const std::array<std::optional<size_t>,
                   ParsedBmsChart::Measure::columnNumber>&
    lastInsertedRdmNoteP1,
  const std::array<std::optional<size_t>,
                   ParsedBmsChart::Measure::columnNumber>&
    lastInsertedRdmNoteP2,
  std::span<std::vector<Note>> notes)
{
    for (int i = 0; i < columnMapping.size(); i++) {
        auto lastNote = lastInsertedRdmNoteP1.at(columnMapping.at(i));
        if (!lastNote.has_value()) {
            continue;
        }
        if (notes[i].at(*lastNote).noteType == NoteType::LongNoteBegin) {
            notes[i].at(*lastNote).noteType = NoteType::Normal;
        }
        lastNote = lastInsertedRdmNoteP2.at(columnMapping.at(i));
        if (!lastNote.has_value()) {
            continue;
        }
        if (notes[i + columnMapping.size()].at(*lastNote).noteType ==
            NoteType::LongNoteBegin) {
            notes[i + columnMapping.size()].at(*lastNote).noteType =
              NoteType::Normal;
        }
    }
}

void
BmsNotesData::fillEmptyMeasures(int64_t lastMeasure,
                                int64_t measureIndex,
                                Time& measureStart,
                                double lastBpm,
                                double lastScroll)
{
    lastMeasure++;
    for (; lastMeasure < measureIndex; ++lastMeasure) {
        auto measureLength =
          Time{ std::chrono::nanoseconds(static_cast<int64_t>(
                  60.0 * defaultBeatsPerMeasure * 1'000'000'000 / lastBpm)),
                defaultBeatsPerMeasure,
                defaultBeatsPerMeasure * lastScroll };
        auto measureEnd = measureStart + measureLength;
        barLines.push_back(measureEnd);
        measureStart = measureEnd;
    }
}
} // namespace charts
