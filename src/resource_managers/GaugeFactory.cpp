//
// Created by PC on 29/09/2024.
//

#include "GaugeFactory.h"
#include <spdlog/spdlog.h>
#include "gameplay_logic/rules/Lr2Gauge.h"

namespace resource_managers {
QList<gameplay_logic::rules::BmsGauge*>
GaugeFactory::selectGauges(
  Profile* profile,
  std::vector<std::unique_ptr<gameplay_logic::rules::BmsGauge>> gauges) const
{
    const auto* vars = profile->getVars()->getGeneralVars();
    const auto gaugeMode = vars->getGaugeMode();
    const auto selectedGauge = vars->getGaugeType();
    const auto equivalents = QHash<QString, QString>{
        { "DAN", "NORMAL" }, { "EXDAN", "HARD" }, { "EXHARDDAN", "EXHARD" }
    };
    auto selectedIt = std::ranges::find_if(gauges, [&](const auto& elem) {
        return elem->getName() == selectedGauge ||
               equivalents[elem->getName()] == selectedGauge;
    });
    // invalid gauge selected
    if (selectedIt == std::ranges::end(gauges)) {
        selectedIt = gauges.begin();
    }
    if (gaugeMode == GaugeMode::Exclusive) {
        return { selectedIt->release() };
    }
    auto ret = QList<gameplay_logic::rules::BmsGauge*>{};
    if (gaugeMode == GaugeMode::SelectToUnder) {
        std::transform(selectedIt,
                       std::ranges::end(gauges),
                       std::back_insert_iterator(ret),
                       [](auto& elem) { return elem.release(); });
    }
    if (gaugeMode == GaugeMode::Best) {
        std::ranges::transform(gauges,
                               std::back_insert_iterator(ret),
                               [](auto& elem) { return elem.release(); });
    }
    return ret;
}
auto
GaugeFactory::getStandardGauges(Profile* profile,
                                double total,
                                int noteCount) const
  -> QList<gameplay_logic::rules::BmsGauge*>
{
    auto gauges = gameplay_logic::rules::Lr2Gauge::getGauges(total, noteCount);
    return selectGauges(profile, std::move(gauges));
}
auto
GaugeFactory::getCourseGauges(Profile* profile, const QHash<QString, double>& initialValues) const
  -> QList<gameplay_logic::rules::BmsGauge*>
{
    auto gauges = gameplay_logic::rules::Lr2Gauge::getDanGauges(initialValues);
    return selectGauges(profile, std::move(gauges));
}
} // resource_managers