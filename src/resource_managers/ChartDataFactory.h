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
#include <uchardet/uchardet.h>
#include <fstream>
#include <boost/locale/encoding.hpp>

namespace resource_managers {

class ChartDataFactory
{
    std::unique_ptr<uchardet, decltype(&uchardet_delete)> detector{
        uchardet_new(),
        &uchardet_delete
    };
    charts::chart_readers::BmsChartReader chartReader;
    auto detectEncoding(std::string_view string) const -> std::string;

    static auto loadFile(const QUrl& chartPath) -> std::string;
    static auto makeNotes(
      charts::gameplay_models::BmsNotesData& calculatedNotesData)
      -> gameplay_logic::BmsNotes*;
    static auto convertToQVector(
      const std::vector<charts::gameplay_models::BmsNotesData::Note>& column)
      -> QVector<gameplay_logic::Note>;

  public:
    struct ChartComponents
    {
        gameplay_logic::ChartData* chartData;
        charts::gameplay_models::BmsNotesData notesData;
        std::map<std::string, std::string> wavs;
    };

    auto loadChartData(const QUrl& chartPath) const -> ChartComponents;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTDATAFACTORY_H
