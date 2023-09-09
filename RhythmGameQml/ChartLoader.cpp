//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"
#include "gameplay_logic/Chart.h"

namespace qml_components {
auto
ChartLoader::loadChart(const QString& filename) -> gameplay_logic::Chart*
{
    try {
        return chartFactory->createChart(filename);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return nullptr;
    }
}
ChartLoader::ChartLoader(resource_managers::ChartFactory* chartFactory,
                         QObject* parent)
  : QObject(parent)
  , chartFactory(chartFactory)
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