//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_CHARTFACTORY_H
#define RHYTHMGAME_CHARTFACTORY_H

#include "ChartDataFactory.h"
#include "gameplay_logic/Chart.h"
#include "charts/helper_functions/loadBmsSounds.h"
namespace resource_managers {

class ChartFactory
{
    ChartDataFactory* chartDataFactory;

  public:
    explicit ChartFactory(ChartDataFactory* chartDataFactory);

    auto createChart(const QString& filename) -> gameplay_logic::Chart*;
};

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTFACTORY_H
