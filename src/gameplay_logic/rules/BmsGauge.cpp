//
// Created by bobini on 10.09.23.
//

#include "BmsGauge.h"

namespace gameplay_logic::rules {
auto
BmsGauge::getGauge() const -> double
{
    return gaugeHistory.back().getGauge();
}
auto
BmsGauge::getGaugeMax() const -> double
{
    return gaugeMax;
}
auto
BmsGauge::getThreshold() const -> double
{
    return threshold;
}
BmsGauge::BmsGauge(double gaugeMax,
                   double initialValue,
                   double threshold,
                   QObject* parent)
  : QObject(parent)
  , gaugeMax(gaugeMax)
  , threshold(threshold)
{
    // todo: pass -1 in constructor
    addGaugeHistoryEntry(
      { 0,
        initialValue });
}
auto
BmsGauge::getGaugeHistory() const -> const QList<GaugeHistoryEntry>&
{
    return gaugeHistory;
}
void
BmsGauge::addGaugeHistoryEntry(const GaugeHistoryEntry entry)
{
    auto currentGauge = gaugeHistory.empty() ? std::nan("") : gaugeHistory.back().getGauge();
    gaugeHistory.append(entry);
    if (currentGauge != entry.getGauge()) {
        emit gaugeChanged();
    }
}
GaugeHistoryEntry::GaugeHistoryEntry(int64_t offsetFromStart, double gauge)
  : offsetFromStart(offsetFromStart)
  , gauge(gauge)
{
}
auto
GaugeHistoryEntry::getOffsetFromStart() const -> int64_t
{
    return offsetFromStart;
}
auto
GaugeHistoryEntry::getGauge() const -> double
{
    return gauge;
}
auto
operator<<(QDataStream& stream, const GaugeHistoryEntry& entry) -> QDataStream&
{
    stream << static_cast<qint64>(entry.offsetFromStart) << entry.gauge;
    return stream;
}
auto
operator>>(QDataStream& stream, GaugeHistoryEntry& entry) -> QDataStream&
{
    qint64 offsetFromStart;
    double gauge;
    stream >> offsetFromStart >> gauge;
    entry.offsetFromStart = offsetFromStart;
    entry.gauge = gauge;
    return stream;
}
} // namespace gameplay_logic::rules
