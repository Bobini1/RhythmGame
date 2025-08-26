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
  QString gaugeName,
  double gaugeMax,
  double initialValue,
  double threshold,
  bool permanentDeath,
  bool courseGauge,
  std::function<double(double, Judgement)> judgementValueFactory,
  QObject* parent)
  : BmsGauge(std::move(gaugeName),
             gaugeMax,
             initialValue,
             threshold,
             courseGauge,
             parent)
  , permanentDeath(permanentDeath)
  , judgementValueFactory(std::move(judgementValueFactory))
{
}
auto
gameplay_logic::rules::Lr2Gauge::getGauges(double total, int noteCount)
  -> std::vector<std::unique_ptr<BmsGauge>>
{
    auto totalRatio = total / noteCount;

    auto gauges = std::vector<std::unique_ptr<BmsGauge>>();
    auto fcGauge = std::make_unique<Lr2Gauge>(
      "FC",
      100,
      100,
      0,
      true,
      false,
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
    gauges.push_back(std::move(fcGauge));

    auto exhardGauge =
      std::make_unique<Lr2Gauge>("EXHARD",
                                 100,
                                 100,
                                 0,
                                 true,
                                 false,
                                 [](double currentGauge, Judgement judgement) {
                                     switch (judgement) {
                                         case Judgement::Perfect:
                                             return 0.1;
                                         case Judgement::Great:
                                             return 0.1;
                                         case Judgement::Good:
                                             return 0.05;
                                         case Judgement::Bad:
                                             return (currentGauge > 30) ? -12.0 : -7.2;
                                         case Judgement::Poor:
                                             return (currentGauge > 30) ? -20.0 : -12.0;
                                         case Judgement::EmptyPoor:
                                             return (currentGauge > 30) ? -2.0 : -1.2;
                                         default:
                                             return 0.0;
                                     }
                                 });
    gauges.push_back(std::move(exhardGauge));

    auto hardGauge = std::make_unique<Lr2Gauge>(
      "HARD",
      100,
      100,
      0,
      true,
      false,
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
    gauges.push_back(std::move(hardGauge));

    auto normalGauge = std::make_unique<Lr2Gauge>(
      "NORMAL",
      100,
      20,
      80,
      false,
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
    gauges.push_back(std::move(normalGauge));

    auto easyGauge = std::make_unique<Lr2Gauge>(
      "EASY",
      100,
      20,
      80,
      false,
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
    gauges.push_back(std::move(easyGauge));

    auto aeasyGauge = std::make_unique<Lr2Gauge>(
      "AEASY",
      100,
      20,
      60,
      false,
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
    gauges.push_back(std::move(aeasyGauge));

    return gauges;
}
auto
gameplay_logic::rules::Lr2Gauge::getDanGauges(
  const QHash<QString, double>& initialValues)
  -> std::vector<std::unique_ptr<BmsGauge>>
{
    auto gauges = std::vector<std::unique_ptr<BmsGauge>>{};

    auto exhardDanGauge =
      std::make_unique<Lr2Gauge>("EXHARDDAN",
                                 100,
                                 initialValues.value("EXHARDDAN", 100),
                                 0,
                                 true,
                                 true,
                                 [](double currentGauge, Judgement judgement) {
                                     switch (judgement) {
                                         case Judgement::Perfect:
                                             return 0.15;
                                         case Judgement::Great:
                                             return 0.08;
                                         case Judgement::Good:
                                             return 0.0;
                                         case Judgement::Bad:
                                             return -5.0;
                                         case Judgement::Poor:
                                             return -10.0;
                                         case Judgement::EmptyPoor:
                                             return -5.0;
                                         default:
                                             return 0.0;
                                     }
                                 });
    gauges.push_back(std::move(exhardDanGauge));

    auto exdanGauge =
      std::make_unique<Lr2Gauge>("EXDAN",
                                 100,
                                 initialValues.value("EXDAN", 100),
                                 0,
                                 true,
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
                                             return -3.0;
                                         case Judgement::Poor:
                                             return -5.0;
                                         case Judgement::EmptyPoor:
                                             return -3.0;
                                         default:
                                             return 0.0;
                                     }
                                 });
    gauges.push_back(std::move(exdanGauge));

    auto danGauge = std::make_unique<Lr2Gauge>(
      "DAN",
      100,
      initialValues.value("DAN", 100),
      0,
      true,
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
                  return (currentGauge > 30) ? -2.0 : -1.2;
              case Judgement::Poor:
                  return (currentGauge > 30) ? -3.0 : -1.8;
              case Judgement::EmptyPoor:
                  return (currentGauge > 30) ? -2.0 : -1.2;
              default:
                  return 0.0;
          }
      });
    gauges.push_back(std::move(danGauge));

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