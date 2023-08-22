//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"
#include "gameplay_logic/Chart.h"

namespace qml_components {
auto
ChartLoader::loadChart(const QString& filename) -> gameplay_logic::Chart*
{
    return nullptr;
}
ChartLoader::ChartLoader(QObject* parent)
  : QObject(parent)
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