//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include <QObject>
#include <QtQmlIntegration>
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
    QML_ELEMENT

    resource_managers::ChartDataFactory* chartDataFactory;
    std::function<gameplay_logic::rules::TimingWindows(
      gameplay_logic::rules::BmsRank)>
      timingWindowsFactory;
    std::function<std::unique_ptr<gameplay_logic::rules::BmsHitRules>(
      gameplay_logic::rules::TimingWindows,
      std::function<double(std::chrono::nanoseconds)>)>
      hitRulesFactory;
    std::function<double(gameplay_logic::rules::TimingWindows,
                         std::chrono::nanoseconds)>
      hitValueFactory;
    std::function<QList<gameplay_logic::rules::BmsGauge*>(
      gameplay_logic::rules::TimingWindows,
      double,
      int)>
      gaugeFactory;
    resource_managers::ChartFactory* chartFactory;
    double maxHitValue;

  public:
    ChartLoader(
      resource_managers::ChartDataFactory* chartDataFactory,
      std::function<gameplay_logic::rules::TimingWindows(
        gameplay_logic::rules::BmsRank)> timingWindowsFactory,
      std::function<std::unique_ptr<gameplay_logic::rules::BmsHitRules>(
        gameplay_logic::rules::TimingWindows,
        std::function<double(std::chrono::nanoseconds)>)> hitRulesFactory,
      std::function<double(gameplay_logic::rules::TimingWindows,
                           std::chrono::nanoseconds)> hitValueFactory,
      std::function<QList<gameplay_logic::rules::BmsGauge*>(
        gameplay_logic::rules::TimingWindows timingWindows,
        double,
        int)> gaugeFactory,
      resource_managers::ChartFactory* chartFactory,
      double maxHitValue,
      QObject* parent = nullptr);

    Q_INVOKABLE gameplay_logic::Chart* loadChart(const QString& filename);
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
