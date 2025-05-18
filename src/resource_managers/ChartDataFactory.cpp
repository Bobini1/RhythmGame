//
// Created by bobini on 23.08.23.
//

#include "ChartDataFactory.h"

#include <utility>
#include <fstream>
#include <qfileinfo.h>
#include <boost/locale/encoding.hpp>
#include "support/UtfStringToPath.h"
#include "support/PathToQString.h"
#include <llfio.hpp>

namespace llfio = LLFIO_V2_NAMESPACE;

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
  const std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
                   charts::gameplay_models::BmsNotesData::columnNumber>& notes,
  const std::vector<
    std::pair<charts::gameplay_models::BmsNotesData::Time, double>>& bpmChanges,
  const std::vector<charts::gameplay_models::BmsNotesData::Time>& barLines)
  -> std::unique_ptr<gameplay_logic::BmsNotes>
{
    auto visibleNotesQ = QVector<QVector<gameplay_logic::Note>>{};
    for (const auto& column : notes) {
        visibleNotesQ.append(convertToQVector(column));
    }
    auto bpmChangesQ = QVector<gameplay_logic::BpmChange>{};
    for (const auto& bpmChange : bpmChanges) {
        bpmChangesQ.append({ { .timestamp = bpmChange.first.timestamp.count(),
                               .position = bpmChange.first.position },
                             bpmChange.second });
    }
    auto barLinesQ = QVector<gameplay_logic::Time>{};
    for (const auto& barLine : barLines) {
        barLinesQ.append({ barLine.timestamp.count(), barLine.position });
    }
    return std::make_unique<gameplay_logic::BmsNotes>(
      std::move(visibleNotesQ), std::move(bpmChangesQ), std::move(barLinesQ));
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
            case charts::gameplay_models::BmsNotesData::NoteType::Invisible:
                type = gameplay_logic::Note::Type::Invisible;
                break;
        }
        columnNotes.append(gameplay_logic::Note{
          { note.time.timestamp.count(), note.time.position },
          { note.snap.numerator, note.snap.denominator },
          type });
    }
    return columnNotes;
}
auto
ChartDataFactory::loadChartData(
  const std::filesystem::path& chartPath,
  RandomGenerator randomGenerator,
  int64_t directory) const -> ChartComponents
{
    auto mfh = llfio::mapped_file({}, chartPath).value();
    auto length = mfh.maximum_extent().value();
    auto chart = std::string_view{ reinterpret_cast<char*>(mfh.address()),
                                   static_cast<unsigned long>(length) };
    auto sha256 = support::sha256(chart);
    auto md5 = support::md5(chart);
    auto randomValues = QList<qint64>{};
    auto randomGeneratorRecorder =
      [&randomValues, &randomGenerator](
        const charts::parser_models::ParsedBmsChart::RandomRange number) mutable {
          const auto generated = randomGenerator(number);
          randomValues.append(generated);
          return generated;
      };
    auto parsedChart = chartReader.readBmsChart(chart, randomGeneratorRecorder);
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
        bmps.emplace(bmp.first, support::utfStringToPath(bmp.second));
    }
    auto calculatedNotesData =
      charts::gameplay_models::BmsNotesData{ parsedChart };

    auto lastNoteTimestamp = std::chrono::nanoseconds{ 0 };
    for (const auto& column : calculatedNotesData.notes) {
        if (column.empty()) {
            continue;
        }
        if (auto lastNote = column.back();
            lastNote.time.timestamp > lastNoteTimestamp) {
            lastNoteTimestamp = lastNote.time.timestamp;
        }
    }
    auto keymode = gameplay_logic::ChartData::Keymode::K7;
    constexpr auto startColumn = calculatedNotesData.notes.size() / 2;
    for (auto columnIndex = startColumn;
         columnIndex < calculatedNotesData.notes.size();
         columnIndex++) {
        if (!calculatedNotesData.notes[columnIndex].empty()) {
            keymode = gameplay_logic::ChartData::Keymode::K14;
            break;
        }
    }
    auto initialBpm = calculatedNotesData.bpmChanges[0]; // guaranteed to exist
    auto maxBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.second > maxBpm.second) {
            maxBpm = bpmChange;
        }
    }
    auto minBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.second < minBpm.second) {
            minBpm = bpmChange;
        }
    }
    auto bpms = std::unordered_map<double,
                              std::chrono::nanoseconds>{};
    for (auto it = calculatedNotesData.bpmChanges.begin();
         it != calculatedNotesData.bpmChanges.end();
         ++it) {
        auto bpmChange = *it;
        auto bpm = bpmChange.second;
        auto timestamp = bpmChange.first.timestamp;
        auto nextTimestamp = std::chrono::nanoseconds{ 0 };
        if (it + 1 != calculatedNotesData.bpmChanges.end()) {
            nextTimestamp = (it + 1)->first.timestamp;
        } else {
            nextTimestamp = lastNoteTimestamp;
        }
        auto duration = nextTimestamp - timestamp;
        bpms[bpm] += duration;
    }
    auto totalBpm = 0.0;
    auto totalDuration = std::chrono::nanoseconds{ 0 };
    auto maxBpmDuration = std::chrono::nanoseconds{ 0 };
    auto mainBpm = 0.0;
    for (const auto& [bpm, duration] : bpms) {
        totalBpm += bpm * duration.count();
        totalDuration += duration;
        if (duration > maxBpmDuration) {
            maxBpmDuration = duration;
            mainBpm = bpm;
        }
    }
    auto avgBpm = totalBpm;
    if (totalDuration.count() > 0) {
        avgBpm /= totalDuration.count();
    }
    auto normalNotes = 0;
    auto lnNotes = 0;
    auto mineNotes = 0;
    for (const auto& column : calculatedNotesData.notes) {
        for (const auto& note : column) {
            switch (note.noteType) {
                case charts::gameplay_models::BmsNotesData::NoteType::Normal:
                    normalNotes++;
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::
                  LongNoteBegin:
                    lnNotes++;
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::
                  LongNoteEnd:
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::Landmine:
                    mineNotes++;
                    break;
                case charts::gameplay_models::BmsNotesData::NoteType::Invisible:
                    break;
            }
        }
    }
    auto path = support::pathToQString(chartPath);
    auto chartData = std::make_unique<gameplay_logic::ChartData>(
      std::move(title),
      std::move(artist),
      std::move(subtitle),
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
      randomValues,
      normalNotes,
      lnNotes,
      mineNotes,
      lastNoteTimestamp.count(),
      initialBpm.second,
      maxBpm.second,
      minBpm.second,
        mainBpm,
        avgBpm,
      path,
      directory,
      QString::fromStdString(sha256),
      QString::fromStdString(md5),
      keymode);
    auto noteData = makeNotes(calculatedNotesData.notes,
                              calculatedNotesData.bpmChanges,
                              calculatedNotesData.barLines);
    return { .chartData = std::move(chartData),
             .notesData = std::move(calculatedNotesData),
             .wavs = std::move(wavs),
             .bmps = std::move(bmps) };
}
} // namespace resource_managers