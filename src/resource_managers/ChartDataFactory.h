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
    auto detectEncoding(std::string_view string) const -> std::string
    {

        auto encoding = std::string{};

        uchardet_handle_data(detector.get(), string.data(), string.size());
        uchardet_data_end(detector.get());
        encoding = uchardet_get_charset(detector.get());
        return encoding;
    }

    auto loadFile(const QUrl& chartPath) const -> std::string
    {
        auto chartFile = std::ifstream{ chartPath.toLocalFile().toStdString() };
        if (!chartFile.is_open()) {
            throw std::runtime_error{ "Failed to open chart file" };
        }

        auto chart = std::string{};
        chartFile.seekg(0, std::ios::end);
        chart.reserve(chartFile.tellg());
        chartFile.seekg(0, std::ios::beg);
        chart.assign(std::istreambuf_iterator<char>{ chartFile },
                     std::istreambuf_iterator<char>{});
        return chart;
    }
    auto makeNotes(charts::gameplay_models::BmsNotesData& calculatedNotesData)
      const -> gameplay_logic::BmsNotes*
    {
        auto visibleNotes = QList<QList<int64_t>>{};
        for (const auto& column : calculatedNotesData.visibleNotes) {
            auto columnNotes = QList<int64_t>{};
            for (const auto& note : column) {
                columnNotes.append(
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                    note.first)
                    .count());
            }
            visibleNotes.append(std::move(columnNotes));
        }
        auto invisibleNotes = QList<QList<int64_t>>{};
        for (const auto& column : calculatedNotesData.invisibleNotes) {
            auto columnNotes = QList<int64_t>{};
            for (const auto& note : column) {
                columnNotes.append(
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                    note.first)
                    .count());
            }
            invisibleNotes.append(std::move(columnNotes));
        }
        auto* notes = new gameplay_logic::BmsNotes{ std::move(visibleNotes),
                                                    std::move(invisibleNotes) };
        return notes;
    }

  public:
    struct ChartComponents
    {
        gameplay_logic::ChartData* chartData;
        charts::gameplay_models::BmsNotesData notesData;
        std::map<std::string, std::string> wavs;
    };

    auto loadChartData(const QUrl& chartPath) const -> ChartComponents
    {
        auto chart = loadFile(chartPath);
        auto hash = support::sha256(chart);
        auto encodingName = detectEncoding(chart);
        if (encodingName.empty()) {
            throw std::runtime_error{ "Failed to detect encoding" };
        }
        auto chartUtf = boost::locale::conv::to_utf<char>(chart, encodingName);
        auto parsedChart = chartReader.readBmsChart(chartUtf);
        auto calculatedNotesData =
          charts::gameplay_models::BmsNotesData{ parsedChart };
        auto noteCount = 0;
        for (const auto& column : calculatedNotesData.visibleNotes) {
            noteCount += column.size();
        }
        auto* noteData = makeNotes(calculatedNotesData);
        // todo: length
        auto* chartData = new gameplay_logic::ChartData{
            QString::fromStdString(parsedChart.tags.title.value_or("")),
            QString::fromStdString(parsedChart.tags.artist.value_or("")),
            QString::fromStdString(parsedChart.tags.genre.value_or("")),
            noteCount,
            120'000,
            QFileInfo{ chartPath.toLocalFile() }.absolutePath(),
            noteData
        };
        return { chartData,
                 std::move(calculatedNotesData),
                 parsedChart.tags.wavs };
    }
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTDATAFACTORY_H
