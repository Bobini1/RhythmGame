//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_LR2GAUGE_H
#define RHYTHMGAME_LR2GAUGE_H

#include "BmsGauge.h"
#include "TimingWindows.h"
#include "BmsRanks.h"
namespace gameplay_logic::rules {
class Lr2Gauge : public BmsGauge
{
    TimingWindows timingWindows;
    bool permanentDeath;
    std::function<double(double, Judgement)> judgementValueFactory;

  public:
    explicit Lr2Gauge(
      TimingWindows timingWindows,
      double gaugeMax,
      double initialValue,
      double threshold,
      bool permanentDeath,
      std::function<double(double, Judgement)> judgementValueFactory,
      QObject* parent = nullptr);
    void addHit(std::chrono::nanoseconds offsetFromStart,
                std::chrono::nanoseconds hitOffset) override;

    static auto getGauges(gameplay_logic::rules::TimingWindows timingWindows,
                          double total,
                          int noteCount) -> QList<BmsGauge*>;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_LR2GAUGE_H
