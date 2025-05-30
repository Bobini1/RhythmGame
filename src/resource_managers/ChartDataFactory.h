//
// Created by bobini on 23.08.23.
//

#ifndef RHYTHMGAME_CHARTDATAFACTORY_H
#define RHYTHMGAME_CHARTDATAFACTORY_H

#include "gameplay_logic/ChartData.h"
#include "charts/chart_readers/BmsChartReader.h"
#include "charts/gameplay_models/BmsNotesData.h"
#include "gameplay_logic/BmsNotes.h"
namespace resource_managers {

class ChartDataFactory
{
    charts::chart_readers::BmsChartReader chartReader;

    static auto loadFile(const QUrl& chartPath) -> std::string;
    static auto convertToQVector(
      const std::vector<charts::gameplay_models::BmsNotesData::Note>& column)
      -> QVector<gameplay_logic::Note>;

  public:
    using RandomGenerator =
      std::function<charts::parser_models::ParsedBmsChart::RandomRange(
        charts::parser_models::ParsedBmsChart::RandomRange)>;
    struct ChartComponents
    {
        std::unique_ptr<gameplay_logic::ChartData> chartData;
        charts::gameplay_models::BmsNotesData notesData;
        std::unordered_map<uint16_t, std::filesystem::path> wavs;
        std::unordered_map<uint16_t, std::filesystem::path> bmps;
    };
    static auto makeNotes(
      const std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
                       charts::gameplay_models::BmsNotesData::columnNumber>&
        notes,
      const std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time,
                                  double>>& bpmChanges,
      const std::vector<charts::gameplay_models::BmsNotesData::Time>& barLines)
      -> std::unique_ptr<gameplay_logic::BmsNotes>;
    auto loadChartData(
      const std::filesystem::path& chartPath,
      RandomGenerator randomGenerator,
      int64_t directory = 0) const -> ChartComponents;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTDATAFACTORY_H
