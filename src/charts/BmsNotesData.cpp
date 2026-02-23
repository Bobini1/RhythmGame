//
// Created by bobini on 15.06.23.
//

#include <algorithm>
#include <numeric>
#include <set>
#include "BmsNotesData.h"
#include "sounds/SoundBuffer.h"

#include <ranges>
#include <spdlog/spdlog.h>
#include <QJsonArray>

using namespace std::chrono_literals;

namespace charts {
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
combineBpmChanges(std::span<const std::vector<uint16_t>> exBpmChanges,
                  std::span<const std::vector<uint16_t>> bpmChanges,
                  std::span<const std::vector<uint16_t>> stops,
                  const std::unordered_map<uint16_t, double>& bpms,
                  const std::unordered_map<uint16_t, double>& stopDefs)
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
              BpmChangeDef{ fraction, false, fractionDec, bpmValue->second });
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
            combinedBpmChanges.emplace_back(BpmChangeDef{
              fraction, false, fractionDec, static_cast<double>(bpmChange) });
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
              BpmChangeDef{ fraction, true, fractionDec, stopValue->second });
        }
    }
    std::ranges::stable_sort(combinedBpmChanges,
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

} // namespace

BmsNotesData
BmsNotesData::fromParsedChart(const ParsedBmsChart& chart)
{
    auto lnType = defaultLnType;
    if (chart.tags.lnType.has_value()) {
        lnType = static_cast<LnType>(chart.tags.lnType.value());
    }
    const auto bpm = chart.tags.bpm.value_or(defaultBpm);
    if (bpm <= 0.0) {
        throw std::runtime_error{ "Initial bpm must be positive, was: " +
                                  std::to_string(bpm) };
    }
    auto data = BmsNotesData{};
    data.generateMeasures(bpm,
                          chart.tags.exBpms,
                          chart.tags.stops,
                          chart.tags.measures,
                          lnType,
                          chart.tags.lnObj);
    return data;
}
BmsNotesData
BmsNotesData::fromBmson(const QJsonObject& bmson)
{
    auto data = BmsNotesData{};

    // ── 1. Read basic info ──────────────────────────────────────────────
    const auto info = bmson["info"].toObject();
    const auto initBpm = info["init_bpm"].toDouble(defaultBpm);
    if (initBpm <= 0.0) {
        throw std::runtime_error{ "Initial bpm must be positive, was: " +
                                  std::to_string(initBpm) };
    }
    // Resolution: pulses per quarter note (default 240)
    const auto resolution = info["resolution"].toInt(240);
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
    auto currentTime = Time{ 0ns, 0.0 };
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
              currentTime + Time{ std::chrono::nanoseconds(deltaNs), deltaPos };
            lastPulse = targetPulse;
        }
    };

    // Process timeline
    timeline.push_back({ 0, currentTime, currentBpm });
    data.bpmChanges.emplace_back(currentTime, currentBpm);

    for (auto pulse : allPulses) {
        // Advance time to this pulse
        advanceTime(pulse);

        // Apply BPM change if one exists at this pulse
        while (bpmIt != bpmEvents.end() && bpmIt->pulse == pulse) {
            currentBpm = bpmIt->bpm;
            if (pulse > 0 || bpmIt != bpmEvents.begin()) {
                // Don't double-add the initial BPM
                data.bpmChanges.emplace_back(currentTime, currentBpm);
            }
            ++bpmIt;
        }

        timeline.push_back({ pulse, currentTime, currentBpm });

        // Apply stops at this pulse
        while (stopIt != stopEvents.end() && stopIt->pulse == pulse) {
            // A stop adds time but no position change
            data.bpmChanges.emplace_back(currentTime, 0.0);
            auto stopNs = static_cast<int64_t>(
              static_cast<double>(stopIt->duration) / resolution * 60.0 *
              1'000'000'000 / currentBpm);
            currentTime =
              currentTime + Time{ std::chrono::nanoseconds(stopNs), 0.0 };
            data.bpmChanges.emplace_back(currentTime, currentBpm);
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
        return it->time + Time{ std::chrono::nanoseconds(deltaNs), deltaPos };
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
        if (x >= 1 && x <= 8) {
            // P1
            if (x == 1) {
                return 7; // scratch
            }
            return x - 2; // keys: x=2→col0, x=3→col1, ..., x=8→col6
        }
        if (x >= 9 && x <= 16) {
            // P2
            if (x == 9) {
                return 15; // P2 scratch
            }
            return (x - 10) + 8; // keys: x=10→col8, ..., x=16→col14
        }
        spdlog::debug("Unknown bmson lane x={}", x);
        return std::nullopt;
    };

    // ── 7. Process sound channels ───────────────────────────────────────
    const auto soundChannels = bmson["sound_channels"].toArray();
    uint16_t nextSoundId = 1; // 0 is reserved for "no sound"
    for (const auto& channel : soundChannels) {
        auto channelObj = channel.toObject();
        auto soundId = nextSoundId++;

        const auto notesArr = channelObj["notes"].toArray();
        for (const auto& noteVal : notesArr) {
            auto noteObj = noteVal.toObject();
            auto x = noteObj["x"].toInt(0);
            auto y = static_cast<int64_t>(noteObj["y"].toDouble(0));
            auto l = static_cast<int64_t>(noteObj["l"].toDouble(0));
            auto c = noteObj["c"].toBool(false);

            auto time = pulseToTime(y);
            auto snap = pulseToSnap(y);

            // Use sound ID 0 for continuation notes (don't restart sound)
            auto noteSoundId = c ? static_cast<uint16_t>(0) : soundId;

            auto column = bmsonXToColumn(x);
            if (!column.has_value()) {
                // BGM note
                data.bgmNotes.emplace_back(time, noteSoundId);
                continue;
            }

            auto col = column.value();
            if (col < 0 || col >= static_cast<int>(columnNumber)) {
                spdlog::debug(
                  "bmson lane x={} mapped to invalid column {}", x, col);
                continue;
            }

            if (l > 0) {
                // Long note
                data.notes.at(col).emplace_back(
                  Note{ time, snap, NoteType::LongNoteBegin, noteSoundId });
                auto endTime = pulseToTime(y + l);
                auto endSnap = pulseToSnap(y + l);
                data.notes.at(col).emplace_back(
                  Note{ endTime, endSnap, NoteType::LongNoteEnd, noteSoundId });
            } else {
                // Normal note
                data.notes.at(col).emplace_back(
                  Note{ time, snap, NoteType::Normal, noteSoundId });
            }
        }
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
  const std::map<int64_t, ParsedBmsChart::Measure>& measures,
  LnType lnType,
  std::optional<uint16_t> lnObj)
{
    auto lastBpm = baseBpm;
    auto lastMeasure = int64_t{ -1 };
    auto measureStart = Time{ 0ns, 0.0 };
    bpmChanges.emplace_back(measureStart, baseBpm);
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
            adjustMgqLnEnds(
              lastBpm, measureStart, insideLnP1, insideLnP2, lnNotes);
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
        auto meter =
          measure.meter.value_or(ParsedBmsChart::Measure::defaultMeter);
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
        adjustMgqLnEnds(lastBpm, measureStart, insideLnP1, insideLnP2, lnNotes);
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
  Time measureStart,
  std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP1,
  std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP2,
  std::span<std::vector<Note>> target)

{
    auto bpmChangesInMeasureTemp =
      std::map<std::pair<double, BpmChangeType>, std::pair<double, Time>>{
          { { 0.0, BpmChangeType::Normal }, { lastBpm, measureStart } }
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
} // namespace charts