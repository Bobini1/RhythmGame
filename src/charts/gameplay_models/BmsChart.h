//
// Created by bobini on 15.06.23.
//

#ifndef RHYTHMGAME_BMSCHART_H
#define RHYTHMGAME_BMSCHART_H

#include <string>
#include <vector>
#include <chrono>
#include "Snap.h"
#include "sounds/OpenAlSound.h"
#include "charts/parser_models/ParsedBmsChart.h"

namespace charts::gameplay_models {
struct BmsChart
{
    struct Note
    {
        std::string sound;
        Snap snap;
    };
    static constexpr auto columnMapping =
      std::array{ 0, 1, 2, 3, 4, 7, 8, 5 }; // ignore "foot pedal"
    static constexpr auto columnNumber = columnMapping.size() * 2;
    std::array<std::vector<std::pair<std::chrono::nanoseconds, Note>>,
               columnNumber>
      visibleNotes;
    std::array<std::vector<std::pair<std::chrono::nanoseconds, Note>>,
               columnNumber>
      invisibleNotes;
    std::vector<std::pair<std::chrono::nanoseconds, std::string>> bgmNotes;
    std::vector<std::pair<std::chrono::nanoseconds, double>> bpmChanges;
    std::vector<std::chrono::nanoseconds> barLines;
    static constexpr auto defaultBpm = 120.0;
    explicit BmsChart(const parser_models::ParsedBmsChart& chart);

  private:
    void generateMeasures(
      double baseBpm,
      const std::map<std::string, double>& bpms,
      const std::map<uint64_t, parser_models::ParsedBmsChart::Measure>&
        measures);
    void fillEmptyMeasures(uint64_t lastMeasure,
                           uint64_t& measureIndex,
                           std::chrono::nanoseconds& measureStart,
                           double lastBpm);
    void calculateOffsetsForColumn(
      const std::vector<std::string>& notes,
      std::vector<std::pair<std::chrono::nanoseconds, Note>>& target,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure,
      double meter);
    void calculateOffsetsForBgm(
      const std::vector<std::string>& notes,
      std::vector<std::pair<std::chrono::nanoseconds, std::string>>& target,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure,
      double meter);
    auto createNoteInfo(
      const std::vector<std::string>& notes,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure,
      int index,
      const std::string& note,
      double meter)
      -> std::tuple<std::chrono::nanoseconds, std::string, double>;
};
} // namespace charts::gameplay_models
#endif // RHYTHMGAME_BMSCHART_H
