//
// Created by bobini on 30.09.23.
//

#include "BmsReplayData.h"
#include "support/Compress.h"
#include <QIODevice>
#include <utility>
#include <QJsonArray>
#include <QJsonObject>

namespace gameplay_logic {
BmsReplayData::BmsReplayData(QList<HitEvent> hitEvents,
                             QString guid,
                             QObject* parent)
  : QObject(parent)
  , hitEvents(std::move(hitEvents))
  , guid(std::move(guid))
{
}
auto
BmsReplayData::getHitEvents() const -> const QList<HitEvent>&
{
    return hitEvents;
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
      db.createStatement("INSERT OR IGNORE INTO replay_data (score_guid, "
                         "replay_data) VALUES (?, ?)");
    auto compressed = support::compress(hitEvents);
    statement.bind(1, guid.toStdString());
    statement.bind(2, compressed.data(), compressed.size());
    statement.execute();
}
auto
BmsReplayData::load(const DTO& dto) -> std::unique_ptr<BmsReplayData>
{
    const auto data = QByteArray::fromStdString(dto.hitEvents);
    auto hitEvents = QList<HitEvent>{};
    support::decompress(data, hitEvents);
    return std::make_unique<BmsReplayData>(std::move(hitEvents),
                                           QString::fromStdString(dto.guid));
}
auto
BmsReplayData::toJsonArray() const -> QJsonArray
{
    QJsonArray arr;
    for (const auto& e : hitEvents) {
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
        int column = o["column"].toInt();
        int noteIndex = o["noteIndex"].toInt(-1);
        auto action = static_cast<HitEvent::Action>(o["action"].toInt());
        bool noteRemoved = o["noteRemoved"].toBool();
        ret.append(HitEvent(column,
                            noteIndex == -1 ? std::optional<int>{}
                                            : std::optional(noteIndex),
                            offset,
                            pts,
                            action,
                            noteRemoved));
    }
    return ret;
}
} // namespace gameplay_logic