//
// Created by bobini on 15.06.23.
//

#ifndef RHYTHMGAME_BMSNOTESDATA_H
#define RHYTHMGAME_BMSNOTESDATA_H

#include <vector>
#include <chrono>
#include "Snap.h"
#include "ParsedBmsChart.h"

#include <span>

namespace charts {
struct BmsNotesData
{
    using Position = double;
    struct Time
    {
        std::chrono::nanoseconds timestamp;
        Position position;

        auto operator+(const Time& other) const -> Time
        {
            return { timestamp + other.timestamp, position + other.position };
        }
        auto operator-=(const Time& other) -> Time&
        {
            timestamp -= other.timestamp;
            position -= other.position;
            return *this;
        }

        auto operator<=>(const Time& other) const = default;
    };

    enum class NoteType
    {
        LongNoteBegin,
        Normal,
        Landmine,
        Invisible,
        LongNoteEnd
    };

    struct Note
    {
        Time time;
        Snap snap;
        NoteType noteType;
        uint16_t sound;
    };

    enum class LnType
    {
        RDM = 1,
        MGQ = 2
    };

    static constexpr auto defaultBeatsPerMeasure = 4;
    static constexpr auto columnMapping =
      std::array{ 0, 1, 2, 3, 4, 7, 8, 5 }; // ignore "foot pedal"
    static constexpr auto columnNumber = columnMapping.size() * 2;
    std::array<std::vector<Note>, columnNumber> notes;
    std::vector<std::pair<Time, uint16_t>> bgmNotes;
    std::vector<std::pair<Time, uint16_t>> bgaBase;
    std::vector<std::pair<Time, uint16_t>> bgaPoor;
    std::vector<std::pair<Time, uint16_t>> bgaLayer;
    std::vector<std::pair<Time, uint16_t>> bgaLayer2;
    std::vector<std::pair<Time, double>> bpmChanges;
    std::vector<Time> barLines;
    static constexpr auto defaultBpm = 120.0;
    static constexpr auto defaultLnType = LnType::RDM;
    explicit BmsNotesData(const ParsedBmsChart& chart);

  private:
    void generateMeasures(
      double baseBpm,
      const std::unordered_map<uint16_t, double>& bpms,
      const std::unordered_map<uint16_t, double>& stops,
      const std::map<int64_t, ParsedBmsChart::Measure>& measures,
      LnType lnType,
      std::optional<uint16_t> lnObj);
    void fillEmptyMeasures(int64_t lastMeasure,
                           int64_t measureIndex,
                           Time& measureStart,
                           double lastBpm);
    void adjustRdmLnEnds(
      const std::array<std::optional<size_t>,
                       ParsedBmsChart::Measure::columnNumber>&
        lastInsertedRdmNoteP1,
      const std::array<std::optional<size_t>,
                       ParsedBmsChart::Measure::columnNumber>&
        lastInsertedRdmNoteP2);
    void adjustMgqLnEnds(
      double lastBpm,
      Time measureStart,
      std::array<bool, ParsedBmsChart::Measure::columnNumber>&
        insideLnP1,
      std::array<bool, ParsedBmsChart::Measure::columnNumber>&
        insideLnP2,
        std::span<std::vector<Note>> target);
};
} // namespace charts
#endif // RHYTHMGAME_BMSNOTESDATA_H
