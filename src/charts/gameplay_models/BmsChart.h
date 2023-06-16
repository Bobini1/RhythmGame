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
        sounds::OpenALSound* sound;
        Snap snap;
    };
    static constexpr auto columnNumber = 16;
    std::array<std::vector<std::pair<std::chrono::nanoseconds, Note>>,
               columnNumber>
      visibleNotes;
    std::array<std::vector<std::pair<std::chrono::nanoseconds, Note>>,
               columnNumber>
      invisibleNotes;
    std::vector<std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>>
      bgmNotes;
    std::vector<std::pair<std::chrono::nanoseconds, double>> bpmChanges;
    std::vector<std::chrono::nanoseconds> barLines;
    static constexpr auto defaultBpm = 120.0;
    BmsChart(const parser_models::ParsedBmsChart& chart,
             const std::string& path);

  private:
    std::unordered_map<std::string, sounds::OpenALSound> sounds;
    void loadSounds(const std::map<std::string, std::string>& wavs,
                    const std::string& path);
    void generateMeasures(
      double baseBpm,
      const std::map<std::string, double>& bpms,
      const std::map<uint64_t, parser_models::ParsedBmsChart::Measure>&
        measures);
    void fillEmptyMeasures(uint64_t lastMeasure,
                           uint64_t& measureIndex,
                           std::chrono::nanoseconds measureStart,
                           double lastBpm);
    void calculateOffsetsForColumn(
      const std::vector<std::string>& notes,
      std::vector<std::pair<std::chrono::nanoseconds, Note>>& target,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure,
      double meter);
    void calculateOffsetsForBgm(
      const std::vector<std::string>& notes,
      std::vector<std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>>&
        target,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure);
    auto createNoteInfo(
      const std::vector<std::string>& notes,
      const std::map<double, std::pair<double, std::chrono::nanoseconds>>&
        bpmChangesInMeasure,
      int index,
      const std::string& note)
      -> std::tuple<std::chrono::nanoseconds, sounds::OpenALSound*, double>;
};
} // namespace charts::gameplay_models
#endif // RHYTHMGAME_BMSCHART_H
