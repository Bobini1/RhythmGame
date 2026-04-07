//
// Created by Codex on 30.03.2026.
//

#include "BeatorajaReplayImporter.h"
#include "ReplayImportOperation.h"

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
#include <zlib-ng.h>

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
    qint64 randomOptionSeed{ -1 };
    int randomOptionP2{};
    qint64 randomOptionSeedP2{ -1 };
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

    zng_stream stream{};
    stream.next_in =
      reinterpret_cast<uint8_t*>(const_cast<char*>(compressed.data()));
    stream.avail_in = static_cast<uint32_t>(compressed.size());

    if (zng_inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Failed to initialize gzip decompressor");
    }
    const auto cleanup = qScopeGuard([&stream] { zng_inflateEnd(&stream); });

    QByteArray output;
    std::array<char, 16 * 1024> buffer{};
    int rc = Z_OK;
    do {
        stream.next_out = reinterpret_cast<uint8_t*>(buffer.data());
        stream.avail_out = static_cast<uint32_t>(buffer.size());
        rc = zng_inflate(&stream, Z_NO_FLUSH);
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
    for (int offset = 0, order = 0; offset + 9 <= raw.size();
         offset += 9, ++order) {
        const auto keyByte = static_cast<qint8>(raw[offset]);
        const auto keycode = std::abs(static_cast<int>(keyByte)) - 1;
        const auto timeUs = qFromLittleEndian<qint64>(
          reinterpret_cast<const uchar*>(raw.constData() + offset + 1));
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
        const auto keycode =
          entry.contains("keycode") ? entry["keycode"].toInt() : 0;
        const auto hasPresstime = entry.contains("presstime");
        const auto rawTime = static_cast<qint64>(
          hasPresstime ? entry["presstime"].toVariant().toLongLong()
                       : entry["time"].toVariant().toLongLong());
        events.append({
          .offsetFromStartNs =
            hasPresstime ? rawTime * 1000 : rawTime * 1'000'000,
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
        .randomOptionSeed = object["randomoptionseed"].toInteger(0),
        .randomOptionP2 = object["randomoption2"].toInt(0),
        .randomOptionSeedP2 = object["randomoption2seed"].toInteger(0),
        .date = static_cast<qint64>(object["date"].toVariant().toLongLong()),
        .gauge = object["gauge"].toInt(-1),
        .doubleOption = object["doubleoption"].toInt(0),
    };
    for (const auto& value : object["rand"].toArray()) {
        payload.randomSequence.append(value.toVariant().toLongLong());
    }

    if (object.contains("keyinput") && object["keyinput"].isString()) {
        payload.keylog = decodeKeyinput(object["keyinput"].toString());
    } else {
        payload.keylog = decodeKeylogArray(object["keylog"].toArray());
    }

    std::ranges::stable_sort(
      payload.keylog, [](const auto& left, const auto& right) {
          if (left.offsetFromStartNs != right.offsetFromStartNs) {
              return left.offsetFromStartNs < right.offsetFromStartNs;
          }
          return left.order < right.order;
      });

    if (payload.sha256.isEmpty()) {
        throw std::runtime_error("Replay payload is missing chart hash");
    }

    return payload;
}

auto
getChartPath(resource_managers::Profile& profile, const QString& sha256)
  -> std::optional<std::filesystem::path>
{
    auto statement = profile.getDb().createStatement(
      "SELECT path FROM song_db.charts WHERE sha256 = ? LIMIT 1;");
    statement.bind(1, sha256.toStdString());
    return statement.executeAndGet<std::string>().transform(
      support::utfStringToPath);
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
        case 5:
            return NoteOrderAlgorithm::BeatorajaRandomEx;
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
        case ChartData::Keymode::K10:
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
                case 7:
                    return BmsKey::Col21;
                case 8:
                    return BmsKey::Col22;
                case 9:
                    return BmsKey::Col23;
                case 10:
                    return BmsKey::Col24;
                case 11:
                    return BmsKey::Col25;
                case 12:
                    return BmsKey::Col2sUp;
                case 13:
                    return BmsKey::Col2sDown;
                default:
                    return std::nullopt;
            }
        case ChartData::Keymode::K14:
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
                case 9:
                    return BmsKey::Col21;
                case 10:
                    return BmsKey::Col22;
                case 11:
                    return BmsKey::Col23;
                case 12:
                    return BmsKey::Col24;
                case 13:
                    return BmsKey::Col25;
                case 14:
                    return BmsKey::Col26;
                case 15:
                    return BmsKey::Col27;
                case 16:
                    return BmsKey::Col2sUp;
                case 17:
                    return BmsKey::Col2sDown;
                default:
                    return std::nullopt;
            }
        default:
            return std::nullopt;
    }
}

auto
requiresReplaySeed(resource_managers::NoteOrderAlgorithm algorithm) -> bool
{
    return algorithm != resource_managers::NoteOrderAlgorithm::Normal &&
           algorithm != resource_managers::NoteOrderAlgorithm::Mirror;
}

auto
flipPlayfields(std::array<std::vector<charts::BmsNotesData::Note>,
                          charts::BmsNotesData::columnNumber>& visibleNotes)
  -> void
{
    for (int i = 0; i < 7; ++i) {
        std::swap(visibleNotes[14 - i], visibleNotes[i]);
    }
    std::swap(visibleNotes[15], visibleNotes[7]);
}

auto
applyImportedOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                   resource_managers::NoteOrderAlgorithm algorithm,
                   uint64_t seed,
                   bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
    }

    const auto result = support::generateBeatorajaLanePermutation(
      workingNotes, algorithm, static_cast<int64_t>(seed));

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
createScoreFromReplay(resource_managers::Profile& profile,
                      ReplayPayload replay,
                      const QString& guid)
  -> std::unique_ptr<gameplay_logic::BmsScore>
{
    auto chartPath = getChartPath(profile, replay.sha256);
    if (!chartPath) {
        throw std::runtime_error(
          "Chart hash was not found in the song database");
    }

    auto chartDataFactory = resource_managers::ChartDataFactory{};
    const auto recordedRandoms = replay.randomSequence;
    auto randomGenerator =
      [recordedRandoms,
       index = 0](charts::ParsedBmsChart::RandomRange range) mutable {
          if (index < recordedRandoms.size()) {
              return static_cast<charts::ParsedBmsChart::RandomRange>(
                recordedRandoms[index++]);
          }
          if (range <= 1) {
              return charts::ParsedBmsChart::RandomRange{ 1 };
          }
          return charts::ParsedBmsChart::RandomRange{ 1 };
      };
    auto chartComponents =
      chartDataFactory.loadChartData(*chartPath, randomGenerator);
    auto& chartData = chartComponents.chartData;
    if (chartData->getIsRandom() &&
        chartData->getRandomSequence().size() != replay.randomSequence.size()) {
        throw std::runtime_error(
          "Replay RANDOM sequence did not match chart requirements");
    }
    const auto algorithm = mapRandomOption(replay.randomOption);
    const auto algorithmP2 = mapRandomOption(replay.randomOptionP2);
    if (!algorithm) {
        throw std::runtime_error(
          "Replay used an unsupported beatoraja random option");
    }
    if (!algorithmP2) {
        throw std::runtime_error("Replay used an unsupported beatoraja random "
                                 "option for player 2 side");
    }

    auto keymode = chartData->getKeymode();
    auto dpOptions = resource_managers::DpOptions::Off;
    auto visibleNotes = chartComponents.notesData.notes;
    if (replay.doubleOption == 3) {
        throw std::runtime_error("BATTLE replays are not supported atm");
    }

    switch (replay.doubleOption) {
        case 0:
            break;
        case 1:
            if (keymode == gameplay_logic::ChartData::Keymode::K10 ||
                keymode == gameplay_logic::ChartData::Keymode::K14) {
                flipPlayfields(visibleNotes);
                dpOptions = resource_managers::DpOptions::Flip;
            }
            break;
        default:
            throw std::runtime_error(
              "Replay used an unsupported double option");
    }

    auto shuffle = support::ShuffleResult{};
    auto shuffleP2 = support::ShuffleResult{};
    if (keymode == gameplay_logic::ChartData::Keymode::K10 ||
        keymode == gameplay_logic::ChartData::Keymode::K14) {
        auto notes1 = std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
        auto notes2 = std::span{ visibleNotes.data() + visibleNotes.size() / 2,
                                 visibleNotes.size() / 2 };
        const auto is5k = keymode == gameplay_logic::ChartData::Keymode::K10;
        shuffle =
          applyImportedOrder(notes1, *algorithm, replay.randomOptionSeed, is5k);
        shuffleP2 = applyImportedOrder(
          notes2, *algorithmP2, replay.randomOptionSeedP2, is5k);
    } else {
        auto notesSpan =
          std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
        const auto is5k = keymode == gameplay_logic::ChartData::Keymode::K5;
        shuffle = applyImportedOrder(
          notesSpan, *algorithm, replay.randomOptionSeed, is5k);
    }

    const auto rank = magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(
                        chartData->getRank())
                        .value_or(gameplay_logic::rules::defaultBmsRank);
    auto gaugesRaw = gameplay_logic::rules::Lr2Gauge::getGauges(
      chartData->getTotal(),
      (chartData->getNormalNoteCount() + chartData->getLnCount() +
       chartData->getBssCount() + chartData->getScratchCount()));
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
      (chartData->getNormalNoteCount() + chartData->getLnCount() +
       chartData->getBssCount() + chartData->getScratchCount()),
      gameplay_logic::rules::lr2_hit_values::getLr2HitValue(
        0ns, gameplay_logic::Judgement::Perfect),
      gauges,
      chartData->getRandomSequence(),
      *algorithm,
      (keymode == gameplay_logic::ChartData::Keymode::K10 ||
       keymode == gameplay_logic::ChartData::Keymode::K14)
        ? *algorithmP2
        : resource_managers::NoteOrderAlgorithm::Normal,
      dpOptions,
      shuffle.columns + shuffleP2.columns,
      shuffle.seed,
      chartData->getLength(),
      chartData->getSha256(),
      chartData->getMd5(),
      keymode,
      replay.date,
      guid);

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
        const auto mapped = mapReplayKeycode(keymode, event.keycode);
        if (!mapped) {
            throw std::runtime_error(
              "Replay contained an unsupported keycode for this chart");
        }
        const auto offset = std::chrono::nanoseconds(event.offsetFromStartNs);
        referee.update(offset, false);
        if (event.pressed) {
            referee.passPressed(offset, *mapped);
        } else {
            referee.passReleased(offset, *mapped);
        }
    }

    const auto endOffset =
      std::chrono::nanoseconds(chartData->getLength()) + 10s;
    referee.update(endOffset, true);
    for (int i = static_cast<int>(input::BmsKey::Col11);
         i <= static_cast<int>(input::BmsKey::Col2sDown);
         ++i) {
        referee.passReleased(endOffset, static_cast<input::BmsKey>(i));
    }

    return std::make_unique<gameplay_logic::BmsScore>(
      liveScore->getResult(),
      liveScore->getReplayData(),
      liveScore->getGaugeHistory());
}

} // namespace

ReplayImportOperation*
startBeatorajaReplayImport(resource_managers::Profile* profile,
                           const QString& folderPath)
{
    // This function is always called from a background thread
    // (Profile::importPool).

    auto resolvedFolderPath = folderPath;
    const auto folderUrl = QUrl(folderPath);
    if (folderUrl.isValid() && folderUrl.isLocalFile()) {
        resolvedFolderPath = folderUrl.toLocalFile();
    }

    // Enumerate replay files on this background thread.
    auto replayFiles = QStringList{};
    const auto dirExists = QDir(resolvedFolderPath).exists();
    if (dirExists) {
        auto iterator = QDirIterator(resolvedFolderPath,
                                     { QStringLiteral("*.brd") },
                                     QDir::Files,
                                     QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            replayFiles.append(iterator.next());
        }
    }

    // Create and publish the operation on the main thread, then block until
    // done.
    ReplayImportOperation* op = nullptr;
    QMetaObject::invokeMethod(
      profile,
      [profile, count = replayFiles.size(), &op]() {
          op = profile->beginImportOp(count);
      },
      Qt::BlockingQueuedConnection);

    if (op == nullptr) {
        return nullptr;
    }

    if (!dirExists) {
        QMetaObject::invokeMethod(op, [op] {
            op->reportError(QStringLiteral("Folder does not exist"));
        });
        return op;
    }

    // Load every existing (sha256, unix_timestamp) pair in one query so we
    // can do duplicate detection with an in-memory lookup instead of one
    // round-trip per file.
    auto existingStmt = profile->getDb().createStatement(
      "SELECT sha256, unix_timestamp FROM score;");
    const auto existingRows =
      existingStmt.executeAndGetAll<std::tuple<std::string, int64_t>>();
    auto existingScores = QSet<QPair<QString, qint64>>{};
    existingScores.reserve(static_cast<qsizetype>(existingRows.size()));
    for (const auto& [sha256, ts] : existingRows) {
        existingScores.insert({ QString::fromStdString(sha256), ts });
    }

    // Wrap all writes in a single transaction – SQLite auto-commits every
    // INSERT otherwise, which dominates runtime for large imports.
    profile->getDb().execute("BEGIN;");
    bool committed = false;
    const auto rollbackGuard = qScopeGuard([&] {
        if (!committed) {
            try {
                profile->getDb().execute("ROLLBACK;");
            } catch (...) {
            }
        }
    });

    for (const auto& replayPath : replayFiles) {
        try {
            auto replay = parseReplayPayload(replayPath);

            if (existingScores.contains({ replay.sha256, replay.date })) {
                QMetaObject::invokeMethod(op, [op] { op->incrementSkipped(); });
                continue;
            }

            const auto guid = QUuid::createUuid().toString();
            auto score =
              createScoreFromReplay(*profile, std::move(replay), guid);
            score->save(profile->getDb());
            // Track so a duplicate file later in the same batch is also
            // skipped.
            existingScores.insert({ replay.sha256, replay.date });

            QMetaObject::invokeMethod(op, [op] { op->incrementImported(); });
        } catch (const std::exception& e) {
            const auto msg = QStringLiteral("%1: %2").arg(
              replayPath, QString::fromUtf8(e.what()));
            spdlog::warn("Failed to import beatoraja replay {}: {}",
                         replayPath.toStdString(),
                         e.what());
            QMetaObject::invokeMethod(op, [op, msg] {
                op->reportError(msg);
                op->incrementErrored();
            });
        }
    }

    profile->getDb().execute("COMMIT;");
    committed = true;

    return op;
}

} // namespace qml_components
