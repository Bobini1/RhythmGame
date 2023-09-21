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
ProgramSettings::ProgramSettings(QUrl chartPath, QObject* parent)
  : QObject(parent)
  , chartPath(std::move(chartPath))
{
}
} // namespace qml_components