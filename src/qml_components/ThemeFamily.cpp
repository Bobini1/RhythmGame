//
// Created by bobini on 20.11.23.
//

#include "ThemeFamily.h"

#include <utility>
#include <QVariant>
namespace qml_components {
auto
ThemeFamily::getPath() const -> QString
{
    return path;
}
auto
ThemeFamily::getThemes() const -> QVariantMap
{
    return themes;
}
ThemeFamily::
ThemeFamily(QString path, QMap<QString, QString> themes)
  : path(std::move(path))
{
    for (const auto& [key, value] : themes.asKeyValueRange()) {
        this->themes.insert(key, QVariant::fromValue(value));
    }
}
} // namespace qml_components