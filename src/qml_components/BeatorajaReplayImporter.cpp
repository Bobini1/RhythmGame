//
// Created by Codex on 30.03.2026.
//

#include "BeatorajaReplayImporter.h"

#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/rules/BmsRanks.h"
#include "gameplay_logic/rules/Lr2Gauge.h"
#include "gameplay_logic/rules/Lr2HitValues.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/Profile.h"
#include "support/GeneratePermutation.h"
#include "support/PathToUtfString.h"
#include "support/QStringToPath.h"
#include "support/UtfStringToPath.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QSet>
#include <QUrl>
#include <QUuid>
#include <QtEndian>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <zlib.h>

#include <array>
#include <algorithm>
#include <filesystem>
#include <optional>

using namespace std::chrono_literals;

namespace qml_components {
namespace {

struct ReplayKeyEvent
{
    qint64 offsetFromStartNs{};
    int keycode{};
    bool pressed{};
    int order{};
};

struct ReplayPayload
{
    QString sha256;
    QList<qint64> randomSequence;
    QList<ReplayKeyEvent> keylog;
    int randomOption{};
    qint64 randomOptionSeed{-1};
    qint64 date{};
    int gauge{};
    int doubleOption{};
};

auto
gzipDecompress(const QByteArray& compressed) -> QByteArray
{
    if (compressed.isEmpty()) {
        return {};
    }

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    stream.avail_in = static_cast<uInt>(compressed.size());

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Failed to initialize gzip decompressor");
    }
    const auto cleanup = qScopeGuard([&stream] { inflateEnd(&stream); });

    QByteArray output;
    std::array<char, 16 * 1024> buffer{};
    int rc = Z_OK;
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer.data());
        stream.avail_out = static_cast<uInt>(buffer.size());
        rc = inflate(&stream, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            throw std::runtime_error("Failed to decompress gzip payload");
        }
        const auto produced =
          static_cast<qsizetype>(buffer.size() - stream.avail_out);
        output.append(buffer.data(), produced);
    } while (rc != Z_STREAM_END);

    return output;
}

auto
decodeKeyinput(const QString& keyinput) -> QList<ReplayKeyEvent>
{
    const auto decoded =
      QByteArray::fromBase64(keyinput.toUtf8(), QByteArray::Base64UrlEncoding);
    const auto raw = gzipDecompress(decoded);

    auto events = QList<ReplayKeyEvent>{};
    events.reserve(raw.size() / 9);
    for (int offset = 0, order = 0; offset + 9 <= raw.size(); offset += 9, ++order) {
        const auto keyByte = static_cast<qint8>(raw[offset]);
        const auto keycode = std::abs(static_cast<int>(keyByte)) - 1;
        const auto timeUs =
          qFromLittleEndian<qint64>(reinterpret_cast<const uchar*>(raw.constData() + offset + 1));
        events.append({
          .offsetFromStartNs = timeUs * 1000,
          .keycode = keycode,
          .pressed = keyByte >= 0,
          .order = order,
        });
    }
    return events;
}

auto
decodeKeylogArray(const QJsonArray& keylog) -> QList<ReplayKeyEvent>
{
    auto events = QList<ReplayKeyEvent>{};
    events.reserve(keylog.size());
    int order = 0;
    for (const auto& entryValue : keylog) {
        if (!entryValue.isObject()) {
            continue;
        }
        const auto entry = entryValue.toObject();
        const auto keycode = entry.contains("keycode")
                               ? entry["keycode"].toInt()
                               : 0;
        const auto hasPresstime = entry.contains("presstime");
        const auto rawTime = static_cast<qint64>(
          hasPresstime ? entry["presstime"].toVariant().toLongLong()
                       : entry["time"].toVariant().toLongLong());
        events.append({
          .offsetFromStartNs = hasPresstime ? rawTime * 1000 : rawTime * 1'000'000,
          .keycode = keycode,
          .pressed = entry["pressed"].toBool(),
          .order = order++,
        });
    }
    return events;
}

auto
parseReplayPayload(const QString& filePath) -> ReplayPayload
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open replay file");
    }

    const auto jsonBytes = gzipDecompress(file.readAll());
    const auto document = QJsonDocument::fromJson(jsonBytes);
    if (!document.isObject()) {
        throw std::runtime_error("Replay file did not contain a JSON object");
    }

    const auto object = document.object();
    auto payload = ReplayPayload{
        .sha256 = object["sha256"].toString().toUpper(),
        .randomOption = object["randomoption"].toInt(0),
        .randomOptionSeed =
          static_cast<qint64>(object["randomoptionseed"].toVariant().toLongLong()),
        .date = static_cast<qint64>(object["date"].toVariant().toLongLong()),
        .gauge = object["gauge"].toInt(-1),
        .doubleOption = object["doubleoption"].toInt(0),
    };
    for (const auto& value : object["rand"].toArray()) {
        payload.randomSequence.append(
          static_cast<qint64>(value.toVariant().toLongLong()));
    }

    if (object.contains("keyinput") && object["keyinput"].isString()) {
        payload.keylog = decodeKeyinput(object["keyinput"].toString());
    } else {
        payload.keylog = decodeKeylogArray(object["keylog"].toArray());
    }

    std::ranges::stable_sort(payload.keylog, [](const auto& left, const auto& right) {
        if (left.offsetFromStartNs != right.offsetFromStartNs) {
            return left.offsetFromStartNs < right.offsetFromStartNs;
        }
        return left.order < right.order;
    });

    if (payload.sha256.isEmpty() || payload.keylog.isEmpty()) {
        throw std::runtime_error("Replay payload is missing chart hash or keylog");
    }

    return payload;
}

auto
getChartPath(resource_managers::Profile& profile, const QString& sha256)
  -> std::optional<std::filesystem::path>
{
    auto statement =
      profile.getDb().createStatement(
        "SELECT path FROM song_db.charts WHERE sha256 = ? LIMIT 1;");
    statement.bind(1, sha256.toStdString());
    return statement.executeAndGet<std::string>().transform(support::utfStringToPath);
}

auto
mapRandomOption(int randomOption)
  -> std::optional<resource_managers::NoteOrderAlgorithm>
{
    using resource_managers::NoteOrderAlgorithm;
    switch (randomOption) {
        case 0:
            return NoteOrderAlgorithm::Normal;
        case 1:
            return NoteOrderAlgorithm::Mirror;
        case 2:
            return NoteOrderAlgorithm::BeatorajaRandom;
        case 3:
            return NoteOrderAlgorithm::BeatorajaRotate;
        case 4:
            return NoteOrderAlgorithm::BeatorajaSRandom;
        case 5:
            return NoteOrderAlgorithm::BeatorajaSpiral;
        case 6:
            return NoteOrderAlgorithm::BeatorajaHRandom;
        case 7:
            return NoteOrderAlgorithm::BeatorajaAllScr;
        case 8:
            return NoteOrderAlgorithm::BeatorajaRandomEx;
        case 9:
            return NoteOrderAlgorithm::BeatorajaSRandomEx;
        default:
            return std::nullopt;
    }
}

auto
mapReplayKeycode(gameplay_logic::ChartData::Keymode keymode, int keycode)
  -> std::optional<input::BmsKey>
{
    using gameplay_logic::ChartData;
    using input::BmsKey;

    switch (keymode) {
        case ChartData::Keymode::K5:
            switch (keycode) {
                case 0:
                    return BmsKey::Col11;
                case 1:
                    return BmsKey::Col12;
                case 2:
                    return BmsKey::Col13;
                case 3:
                    return BmsKey::Col14;
                case 4:
                    return BmsKey::Col15;
                case 5:
                    return BmsKey::Col1sUp;
                case 6:
                    return BmsKey::Col1sDown;
                default:
                    return std::nullopt;
            }
        case ChartData::Keymode::K7:
            switch (keycode) {
                case 0:
                    return BmsKey::Col11;
                case 1:
                    return BmsKey::Col12;
                case 2:
                    return BmsKey::Col13;
                case 3:
                    return BmsKey::Col14;
                case 4:
                    return BmsKey::Col15;
                case 5:
                    return BmsKey::Col16;
                case 6:
                    return BmsKey::Col17;
                case 7:
                    return BmsKey::Col1sUp;
                case 8:
                    return BmsKey::Col1sDown;
                default:
                    return std::nullopt;
            }
        default:
            return std::nullopt;
    }
}

auto
applyImportedOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                   resource_managers::NoteOrderAlgorithm algorithm,
                   uint64_t seed,
                   bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    auto scratchCol = static_cast<int>(notes.size()) - 1;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
        scratchCol = 5;
    }

    const auto result = [&]() {
        if (support::isBeatorajaNoteOrderAlgorithm(algorithm)) {
            const auto includeScratch =
              support::beatorajaNoteOrderIncludesScratch(algorithm);
            switch (algorithm) {
                case resource_managers::NoteOrderAlgorithm::Mirror:
                case resource_managers::NoteOrderAlgorithm::BeatorajaRandom:
                case resource_managers::NoteOrderAlgorithm::BeatorajaRotate:
                case resource_managers::NoteOrderAlgorithm::BeatorajaRandomEx:
                    return support::generateBeatorajaLanePermutation(
                      workingNotes,
                      algorithm,
                      static_cast<int64_t>(seed),
                      scratchCol,
                      includeScratch);
                default:
                    return support::generateBeatorajaPermutation(
                      workingNotes,
                      support::beatorajaRandomFromNoteOrderAlgorithm(algorithm),
                      static_cast<int64_t>(seed),
                      scratchCol,
                      includeScratch);
            }
        }
        return support::generateBeatorajaLanePermutation(
          workingNotes,
          algorithm,
          static_cast<int64_t>(seed),
          scratchCol,
          /*includeScratch=*/algorithm == resource_managers::NoteOrderAlgorithm::Normal);
    }();

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
createScoreFromReplay(resource_managers::Profile& profile,
                      const ReplayPayload& replay)
  -> std::unique_ptr<gameplay_logic::BmsScore>
{
    auto chartPath = getChartPath(profile, replay.sha256);
    if (!chartPath) {
        throw std::runtime_error("Chart hash was not found in the song database");
    }
    if (replay.doubleOption != 0) {
        throw std::runtime_error("Only single-play beatoraja replays are supported");
    }

    auto chartDataFactory = resource_managers::ChartDataFactory{};
    const auto recordedRandoms = replay.randomSequence;
    auto randomGenerator =
      [recordedRandoms, index = 0](charts::ParsedBmsChart::RandomRange range) mutable {
          if (index < recordedRandoms.size()) {
              return static_cast<charts::ParsedBmsChart::RandomRange>(
                recordedRandoms[index++]);
          }
          if (range <= 1) {
              return charts::ParsedBmsChart::RandomRange{ 1 };
          }
          return charts::ParsedBmsChart::RandomRange{ 1 };
      };
    auto chartComponents = chartDataFactory.loadChartData(*chartPath, randomGenerator);
    auto* chartData = chartComponents.chartData.get();
    if (chartData->getIsRandom() &&
        chartData->getRandomSequence().size() != replay.randomSequence.size()) {
        throw std::runtime_error("Replay RANDOM sequence did not match chart requirements");
    }
    if (chartData->getKeymode() != gameplay_logic::ChartData::Keymode::K5 &&
        chartData->getKeymode() != gameplay_logic::ChartData::Keymode::K7) {
        throw std::runtime_error("Only 5-key and 7-key replays are supported");
    }

    const auto algorithm = mapRandomOption(replay.randomOption);
    if (!algorithm) {
        throw std::runtime_error("Replay used an unsupported beatoraja random option");
    }
    if (*algorithm != resource_managers::NoteOrderAlgorithm::Normal &&
        *algorithm != resource_managers::NoteOrderAlgorithm::Mirror &&
        replay.randomOptionSeed < 0) {
        throw std::runtime_error("Replay is missing the random option seed");
    }

    auto visibleNotes = chartComponents.notesData.notes;
    auto notesSpan = std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
    const auto is5k = chartData->getKeymode() == gameplay_logic::ChartData::Keymode::K5;
    const auto shuffle = applyImportedOrder(
      notesSpan,
      *algorithm,
      replay.randomOptionSeed < 0 ? 0 : static_cast<uint64_t>(replay.randomOptionSeed),
      is5k);

    const auto rank = magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(
                        chartData->getRank())
                        .value_or(gameplay_logic::rules::defaultBmsRank);
    auto gaugesRaw = gameplay_logic::rules::Lr2Gauge::getGauges(
      chartData->getTotal(),
      chartData->getNormalNoteCount() + chartData->getLnCount() +
        chartData->getBssCount() + chartData->getScratchCount());
    auto gauges = QList<gameplay_logic::rules::BmsGauge*>{};
    for (auto& gauge : gaugesRaw) {
        gauges.append(gauge.release());
    }

    auto liveScore = std::make_unique<gameplay_logic::BmsLiveScore>(
      chartData->getNormalNoteCount(),
      chartData->getScratchCount(),
      chartData->getLnCount(),
      chartData->getBssCount(),
      chartData->getMineCount(),
      chartData->getNormalNoteCount() + chartData->getLnCount() +
        chartData->getBssCount() + chartData->getScratchCount(),
      gameplay_logic::rules::lr2_hit_values::getLr2HitValue(
        0ns, gameplay_logic::Judgement::Perfect),
      gauges,
      chartData->getRandomSequence(),
      *algorithm,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      shuffle.columns,
      shuffle.seed,
      chartData->getLength(),
      chartData->getSha256(),
      chartData->getMd5(),
      chartData->getKeymode(),
      replay.date > 0 ? replay.date : 0,
      QUuid::createUuid().toString());

    auto referee = gameplay_logic::BmsGameReferee(
      visibleNotes,
      chartComponents.notesData.bgmNotes,
      chartComponents.notesData.bpmChanges,
      nullptr,
      liveScore.get(),
      {},
      gameplay_logic::rules::HitRules(
        gameplay_logic::rules::lr2_timing_windows::getTimingWindows(rank),
        gameplay_logic::rules::lr2_hit_values::getLr2HitValue));

    for (const auto& event : replay.keylog) {
        const auto mapped = mapReplayKeycode(chartData->getKeymode(), event.keycode);
        if (!mapped) {
            throw std::runtime_error("Replay contained an unsupported keycode for this chart");
        }
        const auto offset = std::chrono::nanoseconds(event.offsetFromStartNs);
        referee.update(offset, false);
        if (event.pressed) {
            referee.passPressed(offset, *mapped);
        } else {
            referee.passReleased(offset, *mapped);
        }
    }

    const auto endOffset = std::chrono::nanoseconds(chartData->getLength()) + 10s;
    referee.update(endOffset, true);
    for (int i = 0; i < charts::BmsNotesData::columnNumber; ++i) {
        referee.passReleased(endOffset, static_cast<input::BmsKey>(i));
    }

    return std::make_unique<gameplay_logic::BmsScore>(liveScore->getResult(),
                                                      liveScore->getReplayData(),
                                                      liveScore->getGaugeHistory());
}

} // namespace

BeatorajaReplayImporter::BeatorajaReplayImporter(QObject* parent)
  : QObject(parent)
{
}

QVariantMap
BeatorajaReplayImporter::importFolder(resource_managers::Profile* profile,
                                      const QString& folderPath) const
{
    auto result = QVariantMap{
        { "imported", 0 },
        { "skipped", 0 },
        { "errors", QStringList{} },
    };
    if (profile == nullptr) {
        result["errors"] = QStringList{ QStringLiteral("Profile is null") };
        return result;
    }

    auto resolvedFolderPath = folderPath;
    const auto folderUrl = QUrl(folderPath);
    if (folderUrl.isValid() && folderUrl.isLocalFile()) {
        resolvedFolderPath = folderUrl.toLocalFile();
    }

    const auto directory = QDir(resolvedFolderPath);
    if (!directory.exists()) {
        result["errors"] = QStringList{ QStringLiteral("Folder does not exist") };
        return result;
    }

    auto imported = 0;
    auto skipped = 0;
    auto errors = QStringList{};
    auto iterator = QDirIterator(resolvedFolderPath,
                                 { QStringLiteral("*.brd") },
                                 QDir::Files,
                                 QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const auto replayPath = iterator.next();
        try {
            auto replay = parseReplayPayload(replayPath);
            auto score = createScoreFromReplay(*profile, replay);
            score->save(profile->getDb());
            ++imported;
        } catch (const std::exception& e) {
            ++skipped;
            const auto error = QStringLiteral("%1: %2")
                                 .arg(replayPath, QString::fromUtf8(e.what()));
            errors.append(error);
            spdlog::warn("Failed to import beatoraja replay {}: {}",
                         replayPath.toStdString(),
                         e.what());
        }
    }

    result["imported"] = imported;
    result["skipped"] = skipped;
    result["errors"] = errors;
    return result;
}

} // namespace qml_components
