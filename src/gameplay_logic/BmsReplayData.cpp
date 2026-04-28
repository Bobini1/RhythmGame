//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"

#include <QDataStream>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonObject>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <zstd.h>

namespace gameplay_logic {
namespace {

constexpr quint32 replayBlobMagic = 0x52475032; // "RGP2"
constexpr quint8 replayBlobVersion = 1;
constexpr qsizetype timingBucketCount = 6;

struct LegacyHitEvent
{
    int64_t offsetFromStart{};
    std::optional<BmsPoints> points;
    std::optional<int> noteIndex;
    int column{};
    HitEvent::Action action{};
    bool noteRemoved{};
};

auto
operator<<(QDataStream& stream, const LegacyHitEvent& tap) -> QDataStream&
{
    auto points = tap.points.has_value()
                    ? QVariant::fromValue(tap.points.value())
                    : QVariant{};
    stream << static_cast<qint64>(tap.offsetFromStart) << points << tap.column
           << tap.noteIndex.value_or(-1) << tap.action << tap.noteRemoved;
    return stream;
}

auto
operator>>(QDataStream& stream, LegacyHitEvent& tap) -> QDataStream&
{
    qint64 offsetFromStart;
    QVariant points;
    int column;
    int noteIndex;
    HitEvent::Action action;
    bool noteRemoved;
    stream >> offsetFromStart >> points >> column >> noteIndex >> action >>
      noteRemoved;
    tap.offsetFromStart = offsetFromStart;
    tap.points =
      points.isNull() ? std::nullopt : std::optional(points.value<BmsPoints>());
    tap.noteIndex = noteIndex == -1 ? std::nullopt : std::optional(noteIndex);
    tap.column = column;
    tap.action = action;
    tap.noteRemoved = noteRemoved;
    return stream;
}

auto
compressRaw(const QByteArray& raw) -> QByteArray
{
    using namespace std::string_literals;
    auto compressed = QByteArray{};
    compressed.resize(ZSTD_compressBound(raw.size()));
    auto compressedSize = ZSTD_compress(
      compressed.data(), compressed.size(), raw.data(), raw.size(), 1);
    if (ZSTD_isError(compressedSize)) {
        throw std::runtime_error("Failed to compress replay data: "s +
                                 ZSTD_getErrorName(compressedSize));
    }
    return compressed.left(static_cast<qsizetype>(compressedSize));
}

auto
decompressRaw(const QByteArray& compressed) -> QByteArray
{
    using namespace std::string_literals;
    auto decompressedSize =
      ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (ZSTD_isError(decompressedSize)) {
        throw std::runtime_error("Failed to inspect replay data: "s +
                                 ZSTD_getErrorName(decompressedSize));
    }
    auto raw = QByteArray{};
    raw.resize(static_cast<qsizetype>(decompressedSize));
    decompressedSize = ZSTD_decompress(
      raw.data(), raw.size(), compressed.data(), compressed.size());
    if (ZSTD_isError(decompressedSize)) {
        throw std::runtime_error("Failed to decompress replay data: "s +
                                 ZSTD_getErrorName(decompressedSize));
    }
    raw.resize(static_cast<qsizetype>(decompressedSize));
    return raw;
}

auto
inferLegacyKey(int column) -> std::optional<input::BmsKey>
{
    if (column == static_cast<int>(input::BmsKey::Col1sUp) ||
        column == static_cast<int>(input::BmsKey::Col2sUp)) {
        return static_cast<input::BmsKey>(column);
    }
    if (column >= static_cast<int>(input::BmsKey::Col11) &&
        column <= static_cast<int>(input::BmsKey::Col2sUp)) {
        return static_cast<input::BmsKey>(column);
    }
    return std::nullopt;
}

auto
timingBucketForJudgement(Judgement judgement) -> int
{
    switch (judgement) {
    case Judgement::Perfect:
        return 0;
    case Judgement::Great:
        return 1;
    case Judgement::Good:
        return 2;
    case Judgement::Bad:
        return 3;
    case Judgement::Poor:
        return 4;
    case Judgement::EmptyPoor:
        return 5;
    default:
        return -1;
    }
}

auto
totalTimingCount(const QList<int>& counts) -> int
{
    auto total = 0;
    for (auto bucket = 1; bucket < counts.size(); ++bucket) {
        total += counts[bucket];
    }
    return total;
}

auto
serializeReplayData(const QList<HitEvent>& hitEvents) -> QByteArray
{
    auto raw = QByteArray{};
    auto stream = QDataStream(&raw, QIODevice::WriteOnly);
    stream << replayBlobMagic << replayBlobVersion << hitEvents;
    return compressRaw(raw);
}

auto
deserializeReplayData(const QByteArray& data) -> QList<HitEvent>
{
    auto raw = decompressRaw(data);
    auto stream = QDataStream(&raw, QIODevice::ReadOnly);
    auto magic = quint32{};
    stream >> magic;
    if (magic == replayBlobMagic) {
        auto version = quint8{};
        stream >> version;
        if (version != replayBlobVersion) {
            throw std::runtime_error("Unsupported replay blob version");
        }
        auto hitEvents = QList<HitEvent>{};
        stream >> hitEvents;
        return hitEvents;
    }

    stream.device()->seek(0);
    auto legacyEvents = QList<LegacyHitEvent>{};
    stream >> legacyEvents;
    auto hitEvents = QList<HitEvent>{};
    hitEvents.reserve(legacyEvents.size());
    for (const auto& legacy : legacyEvents) {
        hitEvents.append(HitEvent(legacy.column,
                                  inferLegacyKey(legacy.column),
                                  legacy.noteIndex,
                                  legacy.offsetFromStart,
                                  legacy.points,
                                  legacy.action,
                                  legacy.noteRemoved));
    }
    return hitEvents;
}

} // namespace

BmsReplayData::BmsReplayData(QList<HitEvent> hitEvents,
                             QString guid,
                             QObject* parent)
  : QObject(parent)
  , hitEvents(std::move(hitEvents))
  , guid(std::move(guid))
{
    updateTimingCounts();
}

auto
BmsReplayData::getHitEvents() const -> const QList<HitEvent>&
{
    return hitEvents;
}
auto
BmsReplayData::getEarlyTimingCounts() const -> QList<int>
{
    return earlyTimingCounts;
}
auto
BmsReplayData::getLateTimingCounts() const -> QList<int>
{
    return lateTimingCounts;
}
auto
BmsReplayData::getTotalEarly() const -> int
{
    return totalEarly;
}
auto
BmsReplayData::getTotalLate() const -> int
{
    return totalLate;
}
auto
BmsReplayData::getGuid() const -> QString
{
    return guid;
}
void
BmsReplayData::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement =
      db.createStatement("INSERT OR REPLACE INTO replay_data (score_guid, "
                         "replay_data) VALUES (?, ?)");
    const auto serialized = serializeReplayData(hitEvents);
    statement.bind(1, guid.toStdString());
    statement.bind(2, serialized.data(), serialized.size());
    statement.execute();
}
auto
BmsReplayData::load(const DTO& dto) -> std::unique_ptr<BmsReplayData>
{
    auto data = QByteArray::fromStdString(dto.hitEvents);
    return std::make_unique<BmsReplayData>(
      deserializeReplayData(data), QString::fromStdString(dto.guid));
}

void
BmsReplayData::migrateStoredReplayData(db::SqliteCppDb& db)
{
    auto statement = db.createStatement(
      "SELECT id, score_guid, replay_data FROM replay_data;");
    const auto rows =
      statement
        .executeAndGetAll<std::tuple<int64_t, std::string, std::string>>();
    auto update = db.createStatement(
      "UPDATE replay_data SET replay_data = ? WHERE id = ?;");
    for (const auto& [id, guid, replayData] : rows) {
        try {
            const auto hitEvents =
              deserializeReplayData(QByteArray::fromStdString(replayData));
            const auto serialized = serializeReplayData(hitEvents);
            update.reset();
            update.bind(1, serialized.data(), serialized.size());
            update.bind(2, id);
            update.execute();
        } catch (const std::exception& e) {
            spdlog::warn(
              "Failed to migrate replay data {}: {}", guid, e.what());
        }
    }
}

auto
BmsReplayData::toJsonArray() const -> QJsonArray
{
    QJsonArray arr;
    for (const auto& e : getHitEvents()) {
        QJsonObject o;
        o["offsetFromStart"] = static_cast<qint64>(e.getOffsetFromStart());
        auto pts = e.getPointsOptional();
        if (pts.has_value()) {
            QJsonObject p;
            p["value"] = pts->getValue();
            p["judgement"] = static_cast<int>(pts->getJudgement());
            p["deviation"] = static_cast<qint64>(pts->getDeviation());
            o["points"] = p;
        } else {
            o["points"] = QJsonValue();
        }
        o["column"] = e.getColumn();
        o["key"] = e.getKey();
        o["noteIndex"] = e.getNoteIndex();
        o["action"] = static_cast<int>(e.getAction());
        o["noteRemoved"] = e.getNoteRemoved();
        arr.append(o);
    }
    return arr;
}

auto
BmsReplayData::fromJsonArray(const QJsonArray& array) -> QList<HitEvent>
{
    QList<HitEvent> ret;
    for (const auto& v : array) {
        if (!v.isObject())
            continue;
        auto o = v.toObject();
        auto offset =
          static_cast<int64_t>(o["offsetFromStart"].toVariant().toLongLong());
        std::optional<BmsPoints> pts;
        if (o.contains("points") && o["points"].isObject()) {
            auto p = o["points"].toObject();
            double value = p["value"].toDouble();
            auto judgement = static_cast<Judgement>(p["judgement"].toInt());
            int64_t deviation =
              static_cast<int64_t>(p["deviation"].toVariant().toLongLong());
            pts = BmsPoints(value, judgement, deviation);
        }
        const auto column = o["column"].toInt();
        const auto key = o.contains("key") ? o["key"].toInt(-1) : -1;
        const auto noteIndex = o["noteIndex"].toInt(-1);
        const auto action = static_cast<HitEvent::Action>(o["action"].toInt());
        const auto noteRemoved = o["noteRemoved"].toBool();
        ret.append(HitEvent(
          column,
          key == -1
            ? std::optional<input::BmsKey>{}
            : std::optional<input::BmsKey>(static_cast<input::BmsKey>(key)),
          noteIndex == -1 ? std::optional<int>{} : std::optional(noteIndex),
          offset,
          pts,
          action,
          noteRemoved));
    }
    return ret;
}

void
BmsReplayData::updateTimingCounts()
{
    earlyTimingCounts = QList<int>(timingBucketCount);
    lateTimingCounts = QList<int>(timingBucketCount);
    for (const auto& hit : hitEvents) {
        const auto points = hit.getPointsOptional();
        if (!points.has_value()) {
            continue;
        }
        const auto bucket = timingBucketForJudgement(points->getJudgement());
        if (bucket < 0) {
            continue;
        }
        auto& counts =
          points->getDeviation() < 0 ? earlyTimingCounts : lateTimingCounts;
        counts[bucket] = counts[bucket] + 1;
    }
    totalEarly = totalTimingCount(earlyTimingCounts);
    totalLate = totalTimingCount(lateTimingCounts);
}
} // namespace gameplay_logic
