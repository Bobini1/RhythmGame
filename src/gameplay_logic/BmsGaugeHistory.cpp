//
// Created by bobini on 30.09.23.
//

#include "BmsGaugeHistory.h"
#include "support/Compress.h"
#include <QVariant>
#include <QIODevice>
#include <utility>
namespace gameplay_logic {
BmsGaugeHistory::BmsGaugeHistory(QVariantMap gaugeHistory,
                                 QVariantMap gaugeInfo,
                                 QObject* parent)
  : QObject(parent)
  , gaugeHistory(std::move(gaugeHistory))
  , gaugeInfo(std::move(gaugeInfo))
{
}
auto
BmsGaugeHistory::getGaugeHistory() const -> QVariantMap
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
    auto buffer = QByteArray{};
    auto stream = QDataStream{ &buffer, QIODevice::WriteOnly };
    stream << *this;
    auto compressed = support::compress(buffer);
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
    auto decompressed = support::decompress(buffer);
    auto stream = QDataStream{ &decompressed, QIODevice::ReadOnly };
    auto gaugeHistory = std::make_unique<BmsGaugeHistory>();
    stream >> *gaugeHistory;
    return gaugeHistory;
}
auto
BmsGaugeHistory::getGaugeInfo() const -> QVariantMap
{
    return gaugeInfo;
}
} // namespace gameplay_logic