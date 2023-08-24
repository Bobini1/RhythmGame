//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"
#include "gameplay_logic/Chart.h"

namespace qml_components {
auto
ChartLoader::loadChart(const QString& filename) -> gameplay_logic::Chart*
{
    return chartFactory->createChart(filename);
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
    instance = newInstance;
}
auto
ChartLoader::create(QQmlEngine* engine, QJSEngine* scriptEngine) -> ChartLoader*
{
    Q_ASSERT(instance);
    return instance;
}
} // namespace qml_components