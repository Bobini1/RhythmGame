//
// Created by bobini on 27.11.23.
//

#include "Themes.h"

#include <utility>

namespace qml_components {
auto
Themes::getAvailableThemeFamilies() const -> QVariantMap
{
    return themes;
}
Themes::
Themes(QMap<QString, ThemeFamily> themes)
{
    for (const auto& [key, value] : themes.asKeyValueRange()) {
        this->themes.insert(key, QVariant::fromValue(value));
    }
}
} // namespace qml_components