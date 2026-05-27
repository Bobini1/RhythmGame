#pragma once

#include "qml_components/ThemeFamily.h"

#include <QMap>
#include <filesystem>

namespace resource_managers::lr2_skin {
auto
scanThemeDirectory(const std::filesystem::path& themeDirectory)
  -> QMap<QString, qml_components::ThemeFamily>;
}
