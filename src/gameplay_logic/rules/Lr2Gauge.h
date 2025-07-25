//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_LR2GAUGE_H
#define RHYTHMGAME_LR2GAUGE_H

#include "BmsGauge.h"
#include "TimingWindows.h"

#include <QObject>
#include <functional>
namespace gameplay_logic::rules {
class Lr2Gauge : public BmsGauge
{
    bool permanentDeath;
    std::function<double(double, Judgement)> judgementValueFactory;

  public:
    explicit Lr2Gauge(
      QString gaugeName,
      QString awardedClearType,
      double gaugeMax,
      double initialValue,
      double threshold,
      bool permanentDeath,
      std::function<double(double, Judgement)> judgementValueFactory,
      QObject* parent = nullptr);
    void addHit(std::chrono::nanoseconds offsetFromStart,
                std::chrono::nanoseconds hitOffset,
                Judgement judgement) override;
    void addMineHit(std::chrono::nanoseconds offsetFromStart,
                    double penalty) override;

    static auto getGauges(double total,
                          int noteCount)
      -> std::vector<std::unique_ptr<BmsGauge>>;

    static auto getDanGauges(double total,
                                int noteCount)
      -> std::vector<std::unique_ptr<BmsGauge>>;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_LR2GAUGE_H
