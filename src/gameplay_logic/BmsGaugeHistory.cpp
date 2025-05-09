//
// Created by bobini on 30.09.23.
//

#include "BmsGaugeHistory.h"
#include "support/Compress.h"
#include <QVariant>
#include <QIODevice>
#include <utility>
namespace gameplay_logic {
BmsGaugeHistory::BmsGaugeHistory(
  QHash<QString, QList<rules::GaugeHistoryEntry>> gaugeHistory,
  QHash<QString, BmsGaugeInfo> gaugeInfo,
  QString guid,
  QObject* parent)
  : QObject(parent)
  , gaugeHistory(std::move(gaugeHistory))
  , gaugeInfo(std::move(gaugeInfo))
  , guid(std::move(guid))
{
}
auto
BmsGaugeHistory::getGaugeHistory() const
  -> QHash<QString, QList<rules::GaugeHistoryEntry>>
{
    return gaugeHistory;
}

auto
operator<<(QDataStream& stream, const BmsGaugeInfo& gaugeInfo) -> QDataStream&
{
    return stream << gaugeInfo.maxGauge << gaugeInfo.threshold;
}
auto
operator>>(QDataStream& stream, BmsGaugeInfo& gaugeInfo) -> QDataStream&
{
    return stream >> gaugeInfo.maxGauge >> gaugeInfo.threshold;
}

void
BmsGaugeHistory::save(db::SqliteCppDb& db)
{
    auto statement = db.createStatement(
      "INSERT OR IGNORE INTO gauge_history (guid, gauge_history, gauge_info) "
      "VALUES (?, ?, ?)");
    auto compressedHistory = support::compress(gaugeHistory);
    auto compressedInfo = support::compress(gaugeInfo);
    statement.bind(1, guid.toStdString());
    statement.bind(2, compressedHistory.data(), compressedHistory.size());
    statement.bind(3, compressedInfo.data(), compressedInfo.size());
    statement.execute();
}
auto
BmsGaugeHistory::load(const DTO& dto) -> std::unique_ptr<BmsGaugeHistory>
{
    const auto historyBuffer = QByteArray::fromStdString(dto.gaugeHistory);
    const auto infoBuffer = QByteArray::fromStdString(dto.gaugeInfo);
    decltype(gaugeHistory) gaugeHistory;
    decltype(gaugeInfo) gaugeInfo;
    support::decompress(historyBuffer, gaugeHistory);
    support::decompress(infoBuffer, gaugeInfo);
    return std::make_unique<BmsGaugeHistory>(
      std::move(gaugeHistory),
      std::move(gaugeInfo),
      QString::fromStdString(dto.scoreGuid));
}
auto
BmsGaugeHistory::getGaugeInfo() const -> QHash<QString, BmsGaugeInfo>
{
    return gaugeInfo;
}
auto
BmsGaugeHistory::getGaugeHistoryVariant() const -> QVariantMap
{
    auto ret = QVariantMap{};
    for (const auto& [key, value] : gaugeHistory.asKeyValueRange()) {
        ret[key] = QVariant::fromValue(value);
    }
    return ret;
}

auto
BmsGaugeHistory::getGaugeInfoVariant() const -> QVariantMap
{
    auto ret = QVariantMap{};
    for (const auto& [key, value] : gaugeInfo.asKeyValueRange()) {
        ret[key] = QVariant::fromValue(value);
    }
    return ret;
}
auto
BmsGaugeHistory::getGuid() const -> QString
{
    return guid;
}
} // namespace gameplay_logic