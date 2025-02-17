//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include "ProfileList.h"

#include <memory>
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/Chart.h"
#include "resource_managers/ChartFactory.h"
#include "gameplay_logic/rules/TimingWindows.h"
#include "gameplay_logic/rules/BmsRanks.h"
#include "gameplay_logic/rules/BmsHitRules.h"

namespace qml_components {

class ChartLoader : public QObject
{
    Q_OBJECT

  public:
    using GaugeFactory = std::function<QList<gameplay_logic::rules::BmsGauge*>(
      resource_managers::Profile* profile,
      gameplay_logic::rules::TimingWindows timingWindows,
      double total,
      int noteCount)>;
    using TimingWindowsFactory =
      std::function<gameplay_logic::rules::TimingWindows(
        gameplay_logic::rules::BmsRank)>;
    using HitValueFactory = std::function<double(std::chrono::nanoseconds,
                                                 gameplay_logic::Judgement)>;
    using HitRulesFactory =
      std::function<std::unique_ptr<gameplay_logic::rules::BmsHitRules>(
        gameplay_logic::rules::TimingWindows,
        HitValueFactory)>;

  private:
    resource_managers::ChartDataFactory* chartDataFactory;
    TimingWindowsFactory timingWindowsFactory;
    HitRulesFactory hitRulesFactory;
    HitValueFactory hitValueFactory;
    GaugeFactory gaugeFactory;
    resource_managers::ChartFactory* chartFactory;
    ProfileList* profileList;
    input::InputTranslator* inputTranslator;

  public:
    ChartLoader(ProfileList* profileList,
                input::InputTranslator* inputTranslator,
                resource_managers::ChartDataFactory* chartDataFactory,
                TimingWindowsFactory timingWindowsFactory,
                HitRulesFactory hitRulesFactory,
                HitValueFactory hitValueFactory,
                GaugeFactory gaugeFactory,
                resource_managers::ChartFactory* chartFactory,
                QObject* parent = nullptr);

    Q_INVOKABLE gameplay_logic::Chart* loadChart(
      const QString& filename,
      resource_managers::Profile* player1,
      resource_managers::Profile* player2,
      QList<int64_t> randomSequence = {});
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
