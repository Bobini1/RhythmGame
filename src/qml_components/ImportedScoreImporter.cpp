#include "ImportedScoreImporter.h"

#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/Judgement.h"
#include "resource_managers/Vars.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QUrl>
#include <SQLiteCpp/SQLiteCpp.h>
#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace qml_components {
namespace {

using Source = gameplay_logic::BmsScore::Source;
using LongNoteMode = gameplay_logic::BmsScore::LongNoteMode;

struct ChartMetadata
{
    std::string sha256;
    std::string md5;
    int normalNoteCount{};
    int scratchCount{};
    int lnCount{};
    int bssCount{};
    int mineCount{};
    int64_t length{};
    int keymode{};
};

struct ImportedResult
{
    QString sourceKey;
    QString sha256;
    QString md5;
    QString clearType;
    int perfect{};
    int great{};
    int good{};
    int bad{};
    int poor{};
    int emptyPoor{};
    int maxHits{};
    int points{};
    int maxCombo{};
    int64_t timestamp{};
    LongNoteMode longNoteMode{ LongNoteMode::Ln };
};

auto
localPath(const QString& filePath) -> QString
{
    const auto url = QUrl(filePath);
    return url.isLocalFile() ? url.toLocalFile() : filePath;
}

auto
hasColumn(SQLite::Database& database, const char* table, const char* column)
  -> bool
{
    auto statement = SQLite::Statement(
      database, "SELECT name FROM pragma_table_info(?) WHERE name = ?;");
    statement.bind(1, table);
    statement.bind(2, column);
    return statement.executeStep();
}

auto
findChart(db::SqliteCppDb& database, const QString& sha256, const QString& md5)
  -> std::optional<ChartMetadata>
{
    auto statement = database.createStatement(
      "SELECT sha256, md5, normal_note_count, scratch_count, ln_count, "
      "bss_count, mine_count, length, keymode FROM song_db.charts "
      "WHERE (? <> '' AND UPPER(sha256) = UPPER(?)) "
      "OR (? <> '' AND UPPER(md5) = UPPER(?)) LIMIT 1;");
    statement.bind(1, sha256.toStdString());
    statement.bind(2, sha256.toStdString());
    statement.bind(3, md5.toStdString());
    statement.bind(4, md5.toStdString());
    return statement.executeAndGet<ChartMetadata>();
}

auto
sourceGuid(Source source, const QString& sourceKey) -> QString
{
    static const auto namespaceId =
      QUuid(QStringLiteral("{97e2618d-8924-4fc6-a4a0-b09bb7f662ef}"));
    const auto name = QString::number(static_cast<int>(source)) +
                      QStringLiteral(":") + sourceKey;
    return QUuid::createUuidV5(namespaceId, name.toUtf8())
      .toString(QUuid::WithoutBraces);
}

auto
clearTypeForLr2(int clear) -> QString
{
    switch (clear) {
        case 1:
            return QStringLiteral("FAILED");
        case 2:
            return QStringLiteral("EASY");
        case 3:
            return QStringLiteral("NORMAL");
        case 4:
            return QStringLiteral("HARD");
        case 5:
            return QStringLiteral("FC");
        default:
            return QStringLiteral("NOPLAY");
    }
}

auto
clearTypeForBeatoraja(int clear) -> QString
{
    switch (clear) {
        case 1:
        case 3: // Light assist is below assist easy and is not a clear here.
            return QStringLiteral("FAILED");
        case 2:
            return QStringLiteral("AEASY");
        case 4:
            return QStringLiteral("EASY");
        case 5:
            return QStringLiteral("NORMAL");
        case 6:
            return QStringLiteral("HARD");
        case 7:
            return QStringLiteral("EXHARD");
        case 8:
            return QStringLiteral("FC");
        case 9:
            return QStringLiteral("PERFECT");
        case 10:
            return QStringLiteral("MAX");
        default:
            return QStringLiteral("NOPLAY");
    }
}

auto
longNoteModeForBeatoraja(int mode) -> LongNoteMode
{
    switch (mode) {
        case 0:
            return LongNoteMode::Ln;
        case 1:
            return LongNoteMode::Cn;
        case 2:
            return LongNoteMode::Hcn;
        default:
            throw std::runtime_error("Unsupported beatoraja long-note mode");
    }
}

auto
clearTypeForBokutachi(int lamp,
                      int perfect,
                      int great,
                      int good,
                      int bad,
                      int poor) -> QString
{
    if (lamp == 7) {
        if (great == 0 && good == 0 && bad == 0 && poor == 0)
            return QStringLiteral("MAX");
        if (good == 0 && bad == 0 && poor == 0)
            return QStringLiteral("PERFECT");
        return QStringLiteral("FC");
    }
    switch (lamp) {
        case 1:
            return QStringLiteral("FAILED");
        case 2:
            return QStringLiteral("AEASY");
        case 3:
            return QStringLiteral("EASY");
        case 4:
            return QStringLiteral("NORMAL");
        case 5:
            return QStringLiteral("HARD");
        case 6:
            return QStringLiteral("EXHARD");
        default:
            Q_UNUSED(perfect);
            return QStringLiteral("NOPLAY");
    }
}

void
saveImportedResult(db::SqliteCppDb& database,
                   Source source,
                   const ImportedResult& imported)
{
    const auto chart = findChart(database, imported.sha256, imported.md5);
    if (!chart)
        throw std::out_of_range(
          "Chart hash was not found in the song database");

    auto judgements =
      QList<int>(magic_enum::enum_count<gameplay_logic::Judgement>());
    judgements[static_cast<int>(gameplay_logic::Judgement::Poor)] =
      imported.poor;
    judgements[static_cast<int>(gameplay_logic::Judgement::EmptyPoor)] =
      imported.emptyPoor;
    judgements[static_cast<int>(gameplay_logic::Judgement::Bad)] = imported.bad;
    judgements[static_cast<int>(gameplay_logic::Judgement::Good)] =
      imported.good;
    judgements[static_cast<int>(gameplay_logic::Judgement::Great)] =
      imported.great;
    judgements[static_cast<int>(gameplay_logic::Judgement::Perfect)] =
      imported.perfect;

    const auto chartHits = chart->normalNoteCount + chart->scratchCount +
                           chart->lnCount + chart->bssCount;
    const auto maxHits = imported.maxHits > 0 ? imported.maxHits : chartHits;
    auto result = std::make_unique<gameplay_logic::BmsResult>(
      maxHits * 2.0,
      maxHits,
      chart->normalNoteCount,
      chart->scratchCount,
      chart->lnCount,
      chart->bssCount,
      chart->mineCount,
      imported.clearType,
      std::move(judgements),
      0,
      imported.points,
      imported.maxCombo,
      imported.timestamp,
      chart->length,
      QList<qint64>{},
      0,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      static_cast<gameplay_logic::ChartData::Keymode>(chart->keymode),
      sourceGuid(source, imported.sourceKey),
      QString::fromStdString(chart->sha256),
      QString::fromStdString(chart->md5));
    gameplay_logic::BmsScore::fromImportedResult(
      std::move(result), source, imported.longNoteMode)
      ->save(database);
}

void
importLr2(SQLite::Database& sourceDb,
          db::SqliteCppDb& targetDb,
          int64_t fallbackTimestamp,
          const ScoreImportCallbacks& callbacks)
{
    auto count = SQLite::Statement(sourceDb, "SELECT COUNT(*) FROM score;");
    count.executeStep();
    callbacks.started(count.getColumn(0).getInt());

    auto query = SQLite::Statement(
      sourceDb,
      "SELECT hash, clear, perfect, great, good, bad, poor, totalnotes, "
      "maxcombo FROM score;");
    while (query.executeStep()) {
        try {
            const auto hash = QString::fromUtf8(query.getColumn(0).getText());
            auto imported = ImportedResult{
                .sourceKey = hash.toUpper(),
                .md5 = hash,
                .clearType = clearTypeForLr2(query.getColumn(1).getInt()),
                .perfect = query.getColumn(2).getInt(),
                .great = query.getColumn(3).getInt(),
                .good = query.getColumn(4).getInt(),
                .bad = query.getColumn(5).getInt(),
                .poor = query.getColumn(6).getInt(),
                .maxHits = query.getColumn(7).getInt(),
                .points =
                  query.getColumn(2).getInt() * 2 + query.getColumn(3).getInt(),
                .maxCombo = query.getColumn(8).getInt(),
                .timestamp = fallbackTimestamp,
            };
            saveImportedResult(targetDb, Source::Lr2, imported);
            callbacks.imported();
        } catch (const std::out_of_range&) {
            callbacks.skipped();
        } catch (const std::exception& error) {
            callbacks.failed(QString::fromUtf8(error.what()));
        }
    }
}

void
importBeatoraja(SQLite::Database& sourceDb,
                db::SqliteCppDb& targetDb,
                const ScoreImportCallbacks& callbacks)
{
    auto count = SQLite::Statement(
      sourceDb,
      "SELECT COUNT(*) FROM (SELECT 1 FROM score GROUP BY sha256, mode);");
    count.executeStep();
    callbacks.started(count.getColumn(0).getInt());

    auto query = SQLite::Statement(
      sourceDb,
      "SELECT sha256, mode, clear, epg + lpg, egr + lgr, egd + lgd, "
      "ebd + lbd, epr + lpr, ems + lms, notes, combo, date FROM (SELECT *, "
      "ROW_NUMBER() "
      "OVER "
      "(PARTITION BY sha256, mode ORDER BY (epg + lpg) * 2 + egr + lgr DESC, "
      "clear DESC, date DESC) AS choice FROM score) WHERE choice = 1;");
    while (query.executeStep()) {
        try {
            const auto hash = QString::fromUtf8(query.getColumn(0).getText());
            // Course records concatenate their charts' SHA-256 hashes and
            // encode course options in mode. They are not single-chart scores.
            if (hash.size() > 64 && hash.size() % 64 == 0) {
                callbacks.skipped();
                continue;
            }
            const auto mode = query.getColumn(1).getInt();
            const auto longNoteMode = longNoteModeForBeatoraja(mode);
            const auto perfect = query.getColumn(3).getInt();
            const auto great = query.getColumn(4).getInt();
            auto imported = ImportedResult{
                .sourceKey =
                  hash.toUpper() +
                  (mode == 0 ? QString{}
                             : QStringLiteral(":") + QString::number(mode)),
                .sha256 = hash,
                .clearType = clearTypeForBeatoraja(query.getColumn(2).getInt()),
                .perfect = perfect,
                .great = great,
                .good = query.getColumn(5).getInt(),
                .bad = query.getColumn(6).getInt(),
                .poor = query.getColumn(7).getInt(),
                .emptyPoor = query.getColumn(8).getInt(),
                .maxHits = query.getColumn(9).getInt(),
                .points = perfect * 2 + great,
                .maxCombo = query.getColumn(10).getInt(),
                .timestamp = query.getColumn(11).getInt64(),
                .longNoteMode = longNoteMode,
            };
            saveImportedResult(targetDb, Source::Beatoraja, imported);
            callbacks.imported();
        } catch (const std::out_of_range&) {
            callbacks.skipped();
        } catch (const std::exception& error) {
            callbacks.failed(QString::fromUtf8(error.what()));
        }
    }
}

} // namespace

void
importLocalScoreDatabase(db::SqliteCppDb& targetDb,
                         const QString& filePath,
                         const ScoreImportCallbacks& callbacks)
{
    const auto path = localPath(filePath);
    auto sourceDb = SQLite::Database(path.toStdString(),
                                     SQLite::OPEN_READONLY |
                                       SQLite::OPEN_FULLMUTEX); // NOLINT
    if (!sourceDb.tableExists("score"))
        throw std::runtime_error("The selected database has no score table");

    if (hasColumn(sourceDb, "score", "hash") &&
        hasColumn(sourceDb, "score", "perfect")) {
        const auto timestamp =
          QFileInfo(path).lastModified().toSecsSinceEpoch();
        importLr2(sourceDb, targetDb, timestamp, callbacks);
        return;
    }
    if (hasColumn(sourceDb, "score", "sha256") &&
        hasColumn(sourceDb, "score", "epg")) {
        importBeatoraja(sourceDb, targetDb, callbacks);
        return;
    }
    throw std::runtime_error(
      "The selected database is not an LR2 or beatoraja score database");
}

auto
importBokutachiPersonalBests(db::SqliteCppDb& targetDb,
                             const QByteArray& response) -> int
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(response, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("Invalid Bokutachi score response");

    const auto body =
      document.object().value(QStringLiteral("body")).toObject();
    const auto charts = body.value(QStringLiteral("charts")).toArray();
    auto hashes = QHash<QString, QPair<QString, QString>>{};
    for (const auto& value : charts) {
        const auto chart = value.toObject();
        const auto data = chart.value(QStringLiteral("data")).toObject();
        hashes.insert(chart.value(QStringLiteral("chartID")).toString(),
                      { data.value(QStringLiteral("hashSHA256")).toString(),
                        data.value(QStringLiteral("hashMD5")).toString() });
    }

    auto importedCount = 0;
    for (const auto& value : body.value(QStringLiteral("pbs")).toArray()) {
        const auto pb = value.toObject();
        const auto chartId = pb.value(QStringLiteral("chartID")).toString();
        const auto hash = hashes.value(chartId);
        if (chartId.isEmpty() ||
            (hash.first.isEmpty() && hash.second.isEmpty()))
            continue;

        const auto scoreData = pb.value(QStringLiteral("scoreData")).toObject();
        const auto judgements =
          scoreData.value(QStringLiteral("judgements")).toObject();
        const auto optional =
          scoreData.value(QStringLiteral("optional")).toObject();
        const auto enumIndexes =
          scoreData.value(QStringLiteral("enumIndexes")).toObject();
        const auto perfect = judgements.value(QStringLiteral("pgreat")).toInt();
        const auto great = judgements.value(QStringLiteral("great")).toInt();
        const auto good = judgements.value(QStringLiteral("good")).toInt();
        const auto bad = judgements.value(QStringLiteral("bad")).toInt();
        const auto poor = judgements.value(QStringLiteral("poor")).toInt();
        const auto lamp = enumIndexes.value(QStringLiteral("lamp")).toInt();
        const auto timeValue = pb.value(QStringLiteral("timeAchieved"));
        const auto timestamp =
          timeValue.isDouble() ? timeValue.toInteger() / 1000 : int64_t{};

        auto imported = ImportedResult{
            .sourceKey = pb.value(QStringLiteral("game")).toString() +
                         QStringLiteral(":") + chartId,
            .sha256 = hash.first,
            .md5 = hash.second,
            .clearType =
              clearTypeForBokutachi(lamp, perfect, great, good, bad, poor),
            .perfect = perfect,
            .great = great,
            .good = good,
            .bad = bad,
            .poor = poor,
            .points = scoreData.value(QStringLiteral("score")).toInt(),
            .maxCombo = optional.value(QStringLiteral("maxCombo")).toInt(),
            .timestamp = timestamp,
        };
        try {
            saveImportedResult(targetDb, Source::Bokutachi, imported);
            ++importedCount;
        } catch (const std::out_of_range&) {
            // Bokutachi may contain charts that are not installed locally.
        }
    }
    return importedCount;
}

} // namespace qml_components
