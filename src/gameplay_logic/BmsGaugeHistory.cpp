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
  QObject* parent)
  : QObject(parent)
  , gaugeHistory(std::move(gaugeHistory))
  , gaugeInfo(std::move(gaugeInfo))
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

auto
operator<<(QDataStream& stream, const BmsGaugeHistory& gaugeHistory)
  -> QDataStream&
{
    stream << gaugeHistory.gaugeHistory << gaugeHistory.gaugeInfo;
    return stream;
}
auto
operator>>(QDataStream& stream, BmsGaugeHistory& gaugeHistory) -> QDataStream&
{
    stream >> gaugeHistory.gaugeHistory >> gaugeHistory.gaugeInfo;
    return stream;
}
void
BmsGaugeHistory::save(db::SqliteCppDb& db, int64_t scoreId)
{
    auto statement =
      db.createStatement("INSERT INTO gauge_history (score_id, gauge_history) "
                         "VALUES (?, ?)");
    auto compressed = support::compress(*this);
    statement.bind(1, scoreId);
    statement.bind(2, compressed.data(), compressed.size());
    statement.execute();
}
auto
BmsGaugeHistory::load(db::SqliteCppDb& db, int64_t scoreId)
  -> std::unique_ptr<BmsGaugeHistory>
{
    auto statement = db.createStatement(
      "SELECT gauge_history FROM gauge_history WHERE score_id = ?");
    statement.bind(1, scoreId);
    auto result = statement.executeAndGet<std::string>();
    if (!result.has_value()) {
        return nullptr;
    }
    auto buffer = QByteArray::fromStdString(*result);
    auto gaugeHistory = std::make_unique<BmsGaugeHistory>();
    support::decompress(buffer, *gaugeHistory);
    return gaugeHistory;
}
auto
BmsGaugeHistory::getGaugeInfo() const -> QHash<QString, BmsGaugeInfo>
{
    return gaugeInfo;
}
} // namespace gameplay_logic