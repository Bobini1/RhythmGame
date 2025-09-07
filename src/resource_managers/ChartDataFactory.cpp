//
// Created by bobini on 23.08.23.
//

#include "ChartDataFactory.h"

#include "charts/ReadBmsFile.h"

#include <utility>
#include <fstream>
#include <qfileinfo.h>
#include "support/UtfStringToPath.h"
#include "support/PathToQString.h"

#ifdef _WIN32
#include <windows.h>
#include <wil/resource.h>
#else
#include <llfio.hpp>
namespace llfio = LLFIO_V2_NAMESPACE;
#endif


namespace resource_managers {
ChartDataFactory::ChartComponents::ChartComponents(
  std::unique_ptr<gameplay_logic::ChartData> chartData,
  charts::BmsNotesData notesData,
  std::unordered_map<uint16_t, std::filesystem::path> wavs,
  std::unordered_map<uint16_t, std::filesystem::path> bmps)
  : chartData(std::move(chartData))
  , notesData(std::move(notesData))
  , wavs(std::move(wavs))
  , bmps(std::move(bmps))
{
}
ChartDataFactory::ChartComponents::ChartComponents(const ChartComponents& other)
  : chartData(other.chartData ? other.chartData->clone() : nullptr)
  , notesData(other.notesData)
  , wavs(other.wavs)
  , bmps(other.bmps)
{
}
ChartDataFactory::ChartComponents::ChartComponents(
  ChartComponents&& other) noexcept
  : chartData(std::move(other.chartData))
  , notesData(std::move(other.notesData))
  , wavs(std::move(other.wavs))
  , bmps(std::move(other.bmps))
{
}
ChartDataFactory::ChartComponents&
ChartDataFactory::ChartComponents::operator=(const ChartComponents& other)
{
    if (this != &other) {
        chartData = other.chartData ? other.chartData->clone() : nullptr;
        notesData = other.notesData;
        wavs = other.wavs;
        bmps = other.bmps;
    }
    return *this;
}
ChartDataFactory::ChartComponents&
ChartDataFactory::ChartComponents::operator=(ChartComponents&& other) noexcept
{
    if (this != &other) {
        chartData = std::move(other.chartData);
        notesData = std::move(other.notesData);
        wavs = std::move(other.wavs);
        bmps = std::move(other.bmps);
    }
    return *this;
}
auto
ChartDataFactory::makeNotes(
  const std::array<std::vector<charts::BmsNotesData::Note>,
                   charts::BmsNotesData::columnNumber>& notes,
  const std::vector<std::pair<charts::BmsNotesData::Time, double>>& bpmChanges,
  const std::vector<charts::BmsNotesData::Time>& barLines)
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
  const std::vector<charts::BmsNotesData::Note>& column)
  -> QVector<gameplay_logic::Note>
{
    auto columnNotes = QVector<gameplay_logic::Note>{};
    columnNotes.reserve(column.size());
    for (const auto& note : column) {
        auto type = gameplay_logic::Note::Type::Normal;
        switch (note.noteType) {
            case charts::BmsNotesData::NoteType::Normal:
                type = gameplay_logic::Note::Type::Normal;
                break;
            case charts::BmsNotesData::NoteType::LongNoteEnd:
                type = gameplay_logic::Note::Type::LongNoteEnd;
                break;
            case charts::BmsNotesData::NoteType::LongNoteBegin:
                type = gameplay_logic::Note::Type::LongNoteBegin;
                break;
            case charts::BmsNotesData::NoteType::Landmine:
                type = gameplay_logic::Note::Type::Landmine;
                break;
            case charts::BmsNotesData::NoteType::Invisible:
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

auto readAndParse(const std::filesystem::path& chartPath,
                                ChartDataFactory::RandomGenerator randomGenerator)
{
    #ifndef _WIN32
    auto mfh = llfio::mapped_file({}, chartPath).value();
    auto length = mfh.maximum_extent().value();
    auto chart = std::string_view{ reinterpret_cast<char*>(mfh.address()),
                                   static_cast<unsigned long>(length) };
#else
    // use native windows memory mapping
    auto fileHandle = CreateFileW(chartPath.c_str(),
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Could not open chart file");
    }
    auto fileMapping = wil::unique_handle(CreateFileMappingW(fileHandle,
                                        nullptr,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        nullptr));
    if (fileMapping == nullptr) {
        throw std::runtime_error("Could not create file mapping");
    }
    auto mapView = wil::unique_mapview_ptr<void>{MapViewOfFile(fileMapping.get(), FILE_MAP_READ, 0, 0, 0)};
    if (mapView == nullptr) {
        throw std::runtime_error("Could not map view of file");
    }
    auto fileSize = GetFileSize(fileHandle, nullptr);
    if (fileSize == INVALID_FILE_SIZE) {
        throw std::runtime_error("Could not get file size");
    }
    auto chart = std::string_view{ reinterpret_cast<char*>(mapView.get()),
                                   static_cast<unsigned long>(fileSize) };
#endif

    auto sha256 = support::sha256(chart);
    auto md5 = support::md5(chart);
    auto randomValues = QList<qint64>{};
    auto randomGeneratorRecorder =
      [&randomValues, &randomGenerator](
        const charts::ParsedBmsChart::RandomRange number) mutable {
          const auto generated = randomGenerator(number);
          randomValues.append(generated);
          return generated;
      };
    auto parsedChart = charts::readBmsChart(chart, randomGeneratorRecorder);
    return std::make_tuple(std::move(parsedChart),
                           std::move(randomValues),
                           std::move(sha256),
                           std::move(md5));
}

auto
ChartDataFactory::loadChartData(const std::filesystem::path& chartPath,
                                RandomGenerator randomGenerator,
                                int64_t directory) const -> ChartComponents
{
    auto [parsedChart, randomValues, sha256, md5] = readAndParse(chartPath, randomGenerator);
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
    auto calculatedNotesData = charts::BmsNotesData{ parsedChart };

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
    auto bpms = std::unordered_map<double, std::chrono::nanoseconds>{};
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
                case charts::BmsNotesData::NoteType::Normal:
                    normalNotes++;
                    break;
                case charts::BmsNotesData::NoteType::LongNoteBegin:
                    lnNotes++;
                    break;
                case charts::BmsNotesData::NoteType::LongNoteEnd:
                    break;
                case charts::BmsNotesData::NoteType::Landmine:
                    mineNotes++;
                    break;
                case charts::BmsNotesData::NoteType::Invisible:
                    break;
            }
        }
    }
    auto totalNotes = normalNotes + lnNotes;
    auto total = parsedChart.tags.total.value_or(
      (totalNotes + std::clamp(totalNotes - 400, 0, 200)) * 0.16 + 160);
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
      total,
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
    return { std::move(chartData),
             std::move(calculatedNotesData),
             std::move(wavs),
             std::move(bmps) };
}
} // namespace resource_managers