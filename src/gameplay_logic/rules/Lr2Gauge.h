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
    void addMineHit(std::chrono::nanoseconds offsetFromStart,
                    double penalty) override;
    void addHoldEndHit(std::chrono::nanoseconds offsetFromStart,
                       std::chrono::nanoseconds hitOffset) override;
    void addHoldEndMiss(std::chrono::nanoseconds offsetFromStart) override;

    static auto getGauges(TimingWindows timingWindows,
                          double total,
                          int noteCount)
      -> std::vector<std::unique_ptr<BmsGauge>>;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_LR2GAUGE_H
