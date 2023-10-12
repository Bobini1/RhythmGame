//
// Created by bobini on 23.08.23.
//

#ifndef RHYTHMGAME_CHARTDATAFACTORY_H
#define RHYTHMGAME_CHARTDATAFACTORY_H

#include "gameplay_logic/ChartData.h"
#include "support/Sha256.h"
#include "charts/chart_readers/BmsChartReader.h"
#include "charts/gameplay_models/BmsNotesData.h"
#include "gameplay_logic/BmsNotes.h"
#include <fstream>
#include <boost/locale/encoding.hpp>

namespace resource_managers {

class ChartDataFactory
{
    charts::chart_readers::BmsChartReader chartReader;

    static auto loadFile(const QUrl& chartPath) -> std::string;
    static auto makeNotes(
      charts::gameplay_models::BmsNotesData& calculatedNotesData)
      -> std::unique_ptr<gameplay_logic::BmsNotes>;
    static auto convertToQVector(
      const std::vector<charts::gameplay_models::BmsNotesData::Note>& column)
      -> QVector<gameplay_logic::Note>;

  public:
    struct ChartComponents
    {
        std::unique_ptr<gameplay_logic::ChartData> chartData;
        std::unique_ptr<gameplay_logic::BmsNotes> bmsNotes;
        charts::gameplay_models::BmsNotesData notesData;
        std::map<std::string, std::string> wavs;
        std::map<std::string, std::string> bmps;
    };

    auto loadChartData(
      const QString& chartPath,
      std::function<charts::parser_models::ParsedBmsChart::RandomRange(
        charts::parser_models::ParsedBmsChart::RandomRange)> randomGenerator,
      QString directoryInDb = QString()) const -> ChartComponents;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTDATAFACTORY_H
