//
// Created by PC on 29/09/2024.
//

#ifndef GAUGEFACTORY_H
#define GAUGEFACTORY_H

#include "Profile.h"
#include "gameplay_logic/rules/Lr2Gauge.h"
#include "gameplay_logic/rules/TimingWindows.h"

#include <QQmlPropertyMap>
#include <gameplay_logic/rules/BmsGauge.h>
#include <spdlog/spdlog.h>

namespace resource_managers {

class GaugeFactory
{
  public:
    auto operator()(Profile* profile,
                    gameplay_logic::rules::TimingWindows timingWindows,
                    double total,
                    int noteCount) const
      -> QList<gameplay_logic::rules::BmsGauge*>
    {
        auto gauges = gameplay_logic::rules::Lr2Gauge::getGauges(
          timingWindows, total, noteCount);
        const auto* vars = profile->getVars()->getGeneralVars();
        const auto gaugeMode = vars->getGaugeMode();
        const auto selectedGauge = vars->getGaugeType();
        auto selectedIt =
          std::ranges::find_if(gauges, [selectedGauge](const auto& elem) {
              auto name = elem->objectName();
              return name == selectedGauge;
          });
        // invalid gauge selected
        if (selectedIt == std::ranges::end(gauges)) {
            spdlog::error("Invalid gauge selected: {}",
                          selectedGauge.toStdString());
            return {};
        }
        auto ret = QList<gameplay_logic::rules::BmsGauge*>{};
        if (gaugeMode == GaugeMode::Exclusive) {
            ret = { selectedIt->release() };
        }
        if (gaugeMode == GaugeMode::SelectToUnder) {
            std::transform(selectedIt,
                           std::ranges::end(gauges),
                           std::back_insert_iterator(ret),
                           [](auto& elem) { return elem.release(); });
        }
        if (gaugeMode == GaugeMode::Best) {
            std::transform(gauges.begin(),
                           gauges.end(),
                           std::back_insert_iterator(ret),
                           [](auto& elem) { return elem.release(); });
        }
        return ret;
    }
};

} // namespace resource_managers

#endif // GAUGEFACTORY_H
