//
// Created by bobini on 23.08.23.
//

#ifndef RHYTHMGAME_CHARTDATAFACTORY_H
#define RHYTHMGAME_CHARTDATAFACTORY_H

#include "gameplay_logic/ChartData.h"
#include "charts/BmsNotesData.h"
#include "gameplay_logic/BmsNotes.h"
namespace resource_managers {

class ChartDataFactory
{
    static auto loadFile(const QUrl& chartPath) -> std::string;
    static auto convertToQVector(
      const std::vector<charts::BmsNotesData::Note>& column)
      -> QVector<gameplay_logic::Note>;

  public:
    using RandomGenerator =
      std::function<charts::ParsedBmsChart::RandomRange(
        charts::ParsedBmsChart::RandomRange)>;
    struct ChartComponents
    {
        std::unique_ptr<gameplay_logic::ChartData> chartData;
        charts::BmsNotesData notesData;
        std::unordered_map<uint16_t, std::filesystem::path> wavs;
        std::unordered_map<uint16_t, std::filesystem::path> bmps;

        ChartComponents(std::unique_ptr<gameplay_logic::ChartData> chartData,
            charts::BmsNotesData notesData,
            std::unordered_map<uint16_t, std::filesystem::path> wavs,
            std::unordered_map<uint16_t, std::filesystem::path> bmps);
        ChartComponents(const ChartComponents& other);
        ChartComponents(ChartComponents&& other) noexcept;
        auto operator=(const ChartComponents& other) -> ChartComponents&;
        auto operator=(ChartComponents&& other) noexcept -> ChartComponents&;
    };
    static auto makeNotes(
      const std::array<std::vector<charts::BmsNotesData::Note>,
                       charts::BmsNotesData::columnNumber>&
        notes,
      const std::vector<std::pair<charts::BmsNotesData::Time,
                                  double>>& bpmChanges,
      const std::vector<charts::BmsNotesData::Time>& barLines)
      -> std::unique_ptr<gameplay_logic::BmsNotes>;
    auto loadChartData(
      const std::filesystem::path& chartPath,
      RandomGenerator randomGenerator,
      int64_t directory = 0) const -> ChartComponents;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTDATAFACTORY_H
