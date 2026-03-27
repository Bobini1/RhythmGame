//
// Created by bobini on 23.08.23.
//

#include "charts/Base62.h"
#include "support/Version.h"
#ifdef _WIN32
#include <windows.h>
#include <wil/resource.h>
#else
#include <llfio.hpp>
namespace llfio = LLFIO_V2_NAMESPACE;
#endif

#include "ChartDataFactory.h"

#include "charts/ReadBmsFile.h"

#include <utility>
#include <ranges>
#include <qfileinfo.h>
#include "support/UtfStringToPath.h"
#include "support/PathToQString.h"
#include <magic_enum/magic_enum.hpp>
#include <QJsonDocument>
#include <QJsonArray>

namespace resource_managers {
ChartDataFactory::ChartComponents::ChartComponents(
  std::unique_ptr<gameplay_logic::ChartData> chartData,
  charts::BmsNotesData notesData,
  std::unordered_map<uint64_t, std::filesystem::path> wavs,
  std::unordered_map<uint64_t, std::filesystem::path> bmps)
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
  const std::vector<charts::BmsNotesData::Time>& barLines)
  -> std::unique_ptr<gameplay_logic::BmsNotes>
{
    auto visibleNotesQ = QList<QList<gameplay_logic::Note>>{};
    for (const auto& column : notes) {
        visibleNotesQ.append(convertToQList(column));
    }
    auto barLinesQ = QList<gameplay_logic::Time>{};
    for (const auto& barLine : barLines) {
        barLinesQ.append({ barLine.timestamp.count(), barLine.position });
    }
    return std::make_unique<gameplay_logic::BmsNotes>(std::move(visibleNotesQ),
                                                      std::move(barLinesQ));
}
auto
ChartDataFactory::convertToQList(
  const std::vector<charts::BmsNotesData::Note>& column)
  -> QList<gameplay_logic::Note>
{
    auto columnNotes = QList<gameplay_logic::Note>{};
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

// Helper: memory-map a file and invoke the callback with its contents.
// The mapped memory is valid for the duration of the callback.
template<typename Func>
auto
withMappedFile(const std::filesystem::path& path, Func&& func)
  -> decltype(func(std::string_view{}))
{
#ifndef _WIN32
    auto mfh = llfio::mapped_file({}, path).value();
    auto length = mfh.maximum_extent().value();
    auto content = std::string_view{ reinterpret_cast<char*>(mfh.address()),
                                     static_cast<unsigned long>(length) };
    return func(content);
#else
    auto fileHandle = wil::unique_handle(CreateFileW(path.c_str(),
                                                     GENERIC_READ,
                                                     FILE_SHARE_READ,
                                                     nullptr,
                                                     OPEN_EXISTING,
                                                     FILE_ATTRIBUTE_NORMAL,
                                                     nullptr));
    if (fileHandle.get() == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Could not open file");
    }
    auto fileMapping = wil::unique_handle(CreateFileMappingW(
      fileHandle.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
    if (fileMapping == nullptr) {
        throw std::runtime_error("Could not create file mapping");
    }
    auto mapView = wil::unique_mapview_ptr<void>{ MapViewOfFile(
      fileMapping.get(), FILE_MAP_READ, 0, 0, 0) };
    if (mapView == nullptr) {
        throw std::runtime_error("Could not map view of file");
    }
    auto fileSize = GetFileSize(fileHandle.get(), nullptr);
    if (fileSize == INVALID_FILE_SIZE) {
        throw std::runtime_error("Could not get file size");
    }
    auto content = std::string_view{ reinterpret_cast<char*>(mapView.get()),
                                     static_cast<unsigned long>(fileSize) };
    return func(content);
#endif
}

auto
readAndParse(const std::filesystem::path& chartPath,
             ChartDataFactory::RandomGenerator randomGenerator)
{
    return withMappedFile(chartPath, [&](std::string_view chart) {
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
    });
}

using namespace std::chrono_literals;

QList<QList<qint64>>
createHistogram(const charts::BmsNotesData& calculatedNotesData,
                std::chrono::nanoseconds lastNoteTimestamp,
                size_t numBuckets)
{
    auto histogram = QList<QList<qint64>>{};
    histogram.resize(
      magic_enum::enum_count<gameplay_logic::ChartData::HistogramNoteType>());
    for (auto& column : histogram) {
        column.resize(numBuckets, 0);
    }
    if (lastNoteTimestamp != 0ns) {
        for (const auto& [columnIndex, column] :
             std::ranges::views::enumerate(calculatedNotesData.notes)) {
            auto lastLnBeginPosition = size_t{ 0 };
            for (const auto note : column) {
                auto typeIndex = 0;
                auto position =
                  static_cast<double>(note.time.timestamp.count()) /
                  static_cast<double>(lastNoteTimestamp.count()) * numBuckets;
                auto positionIndex = static_cast<size_t>(position);
                // Include the last note(s) in the last bucket
                if (positionIndex == numBuckets) {
                    positionIndex = numBuckets - 1;
                }
                switch (note.noteType) {
                    case charts::BmsNotesData::NoteType::LongNoteBegin:
                        lastLnBeginPosition = positionIndex;
                        typeIndex =
                          (columnIndex == 7 || columnIndex == 15) ? 3 : 2;
                        break;
                    case charts::BmsNotesData::NoteType::Normal:
                        typeIndex =
                          (columnIndex == 7 || columnIndex == 15) ? 1 : 0;
                        break;
                    case charts::BmsNotesData::NoteType::Landmine:
                        typeIndex = 4;
                        break;
                    case charts::BmsNotesData::NoteType::Invisible:
                        typeIndex = 5;
                        break;
                    case charts::BmsNotesData::NoteType::LongNoteEnd:
                        typeIndex =
                          (columnIndex == 7 || columnIndex == 15) ? -3 : -2;
                        // LnEnds should belong to the previous bin if they fall
                        // exactly on a bin boundary
                        auto prevF = std::nexttoward(position, 0.0);
                        positionIndex = static_cast<size_t>(prevF);
                }
                // Lns can span multiple buckets
                if (typeIndex == -2 || typeIndex == -3) {
                    for (auto index = lastLnBeginPosition;
                         index <= positionIndex;
                         index++) {
                        histogram[-typeIndex][index]++;
                    }
                } else if (typeIndex != 2 && typeIndex != 3) {
                    if (positionIndex < numBuckets) {
                        histogram[typeIndex][positionIndex]++;
                    }
                }
            }
        }
    }
    return histogram;
}

void
ChartDataFactory::handleImplicitSubtitle(QString& title,
                                         QString& subtitle) const
{
    if (title.size() < 3 || !subtitle.isEmpty()) {
        return;
    }

    auto u32 = title.toStdU32String();
    auto delimiters = std::array{ U'～', U'〜', U'-',  U')',  U']', U'>',
                                  U'"',  U'〕', U'】', U'］', U'）' };
    auto delimitersStart = std::array{ U'～', U'〜', U'-',  U'(',  U'[', U'<',
                                       U'"',  U'〔', U'【', U'［', U'（' };

    auto selectedDelim = U'\0';
    for (const auto& delimiter : delimiters) {
        if (u32.back() == delimiter)
            selectedDelim = delimiter;
    }
    if (selectedDelim == U'\0')
        return;

    auto delimIndex =
      std::ranges::find(delimiters, selectedDelim) - delimiters.begin();
    auto delimStart = delimitersStart[delimIndex];

    // For non-paired delimiters (～, -, ") fall back to find_last_of
    // since there's no nesting concept.
    size_t openPos = std::u32string::npos;
    if (delimStart == selectedDelim) {
        openPos = u32.find_last_of(delimStart, u32.size() - 2);
    } else {
        // Walk backwards, tracking nesting depth to find the matching open.
        int depth = 0;
        for (size_t i = u32.size() - 1; i > 0; --i) {
            if (u32[i] == selectedDelim)
                ++depth;
            else if (u32[i] == delimStart) {
                --depth;
                if (depth == 0) {
                    openPos = i;
                    break;
                }
            }
        }
    }

    if (openPos == std::u32string::npos || openPos == 0)
        return;
    // To avoid weird titles like "(^^)⇒(^^X^^)⇒(^^)) ((^^)"
    for (const auto& [index, delimiter] :
         std::ranges::views::enumerate(delimitersStart)) {
        if (u32[openPos - 1] == delimiter && delimiter != delimiters[index]) {
            return;
        }
    }
    // This is pretty much specifically for Δ(^^)
    if (u32[openPos - 1] == U'Δ') {
        return;
    }

    auto count = u32.size() - openPos;
    if (count == 2)
        return;

    auto subTitleU32 = std::u32string(u32.data() + openPos, count);
    auto remainderU32 = std::u32string(u32.data(), openPos);

    subtitle = QString::fromStdU32String(subTitleU32);
    auto titleWithoutTrim = QString::fromStdU32String(remainderU32);
    title = titleWithoutTrim.trimmed();
    auto titleSpaces = titleWithoutTrim.mid(title.size());

    QString innerSubtitle;
    handleImplicitSubtitle(title, innerSubtitle);

    if (!innerSubtitle.isEmpty())
        subtitle = innerSubtitle + titleSpaces + subtitle;
}
auto
ChartDataFactory::loadChartData(const std::filesystem::path& chartPath,
                                RandomGenerator randomGenerator,
                                int64_t directory) const -> ChartComponents
{
    auto [parsedChart, randomValues, sha256, md5] =
      readAndParse(chartPath, randomGenerator);

    auto title = QString::fromUtf8(parsedChart.tags.title.value_or(""));
    auto subtitle = QString::fromUtf8(parsedChart.tags.subTitle.value_or(""));
    if (parsedChart.tags.title.has_value() &&
        !parsedChart.tags.subTitle.has_value()) {
        handleImplicitSubtitle(title, subtitle);
    }

    auto metadata = ChartMetadata{
        .title = title,
        .artist = QString::fromUtf8(parsedChart.tags.artist.value_or("")),
        .subtitle = subtitle,
        .subartist = QString::fromUtf8(parsedChart.tags.subArtist.value_or("")),
        .genre = QString::fromUtf8(parsedChart.tags.genre.value_or("")),
        .stageFile = QString::fromUtf8(parsedChart.tags.stageFile.value_or("")),
        .banner = QString::fromUtf8(parsedChart.tags.banner.value_or("")),
        .backBmp = QString::fromUtf8(parsedChart.tags.backBmp.value_or("")),
        .rank = parsedChart.tags.rank.value_or(2),
        .total = parsedChart.tags.total.value_or(-1.0),
        .playLevel = parsedChart.tags.playLevel.value_or(1),
        .difficulty = parsedChart.tags.difficulty.value_or(1),
        .isRandom = parsedChart.tags.isRandom,
        .randomSequence = randomValues,
        .sha256 = QString::fromStdString(sha256),
        .md5 = QString::fromStdString(md5),
    };

    std::unordered_map<uint64_t, std::filesystem::path> wavs;
    wavs.reserve(parsedChart.tags.wavs.size());
    auto base = parsedChart.tags.base.value_or(36);
    for (const auto& [id, path] : parsedChart.tags.wavs) {
        auto targetId = id;
        if (base == 36) {
            targetId = charts::base62ToBase36(id);
        }
        wavs[targetId] = support::utfStringToPath(path);
    }
    std::unordered_map<uint64_t, std::filesystem::path> bmps;
    bmps.reserve(parsedChart.tags.bmps.size());
    for (const auto& [id, path] : parsedChart.tags.bmps) {
        auto targetId = id;
        if (base == 36) {
            targetId = charts::base62ToBase36(id);
        }
        bmps[targetId] = support::utfStringToPath(path);
    }

    auto calculatedNotesData =
      charts::BmsNotesData::fromParsedChart(parsedChart);

    return buildChartComponents(std::move(calculatedNotesData),
                                std::move(metadata),
                                std::move(wavs),
                                std::move(bmps),
                                chartPath,
                                directory);
}

auto
ChartDataFactory::loadBmsonChartData(const std::filesystem::path& chartPath,
                                     int64_t directory) const -> ChartComponents
{
    return withMappedFile(
      chartPath, [&](std::string_view fileContent) -> ChartComponents {
          auto sha256 = support::sha256(fileContent);
          auto md5 = support::md5(fileContent);

          auto jsonDoc = QJsonDocument::fromJson(QByteArray(
            fileContent.data(), static_cast<qsizetype>(fileContent.size())));
          if (jsonDoc.isNull()) {
              throw std::runtime_error("Could not parse bmson file as JSON");
          }
          auto bmson = jsonDoc.object();

          // Extract metadata from bmson info object
          auto info = bmson["info"].toObject();
          auto subartists = info["subartists"].toArray();
          auto subartistStr = QString{};
          for (const auto& sa : subartists) {
              if (!subartistStr.isEmpty()) {
                  subartistStr += QStringLiteral(" / ");
              }
              subartistStr += sa.toString();
          }

          auto metadata = ChartMetadata{
              .title = info["title"].toString(),
              .artist = info["artist"].toString(),
              .subtitle = info["subtitle"].toString(),
              .subartist = subartistStr,
              .genre = info["genre"].toString(),
              .stageFile = info["eyecatch_image"].toString(),
              .banner = info["banner_image"].toString(),
              .backBmp = info["back_image"].toString(),
              .rank = info["judge_rank"].toInt(2),
              .total = info["total"].toDouble(-1.0),
              .playLevel = info["level"].toInt(1),
              .difficulty = 1,
              .isRandom = false,
              .randomSequence = {},
              .sha256 = QString::fromStdString(sha256),
              .md5 = QString::fromStdString(md5),
          };

          // Extract wav paths from sound_channels (keyed by channel
          // index to match BmsonSliceInfo::channelIndex)
          std::unordered_map<uint64_t, std::filesystem::path> wavs;
          auto soundChannels = bmson["sound_channels"].toArray();
          for (uint64_t idx = 0; idx < soundChannels.size(); ++idx) {
              auto channelObj = soundChannels[idx].toObject();
              auto name = channelObj["name"].toString().toStdString();
              wavs.emplace(idx, std::filesystem::path{ name });
          }

          // Extract bmp paths from bga headers
          std::unordered_map<uint64_t, std::filesystem::path> bmps;
          auto bgaObj = bmson["bga"].toObject();
          for (const auto& header : bgaObj["bga_header"].toArray()) {
              auto headerObj = header.toObject();
              auto id = headerObj["id"].toInteger(0);
              auto name = headerObj["name"].toString().toStdString();
              bmps.emplace(id, std::filesystem::path{ name });
          }

          auto calculatedNotesData = charts::BmsNotesData::fromBmson(bmson);

          return buildChartComponents(std::move(calculatedNotesData),
                                      std::move(metadata),
                                      std::move(wavs),
                                      std::move(bmps),
                                      chartPath,
                                      directory);
      });
}

auto
ChartDataFactory::buildChartComponents(
  charts::BmsNotesData calculatedNotesData,
  ChartMetadata metadata,
  std::unordered_map<uint64_t, std::filesystem::path> wavs,
  std::unordered_map<uint64_t, std::filesystem::path> bmps,
  const std::filesystem::path& chartPath,
  int64_t directory) -> ChartComponents
{
    auto lastNoteTimestamp = 0ns;
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
    if (calculatedNotesData.notes[5].empty() &&
        calculatedNotesData.notes[6].empty()) {
        keymode = gameplay_logic::ChartData::Keymode::K5;
    }
    constexpr auto startColumn = calculatedNotesData.notes.size() / 2;
    for (auto columnIndex = startColumn;
         columnIndex < calculatedNotesData.notes.size();
         columnIndex++) {
        if (!calculatedNotesData.notes[columnIndex].empty()) {
            keymode = gameplay_logic::ChartData::Keymode::K14;
            break;
        }
    }
    if (keymode == gameplay_logic::ChartData::Keymode::K14) {
        if (calculatedNotesData.notes[5].empty() &&
            calculatedNotesData.notes[6].empty() &&
            calculatedNotesData.notes[13].empty() &&
            calculatedNotesData.notes[14].empty()) {
            keymode = gameplay_logic::ChartData::Keymode::K10;
        }
    }
    auto initialBpm = calculatedNotesData.bpmChanges[0]; // guaranteed to exist
    auto maxBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.bpm > maxBpm.bpm) {
            maxBpm = bpmChange;
        }
    }
    auto minBpm = initialBpm;
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        if (bpmChange.bpm > 0.0 && bpmChange.bpm < minBpm.bpm) {
            minBpm = bpmChange;
        }
    }
    auto bpms = std::unordered_map<double, std::chrono::nanoseconds>{};
    for (auto it = calculatedNotesData.bpmChanges.begin();
         it != calculatedNotesData.bpmChanges.end();
         ++it) {
        auto bpmChange = *it;
        auto bpm = bpmChange.bpm;
        auto timestamp = bpmChange.timestamp.timestamp;
        auto nextTimestamp = 0ns;
        if (it + 1 != calculatedNotesData.bpmChanges.end()) {
            nextTimestamp = (it + 1)->timestamp.timestamp;
        } else {
            nextTimestamp =
              timestamp > lastNoteTimestamp ? timestamp : lastNoteTimestamp;
        }
        auto duration = nextTimestamp - timestamp;
        bpms[bpm] += duration;
    }
    auto totalBpm = 0.0;
    auto totalDuration = 0ns;
    for (const auto& [bpm, duration] : bpms) {
        totalBpm += bpm * duration.count();
        totalDuration += duration;
    }
    // find main bpm, which is the bpm with the longest duration
    auto mainBpm = initialBpm.bpm;
    auto longestDuration = 0ns;
    for (const auto& [bpm, duration] : bpms) {
        if (duration > longestDuration && bpm > 0.0) {
            longestDuration = duration;
            mainBpm = bpm;
        }
    }
    auto avgBpm = totalBpm;
    if (totalDuration.count() > 0) {
        avgBpm /= totalDuration.count();
    }
    auto normalNotes = 0;
    auto scratchNotes = 0;
    auto lnNotes = 0;
    auto bssNotes = 0;
    auto mineNotes = 0;
    for (const auto& [index, column] :
         std::ranges::views::enumerate(calculatedNotesData.notes)) {
        for (const auto& note : column) {
            switch (note.noteType) {
                case charts::BmsNotesData::NoteType::Normal:
                    if (index == 7 || index == 15) {
                        scratchNotes++;
                    } else {
                        normalNotes++;
                    }
                    break;
                case charts::BmsNotesData::NoteType::LongNoteBegin:
                    if (index == 7 || index == 15) {
                        bssNotes++;
                    } else {
                        lnNotes++;
                    }
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
    auto total = metadata.total;
    if (total < 0.0) {
        total =
          (totalNotes + std::clamp(totalNotes - 400, 0, 200)) * 0.16 + 160;
    }
    auto path = support::pathToQString(chartPath);
    auto bpmChangesQ = QList<gameplay_logic::BpmChange>{};
    for (const auto& bpmChange : calculatedNotesData.bpmChanges) {
        bpmChangesQ.append({
          { .timestamp = bpmChange.timestamp.timestamp.count(),
            .position = bpmChange.timestamp.position,
            .beatPosition = bpmChange.timestamp.beatPosition },
          bpmChange.bpm,
          bpmChange.scroll,
        });
    }
    const auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(lastNoteTimestamp);
    auto numSeconds = seconds.count();
    if (numSeconds == 0) {
        numSeconds++;
    }
    auto histogramForStats =
      createHistogram(calculatedNotesData, lastNoteTimestamp, numSeconds);
    auto summedNoteNumberBins = QList<int64_t>{};
    summedNoteNumberBins.reserve(histogramForStats[0].size());
    for (size_t i = 0; i < histogramForStats[0].size(); i++) {
        auto sum = int64_t{ 0 };
        sum += histogramForStats[0][i];
        sum += histogramForStats[1][i];
        sum += histogramForStats[2][i];
        sum += histogramForStats[3][i];
        summedNoteNumberBins.append(sum);
    }
    auto histogramForDisplay = std::move(histogramForStats);
    auto avg = std::accumulate(summedNoteNumberBins.begin(),
                               summedNoteNumberBins.end(),
                               0.0) /
               summedNoteNumberBins.size();
    auto peak = *std::ranges::max_element(summedNoteNumberBins);
    auto last = summedNoteNumberBins.back();
    auto chartData =
      std::make_unique<gameplay_logic::ChartData>(std::move(metadata.title),
                                                  std::move(metadata.artist),
                                                  std::move(metadata.subtitle),
                                                  std::move(metadata.subartist),
                                                  std::move(metadata.genre),
                                                  std::move(metadata.stageFile),
                                                  std::move(metadata.banner),
                                                  std::move(metadata.backBmp),
                                                  metadata.rank,
                                                  total,
                                                  metadata.playLevel,
                                                  metadata.difficulty,
                                                  metadata.isRandom,
                                                  metadata.randomSequence,
                                                  normalNotes,
                                                  scratchNotes,
                                                  lnNotes,
                                                  bssNotes,
                                                  mineNotes,
                                                  lastNoteTimestamp.count(),
                                                  initialBpm.bpm,
                                                  maxBpm.bpm,
                                                  minBpm.bpm,
                                                  mainBpm,
                                                  avgBpm,
                                                  peak,
                                                  avg,
                                                  last,
                                                  path,
                                                  directory,
                                                  std::move(metadata.sha256),
                                                  std::move(metadata.md5),
                                                  keymode,
                                                  histogramForDisplay,
                                                  bpmChangesQ,
                                                  support::currentVersion);
    auto noteData =
      makeNotes(calculatedNotesData.notes, calculatedNotesData.barLines);
    return { std::move(chartData),
             std::move(calculatedNotesData),
             std::move(wavs),
             std::move(bmps) };
}
} // namespace resource_managers
