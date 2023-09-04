//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <utility>

namespace qml_components {
auto
ProgramSettings::getChartPath() const -> QUrl
{
    return chartPath;
}
void
ProgramSettings::setInstance(ProgramSettings* newInstance)
{
    QJSEngine::setObjectOwnership(newInstance, QQmlEngine::CppOwnership);
    instance = newInstance;
}
auto
ProgramSettings::create(QQmlEngine* /*engine*/, QJSEngine* /*scriptEngine*/)
  -> ProgramSettings*
{
    Q_ASSERT(instance);
    return instance;
}
ProgramSettings::ProgramSettings(QUrl chartPath, QObject* parent)
  : QObject(parent)
  , chartPath(std::move(chartPath))
{
}
} // qml_components