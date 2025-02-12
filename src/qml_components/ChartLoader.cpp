//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"

#include <utility>
#include "gameplay_logic/Chart.h"
#include "support/QStringToPath.h"
#include "magic_enum/magic_enum.hpp"
#include "qdir.h"

#include <spdlog/spdlog.h>

namespace qml_components {
auto
ChartLoader::loadChart(const QString& filename,
                       resource_managers::Profile* player1,
                       resource_managers::Profile* player2,
                       QList<int64_t> randomSequence) -> gameplay_logic::Chart*
{
    if (!player1) {
        spdlog::error("Player 1 is null");
        return nullptr;
    }
    try {
        auto randomGenerator =
          [randomSequence = std::move(randomSequence),
           counter = 0](const charts::parser_models::ParsedBmsChart::RandomRange
                          randomRange) mutable {
              thread_local auto randomEngine =
                std::default_random_engine{ std::random_device{}() };
              if (counter < randomSequence.size()) {
                  return randomSequence[counter++];
              }
              return std::uniform_int_distribution{
                  charts::parser_models::ParsedBmsChart::RandomRange{ 1 },
                  randomRange
              }(randomEngine);
          };

        auto fileAbsolute = QFileInfo(filename).absoluteFilePath();
        auto chartComponents = chartDataFactory->loadChartData(
          support::qStringToPath(fileAbsolute), randomGenerator);
        if (isDp(chartComponents.chartData->getKeymode()) && player1 &&
            player2) {
            spdlog::error("Can't launch DP for two players");
            return nullptr;
        }
        const auto rankInt = chartComponents.chartData->getRank();
        const auto rank =
          magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(rankInt)
            .value_or(gameplay_logic::rules::defaultBmsRank);
        auto hitRules =
          std::vector<std::unique_ptr<gameplay_logic::rules::BmsHitRules>>{};
        auto gauges = std::vector<QList<gameplay_logic::rules::BmsGauge*>>{};
        auto timingWindows = timingWindowsFactory(rank);
        auto maxHitValue = hitValueFactory(std::chrono::nanoseconds{ 0 },
                                           gameplay_logic::Judgement::Perfect);
        auto player1data = resource_managers::ChartFactory::PlayerSpecificData{
            player1,
            gaugeFactory(player1,
                         timingWindows,
                         chartComponents.chartData->getTotal(),
                         chartComponents.chartData->getNormalNoteCount()),
            hitRulesFactory(timingWindows, hitValueFactory)
        };
        auto player2data =
          player2
            ? std::make_optional<
                resource_managers::ChartFactory::PlayerSpecificData>(
                player2,
                gaugeFactory(player2,
                             timingWindows,
                             chartComponents.chartData->getTotal(),
                             chartComponents.chartData->getNormalNoteCount()),
                hitRulesFactory(timingWindows, hitValueFactory))
            : std::nullopt;
        return chartFactory->createChart(std::move(chartComponents),
                                         std::move(player1data),
                                         std::move(player2data),
                                         maxHitValue);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return nullptr;
    }
}
ChartLoader::ChartLoader(ProfileList* profileList,
                         input::InputTranslator* inputTranslator,
                         resource_managers::ChartDataFactory* chartDataFactory,
                         TimingWindowsFactory timingWindowsFactory,
                         HitRulesFactory hitRulesFactory,
                         HitValueFactory hitValueFactory,
                         GaugeFactory gaugeFactory,
                         resource_managers::ChartFactory* chartFactory,
                         QObject* parent)
  : QObject(parent)
  , chartDataFactory(chartDataFactory)
  , timingWindowsFactory(std::move(timingWindowsFactory))
  , hitRulesFactory(std::move(hitRulesFactory))
  , hitValueFactory(std::move(hitValueFactory))
  , gaugeFactory(std::move(gaugeFactory))
  , chartFactory(chartFactory)
  , profileList(profileList)
  , inputTranslator(inputTranslator)

{
}
} // namespace qml_components