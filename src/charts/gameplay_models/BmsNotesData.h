//
// Created by bobini on 15.06.23.
//

#ifndef RHYTHMGAME_BMSNOTESDATA_H
#define RHYTHMGAME_BMSNOTESDATA_H

#include <string>
#include <vector>
#include <chrono>
#include "Snap.h"
#include "sounds/OpenAlSound.h"
#include "charts/parser_models/ParsedBmsChart.h"

namespace charts::gameplay_models {
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
        Normal,
        LongNoteEnd,
        LongNoteBegin,
        Landmine
    };

    struct Note
    {
        Time time;
        std::string sound;
        Snap snap;
        NoteType noteType;
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
    std::array<std::vector<Note>, columnNumber> visibleNotes;
    std::array<std::vector<Note>, columnNumber> invisibleNotes;
    std::vector<std::pair<Time, std::string>> bgmNotes;
    std::vector<std::pair<Time, std::string>> bgaBase;
    std::vector<std::pair<Time, std::string>> bgaPoor;
    std::vector<std::pair<Time, std::string>> bgaLayer;
    std::vector<std::pair<Time, std::string>> bgaLayer2;
    std::vector<std::pair<Time, double>> bpmChanges;
    std::vector<Time> barLines;
    static constexpr auto defaultBpm = 120.0;
    static constexpr auto defaultLnType = LnType::RDM;
    explicit BmsNotesData(const parser_models::ParsedBmsChart& chart);

  private:
    void generateMeasures(
      double baseBpm,
      const std::map<std::string, double>& bpms,
      const std::map<int64_t, parser_models::ParsedBmsChart::Measure>& measures,
      LnType lnType,
      std::optional<std::string> lnObj);
    void fillEmptyMeasures(int64_t lastMeasure,
                           int64_t& measureIndex,
                           BmsNotesData::Time& measureStart,
                           double lastBpm);
};
} // namespace charts::gameplay_models
#endif // RHYTHMGAME_BMSNOTESDATA_H
