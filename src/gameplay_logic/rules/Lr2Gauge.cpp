//
// Created by bobini on 10.09.23.
//

#include "Lr2Gauge.h"

#include <utility>
void
gameplay_logic::rules::Lr2Gauge::addHit(
  std::chrono::nanoseconds offsetFromStart,
  std::chrono::nanoseconds hitOffset)
{
    auto currentGauge = getGauge();
    if (permanentDeath &&
        (currentGauge == 0 || currentGauge < getThreshold())) {
        return;
    }
    auto judgement = timingWindows.find(hitOffset);
    auto judgementValue = judgementValueFactory(
      currentGauge,
      judgement == timingWindows.end() ? Judgement::Poor : judgement->second);
    auto newGauge =
      std::clamp(currentGauge + judgementValue, 0.0, getGaugeMax());
    if (newGauge != currentGauge) {
        addGaugeHistoryEntry(offsetFromStart, newGauge);
    }
}
gameplay_logic::rules::Lr2Gauge::Lr2Gauge(
  gameplay_logic::rules::TimingWindows timingWindows,
  double gaugeMax,
  double initialValue,
  double threshold,
  bool permanentDeath,
  std::function<double(double, Judgement)> judgementValueFactory,
  QObject* parent)
  : BmsGauge(gaugeMax, initialValue, threshold, parent)
  , timingWindows(std::move(timingWindows))
  , permanentDeath(permanentDeath)
  , judgementValueFactory(std::move(judgementValueFactory))
{
}
auto
gameplay_logic::rules::Lr2Gauge::getGauges(
  gameplay_logic::rules::TimingWindows timingWindows,
  double total,
  int noteCount) -> QList<BmsGauge*>
{
    auto totalRatio = total / noteCount;

    auto gauges = QList<BmsGauge*>();
    auto* fcGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   100,
                   0,
                   true,
                   [](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                           case Judgement::Great:
                           case Judgement::EmptyPoor:
                               return 0.0;
                           default:
                               return -std::numeric_limits<double>::infinity();
                       }
                   });
    fcGauge->setObjectName("FC");
    gauges.append(fcGauge);

    auto* exhardGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   100,
                   0,
                   true,
                   [](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                               return 0.15;
                           case Judgement::Great:
                               return 0.06;
                           case Judgement::Good:
                               return 0.0;
                           case Judgement::Bad:
                               return -8.0;
                           case Judgement::Poor:
                               return -16.0;
                           case Judgement::EmptyPoor:
                               return -8.0;
                       }
                       throw std::runtime_error("Invalid judgement");
                   });
    exhardGauge->setObjectName("EXHARD");
    gauges.append(exhardGauge);

    auto* hardGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   100,
                   0,
                   true,
                   [](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                               return 0.1;
                           case Judgement::Great:
                               return 0.1;
                           case Judgement::Good:
                               return 0.05;
                           case Judgement::Bad:
                               return (currentGauge > 30) ? -6.0 : -3.6;
                           case Judgement::Poor:
                               return (currentGauge > 30) ? -10.0 : -6.0;
                           case Judgement::EmptyPoor:
                               return (currentGauge > 30) ? -2.0 : -1.2;
                       }
                       throw std::runtime_error("Invalid judgement");
                   });
    hardGauge->setObjectName("HARD");
    gauges.append(hardGauge);

    auto* normalGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   20,
                   80,
                   false,
                   [totalRatio](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                               return totalRatio;
                           case Judgement::Great:
                               return totalRatio;
                           case Judgement::Good:
                               return totalRatio * 0.5;
                           case Judgement::Bad:
                               return -4.0;
                           case Judgement::Poor:
                               return -6.0;
                           case Judgement::EmptyPoor:
                               return -2.0;
                       }
                       throw std::runtime_error("Invalid judgement");
                   });
    normalGauge->setObjectName("NORMAL");
    gauges.append(normalGauge);

    auto* easyGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   20,
                   80,
                   false,
                   [totalRatio](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                               return totalRatio * 1.2;
                           case Judgement::Great:
                               return totalRatio * 1.2;
                           case Judgement::Good:
                               return totalRatio * 0.6;
                           case Judgement::Bad:
                               return -3.2;
                           case Judgement::Poor:
                               return -4.8;
                           case Judgement::EmptyPoor:
                               return -1.6;
                       }
                       throw std::runtime_error("Invalid judgement");
                   });
    easyGauge->setObjectName("EASY");
    gauges.append(easyGauge);

    auto* aeasyGauge =
      new Lr2Gauge(timingWindows,
                   100,
                   20,
                   60,
                   false,
                   [totalRatio](double currentGauge, Judgement judgement) {
                       switch (judgement) {
                           case Judgement::Perfect:
                               return totalRatio;
                           case Judgement::Great:
                               return totalRatio;
                           case Judgement::Good:
                               return totalRatio * 0.5;
                           case Judgement::Bad:
                               return -1.5;
                           case Judgement::Poor:
                               return -3.0;
                           case Judgement::EmptyPoor:
                               return -0.5;
                       }
                       throw std::runtime_error("Invalid judgement");
                   });
    aeasyGauge->setObjectName("AEASY");
    gauges.append(aeasyGauge);

    return gauges;
}
