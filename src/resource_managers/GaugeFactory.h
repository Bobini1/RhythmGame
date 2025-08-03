//
// Created by PC on 29/09/2024.
//

#ifndef GAUGEFACTORY_H
#define GAUGEFACTORY_H

#include "Profile.h"
#include "gameplay_logic/rules/BmsGauge.h"

namespace resource_managers {

class GaugeFactory
{
  public:
    auto getStandardGauges(Profile* profile, double total, int noteCount) const
      -> QList<gameplay_logic::rules::BmsGauge*>;
    auto getCourseGauges(Profile* profile, const QHash<QString, double>& initialValues) const
      -> QList<gameplay_logic::rules::BmsGauge*>;
};

} // namespace resource_managers

#endif // GAUGEFACTORY_H
