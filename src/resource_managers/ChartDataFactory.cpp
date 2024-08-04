//
// Created by bobini on 23.08.23.
//

#include "ChartDataFactory.h"

#include <utility>
#include <fstream>
#include <qfileinfo.h>
#include <boost/locale/encoding.hpp>
#include "support/UtfStringToPath.h"

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
        auto type = gameplay_logic::Note::Type::Normal;
        switch (note.noteType) {
            case charts::gameplay_models::BmsNotesData::NoteType::Normal:
                type = gameplay_logic::Note::Type::Normal;
                break;
            case charts::gameplay_models::BmsNotesData::NoteType::LongNoteEnd:
                type = gameplay_logic::Note::Type::LongNoteEnd;
                break;
            case charts::gameplay_models::BmsNotesData::NoteType::LongNoteBegin:
                type = gameplay_logic::Note::Type::LongNoteBegin;
                break;
            case charts::gameplay_models::BmsNotesData::NoteType::Landmine:
                type = gameplay_logic::Note::Type::Landmine;
                break;
        }
        columnNotes.append(gameplay_logic::Note{
          { std::chrono::duration_cast<std::chrono::milliseconds>(
              note.time.timestamp)
              .count(),
            note.time.position },
          { note.snap.numerator, note.snap.denominator },
          type });
    }
    return columnNotes;
}
auto
ChartDataFactory::loadChartData(
  const std::filesystem::path& chartPath,
  std::function<charts::parser_models::ParsedBmsChart::RandomRange(
    charts::parser_models::ParsedBmsChart::RandomRange)> randomGenerator,
  QString directoryInDb) const -> ChartDataFactory::ChartComponents
{
    auto mfh = llfio::mapped_file({}, chartPath).value();
    auto length = mfh.maximum_extent().value();
    auto chart =
      std::string_view{ reinterpret_cast<char*>(mfh.address()), length };
    auto hash = support::sha256(chart);
    auto parsedChart =
      chartReader.readBmsChart(chart, std::move(randomGenerator));
    mfh.close().value();

    auto title = QString::fromUtf8(parsedChart.tags.title.value_or(""));
    auto artist = QString::fromUtf8(parsedChart.tags.artist.value_or(""));
    auto subtitle = QString::fromUtf8(parsedChart.tags.subTitle.value_or(""));
    auto subartist = QString::fromUtf8(parsedChart.tags.subArtist.value_or(""));
    auto genre = QString::fromUtf8(parsedChart.tags.genre.value_or(""));
    auto stageFile = QString::fromUtf8(parsedChart.tags.stageFile.value_or(""));
    auto banner = QString::fromUtf8(parsedChart.tags.banner.value_or(""));
    auto backBmp = QString::fromUtf8(parsedChart.tags.backBmp.value_or(""));
    std::unordered_map<uint16_t, std::filesystem::path> wavs;
    wavs.reserve(parsedChart.tags.wavs.size());
    for (const auto& wav : parsedChart.tags.wavs) {
        wavs.emplace(wav.first, support::utfStringToPath(wav.second));
    }
    std::unordered_map<uint16_t, std::filesystem::path> bmps;
    bmps.reserve(parsedChart.tags.bmps.size());
    for (const auto& bmp : parsedChart.tags.bmps) {
        wavs.emplace(bmp.first, support::utfStringToPath(bmp.second));
    }
    auto calculatedNotesData =
      charts::gameplay_models::BmsNotesData{ parsedChart };
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
    auto normalNotes = 0;
    auto lnNotes = 0;
    auto mineNotes = 0;
    for (const auto& column : calculatedNotesData.visibleNotes) {
        for (const auto& note : column) {
            switch (note.noteType) {
                case charts::gameplay_models::BmsNotesData::NoteType::Normal:
                    normalNotes++;
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::
                  LongNoteBegin:
                    lnNotes++;
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::Landmine:
                    mineNotes++;
                    break;
            }
        }
    }
    auto chartData = std::make_unique<gameplay_logic::ChartData>(
      std::move(title),
      std::move(subtitle),
      std::move(artist),
      std::move(subartist),
      std::move(genre),
      std::move(stageFile),
      std::move(banner),
      std::move(backBmp),
      parsedChart.tags.rank.value_or(2),
      parsedChart.tags.total.value_or(160.0),
      parsedChart.tags.playLevel.value_or(1),
      parsedChart.tags.difficulty.value_or(1),
      parsedChart.tags.isRandom,
      normalNotes,
      lnNotes,
      mineNotes,
      lastNoteTimestamp.count(),
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
             std::move(wavs),
             std::move(bmps) };
}
} // namespace resource_managers