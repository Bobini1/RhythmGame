//
// Created by bobini on 30.09.23.
//

#include "BmsGaugeHistory.h"
#include "support/Compress.h"
#include <QVariant>
#include <QIODevice>
#include <utility>
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
} // namespace gameplay_logic