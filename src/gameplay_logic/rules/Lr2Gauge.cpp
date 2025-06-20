//
// Created by bobini on 10.09.23.
//

#include "Lr2Gauge.h"

#include <utility>
void
gameplay_logic::rules::Lr2Gauge::addHit(
  std::chrono::nanoseconds offsetFromStart,
  std::chrono::nanoseconds hitOffset,
  Judgement judgement)
{
    auto currentGauge = getGauge();
    if (permanentDeath &&
        (currentGauge == 0 || currentGauge < getThreshold())) {
        return;
    }
    const auto judgementValue = judgementValueFactory(currentGauge, judgement);
    auto newGauge =
      std::clamp(currentGauge + judgementValue, 0.0, getGaugeMax());
    if (newGauge != currentGauge) {
        addGaugeHistoryEntry({ offsetFromStart.count(), newGauge });
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
gameplay_logic::rules::Lr2Gauge::getGauges(TimingWindows timingWindows,
                                           double total,
                                           int noteCount)
  -> std::vector<std::unique_ptr<BmsGauge>>
{
    auto totalRatio = total / noteCount;

    auto gauges = std::vector<std::unique_ptr<BmsGauge>>();
    auto fcGauge = std::make_unique<Lr2Gauge>(
      timingWindows,
      100,
      100,
      0,
      true,
      [](double currentGauge, Judgement judgement) {
          switch (judgement) {
              case Judgement::Perfect:
              case Judgement::Great:
              case Judgement::Good:
              case Judgement::EmptyPoor:
              case Judgement::MineAvoided:
              case Judgement::LnBeginHit:
                  return 0.0;
              default:
                  return -std::numeric_limits<double>::infinity();
          }
      });
    fcGauge->setObjectName("FC");
    gauges.push_back(std::move(fcGauge));

    auto exhardGauge =
      std::make_unique<Lr2Gauge>(timingWindows,
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
                                         default:
                                             return 0.0;
                                     }
                                 });
    exhardGauge->setObjectName("EXHARD");
    gauges.push_back(std::move(exhardGauge));

    auto hardGauge = std::make_unique<Lr2Gauge>(
      timingWindows,
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
              default:
                  return 0.0;
          }
      });
    hardGauge->setObjectName("HARD");
    gauges.push_back(std::move(hardGauge));

    auto normalGauge = std::make_unique<Lr2Gauge>(
      timingWindows,
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
              case Judgement::LnBeginHit:
                  return 0.0;
          }
          throw std::runtime_error("Invalid judgement");
      });
    normalGauge->setObjectName("NORMAL");
    gauges.push_back(std::move(normalGauge));

    auto easyGauge = std::make_unique<Lr2Gauge>(
      timingWindows,
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
              case Judgement::LnBeginHit:
                  return 0.0;
          }
          throw std::runtime_error("Invalid judgement");
      });
    easyGauge->setObjectName("EASY");
    gauges.push_back(std::move(easyGauge));

    auto aeasyGauge = std::make_unique<Lr2Gauge>(
      timingWindows,
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
                case Judgement::LnBeginHit:
                  return 0.0;
          }
          throw std::runtime_error("Invalid judgement");
      });
    aeasyGauge->setObjectName("AEASY");
    gauges.push_back(std::move(aeasyGauge));

    return gauges;
}
void
gameplay_logic::rules::Lr2Gauge::addMineHit(
  std::chrono::nanoseconds offsetFromStart,
  double penalty)
{
    auto currentGauge = getGauge();
    if (permanentDeath &&
        (currentGauge == 0 || currentGauge < getThreshold())) {
        return;
    }
    auto newGauge =
      std::clamp(currentGauge + penalty * 100, 0.0, getGaugeMax());
    if (newGauge != currentGauge) {
        addGaugeHistoryEntry({ offsetFromStart.count(), newGauge });
    }
}