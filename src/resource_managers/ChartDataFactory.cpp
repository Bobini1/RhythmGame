//
// Created by bobini on 23.08.23.
//

#include "ChartDataFactory.h"

#include <utility>

namespace resource_managers {
auto
ChartDataFactory::loadFile(const QUrl& chartPath) -> std::string
{
#if defined(_WIN32)
    auto fileUtf = chartPath.toLocalFile().toStdWString();
#else
    auto fileUtf = chartPath.toLocalFile().toStdString();
#endif
    auto chartFile = std::ifstream{ fileUtf };
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
  -> std::unique_ptr<gameplay_logic::BmsNotes>
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
    return std::make_unique<gameplay_logic::BmsNotes>(std::move(visibleNotes),
                                                      std::move(invisibleNotes),
                                                      std::move(bpmChanges),
                                                      std::move(barLines));
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
ChartDataFactory::loadChartData(
  const QString& chartPath,
  std::function<charts::parser_models::ParsedBmsChart::RandomRange(
    charts::parser_models::ParsedBmsChart::RandomRange)> randomGenerator,
  QString directoryInDb) const -> ChartDataFactory::ChartComponents
{
    auto url = QUrl::fromLocalFile(chartPath);
    auto chart = loadFile(url);
    auto hash = support::sha256(chart);
    auto parsedChart = chartReader.readBmsChart(chart, randomGenerator);
    auto calculatedNotesData =
      charts::gameplay_models::BmsNotesData{ parsedChart };
    auto noteCount = 0;
    for (const auto& column : calculatedNotesData.visibleNotes) {
        noteCount += column.size();
    }
    auto noteData = makeNotes(calculatedNotesData);

    auto lastNoteTimestamp = std::chrono::nanoseconds{ 0 };
    for (const auto& column : calculatedNotesData.visibleNotes) {
        if (column.empty()) {
            continue;
        }
        auto lastNote = column.back();
        if (lastNote.time.timestamp > lastNoteTimestamp) {
            lastNoteTimestamp = lastNote.time.timestamp;
        }
    }
    auto endTimestamp = lastNoteTimestamp + std::chrono::seconds{ 3 };
    // find keymode
    auto keymode = gameplay_logic::ChartData::Keymode::K7;
    const auto startColumn = calculatedNotesData.visibleNotes.size() / 2;
    for (auto columnIndex = startColumn;
         columnIndex < calculatedNotesData.visibleNotes.size();
         columnIndex++) {
        if (!calculatedNotesData.visibleNotes[columnIndex].empty()) {
            keymode = gameplay_logic::ChartData::Keymode::K14;
            break;
        }
    }
    // get initial bpm
    auto initialBpm = calculatedNotesData.bpmChanges[0]; // guaranteed to exist
    // get max bpm
    auto maxBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.second > maxBpm.second) {
            maxBpm = bpmChange;
        }
    }
    // get min bpm
    auto minBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.second < minBpm.second) {
            minBpm = bpmChange;
        }
    }
    auto chartData = std::make_unique<gameplay_logic::ChartData>(
      QString::fromStdString(parsedChart.tags.title.value_or("")),
      QString::fromStdString(parsedChart.tags.artist.value_or("")),
      QString::fromStdString(parsedChart.tags.subTitle.value_or("")),
      QString::fromStdString(parsedChart.tags.subArtist.value_or("")),
      QString::fromStdString(parsedChart.tags.genre.value_or("")),
      QString::fromStdString(parsedChart.tags.stageFile.value_or("")),
      QString::fromStdString(parsedChart.tags.banner.value_or("")),
      QString::fromStdString(parsedChart.tags.backBmp.value_or("")),
      parsedChart.tags.rank.value_or(2),
      parsedChart.tags.total.value_or(160.0),
      parsedChart.tags.playLevel.value_or(1),
      parsedChart.tags.difficulty.value_or(1),
      parsedChart.tags.isRandom,
      noteCount,
      endTimestamp.count(),
      initialBpm.second,
      maxBpm.second,
      minBpm.second,
      QFileInfo{ chartPath }.absoluteFilePath(),
      std::move(directoryInDb),
      QString::fromStdString(hash),
      keymode);
    return { std::move(chartData),
             std::move(noteData),
             std::move(calculatedNotesData),
             parsedChart.tags.wavs };
}
} // namespace resource_managers