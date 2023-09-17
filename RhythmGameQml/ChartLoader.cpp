//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"

#include <utility>
#include "gameplay_logic/Chart.h"
#include "magic_enum.hpp"

namespace qml_components {
auto
ChartLoader::loadChart(const QString& filename) -> gameplay_logic::Chart*
{
    try {
        auto randomGenerator =
          [](charts::parser_models::ParsedBmsChart::RandomRange randomRange) {
              static thread_local auto randomEngine =
                std::default_random_engine{ std::random_device{}() };
              return std::uniform_int_distribution{
                  charts::parser_models::ParsedBmsChart::RandomRange{ 1 },
                  randomRange
              }(randomEngine);
          };
        auto chartComponents =
          chartDataFactory->loadChartData(filename, randomGenerator);
        auto rankInt = chartComponents.chartData->getRank();
        auto rank =
          magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(rankInt)
            .value_or(gameplay_logic::rules::defaultBmsRank);
        auto timingWindows = timingWindowsFactory(rank);
        auto hitValuesFactoryPartial =
          [timingWindows, hitValueFactory = this->hitValueFactory](
            std::chrono::nanoseconds offset) {
              return hitValueFactory(timingWindows, offset);
          };
        auto gauges = gaugeFactory(timingWindows,
                                   chartComponents.chartData->getTotal(),
                                   chartComponents.chartData->getNoteCount());
        auto hitRules =
          hitRulesFactory(timingWindows, std::move(hitValuesFactoryPartial));

        return chartFactory->createChart(std::move(chartComponents),
                                         std::move(hitRules),
                                         std::move(gauges),
                                         maxHitValue);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return nullptr;
    }
}
ChartLoader::ChartLoader(
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
  QObject* parent)
  : QObject(parent)
  , chartDataFactory(chartDataFactory)
  , timingWindowsFactory(std::move(timingWindowsFactory))
  , hitRulesFactory(std::move(hitRulesFactory))
  , hitValueFactory(std::move(hitValueFactory))
  , gaugeFactory(std::move(gaugeFactory))
  , chartFactory(chartFactory)
  , maxHitValue(maxHitValue)

{
}
void
ChartLoader::setInstance(ChartLoader* newInstance)
{
    QJSEngine::setObjectOwnership(newInstance, QQmlEngine::CppOwnership);
    instance = newInstance;
}
auto
ChartLoader::create(QQmlEngine* engine, QJSEngine* scriptEngine) -> ChartLoader*
{
    Q_ASSERT(instance);
    return instance;
}
} // namespace qml_components