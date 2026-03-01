//
// Created by bobini on 30.09.23.
//

#include "BmsGaugeHistory.h"
#include "support/Compress.h"
#include <QVariant>
#include <QIODevice>
#include <utility>
#include <QJsonArray>
#include <QJsonObject>
namespace gameplay_logic {
BmsGaugeHistory::BmsGaugeHistory(QList<BmsGaugeInfo> gaugeInfo,
                                 QString guid,
                                 QObject* parent)
  : QObject(parent)
  , gaugeInfo(std::move(gaugeInfo))
  , guid(std::move(guid))
{
}

auto
operator<<(QDataStream& stream, const BmsGaugeInfo& gaugeInfo) -> QDataStream&
{
    return stream << gaugeInfo.maxGauge << gaugeInfo.threshold << gaugeInfo.name
                  << gaugeInfo.courseGauge << gaugeInfo.gaugeHistory;
}
auto
operator>>(QDataStream& stream, BmsGaugeInfo& gaugeInfo) -> QDataStream&
{
    return stream >> gaugeInfo.maxGauge >> gaugeInfo.threshold >>
           gaugeInfo.name >> gaugeInfo.courseGauge >> gaugeInfo.gaugeHistory;
}

void
BmsGaugeHistory::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement = db.createStatement(
      "INSERT OR IGNORE INTO gauge_history (score_guid, gauge_info) "
      "VALUES (?, ?)");
    auto compressedInfo = support::compress(gaugeInfo);
    statement.bind(1, guid.toStdString());
    statement.bind(2, compressedInfo.data(), compressedInfo.size());
    statement.execute();
}
auto
BmsGaugeHistory::load(const DTO& dto) -> std::unique_ptr<BmsGaugeHistory>
{
    const auto infoBuffer = QByteArray::fromStdString(dto.gaugeInfo);
    decltype(gaugeInfo) gaugeInfo;
    support::decompress(infoBuffer, gaugeInfo);
    return std::make_unique<BmsGaugeHistory>(
      std::move(gaugeInfo), QString::fromStdString(dto.scoreGuid));
}
auto
BmsGaugeHistory::getGaugeInfo() const -> QList<BmsGaugeInfo>
{
    return gaugeInfo;
}
auto
BmsGaugeHistory::getGuid() const -> QString
{
    return guid;
}
auto
BmsGaugeHistory::toJsonArray() const -> QJsonArray
{
    QJsonArray arr;
    for (const auto& gi : gaugeInfo) {
        QJsonObject o;
        o["name"] = gi.name;
        o["maxGauge"] = gi.maxGauge;
        o["threshold"] = gi.threshold;
        o["courseGauge"] = gi.courseGauge;
        QJsonArray historyArr;
        for (const auto& he : gi.gaugeHistory) {
            QJsonObject h;
            h["offsetFromStart"] = static_cast<qint64>(he.getOffsetFromStart());
            h["gauge"] = he.getGauge();
            historyArr.append(h);
        }
        o["gaugeHistory"] = historyArr;
        arr.append(o);
    }
    return arr;
}
auto
BmsGaugeHistory::fromJsonArray(const QJsonArray& array) -> QList<BmsGaugeInfo>
{
    QList<BmsGaugeInfo> gaugeInfo;
    for (const auto& v : array) {
        const auto o = v.toObject();
        BmsGaugeInfo gi;
        gi.name = o["name"].toString();
        gi.maxGauge = o["maxGauge"].toDouble();
        gi.threshold = o["threshold"].toDouble();
        gi.courseGauge = o["courseGauge"].toBool();
        const auto historyArr = o["gaugeHistory"].toArray();
        for (const auto& hv : historyArr) {
            const auto ho = hv.toObject();
            auto he = rules::GaugeHistoryEntry{
                ho["offsetFromStart"].toVariant().toLongLong(),
                ho["gauge"].toDouble()
            };
            gi.gaugeHistory.append(he);
        }
        gaugeInfo.append(gi);
    }
    return gaugeInfo;
}
} // namespace gameplay_logic