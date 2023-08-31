//
// Created by bobini on 23.08.23.
//

#include "ChartDataFactory.h"

namespace resource_managers {
auto
ChartDataFactory::detectEncoding(std::string_view string) const -> std::string
{

    auto encoding = std::string{};

    uchardet_handle_data(detector.get(), string.data(), string.size());
    uchardet_data_end(detector.get());
    encoding = uchardet_get_charset(detector.get());
    return encoding;
}
auto
ChartDataFactory::loadFile(const QUrl& chartPath) -> std::string
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
auto
ChartDataFactory::makeNotes(
  charts::gameplay_models::BmsNotesData& calculatedNotesData)
  -> gameplay_logic::BmsNotes*
{
    auto visibleNotes = QVector<QVector<gameplay_logic::Note>>{};
    for (const auto& column : calculatedNotesData.visibleNotes) {
        visibleNotes.append(convertToQVector(column));
    }
    auto invisibleNotes = QVector<QVector<gameplay_logic::Note>>{};
    for (const auto& column : calculatedNotesData.invisibleNotes) {
        invisibleNotes.append(convertToQVector(column));
    }
    auto bpmChanges = QVector<gameplay_logic::BpmChange>{};
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        bpmChanges.append(
          { { std::chrono::duration_cast<std::chrono::milliseconds>(
                bpmChange.first.timestamp)
                .count(),
              bpmChange.first.position },
            bpmChange.second });
    }
    auto barLines = QVector<gameplay_logic::Time>{};
    for (const auto& barLine : calculatedNotesData.barLines) {
        barLines.append({ std::chrono::duration_cast<std::chrono::milliseconds>(
                            barLine.timestamp)
                            .count(),
                          barLine.position });
    }
    auto* notes = new gameplay_logic::BmsNotes{ std::move(visibleNotes),
                                                std::move(invisibleNotes),
                                                std::move(bpmChanges),
                                                std::move(barLines) };
    return notes;
}
auto
ChartDataFactory::convertToQVector(
  const std::vector<charts::gameplay_models::BmsNotesData::Note>& column)
  -> QVector<gameplay_logic::Note>
{
    auto columnNotes = QVector<gameplay_logic::Note>{};
    columnNotes.reserve(column.size());
    for (const auto& note : column) {
        columnNotes.append(gameplay_logic::Note{
          { std::chrono::duration_cast<std::chrono::milliseconds>(
              note.time.timestamp)
              .count(),
            note.time.position },
          { note.snap.numerator, note.snap.denominator } });
    }
    return columnNotes;
}
auto
ChartDataFactory::loadChartData(const QUrl& chartPath) const
  -> ChartDataFactory::ChartComponents
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
    return { chartData, std::move(calculatedNotesData), parsedChart.tags.wavs };
}
} // namespace resource_managers