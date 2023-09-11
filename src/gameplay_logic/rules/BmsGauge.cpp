//
// Created by bobini on 10.09.23.
//

#include "BmsGauge.h"

namespace gameplay_logic::rules {
auto
BmsGauge::getGauge() const -> double
{
    return gaugeHistoryList.back().getGauge();
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
    addGaugeHistoryEntry(std::chrono::nanoseconds(0), initialValue);
}
auto
BmsGauge::getGaugeHistory() const -> QVariantList
{
    return gaugeHistory;
}
void
BmsGauge::addGaugeHistoryEntry(std::chrono::nanoseconds offsetFromStart,
                               double gauge)
{
    gaugeHistoryList.push_back({ offsetFromStart.count(), gauge });
    gaugeHistory.append(QVariant::fromValue(gaugeHistoryList.back()));
    emit gaugeChanged();
}
auto
BmsGauge::getGaugeHistoryVector() const -> const std::vector<GaugeHistoryEntry>&
{
    return gaugeHistoryList;
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
} // namespace gameplay_logic::rules
