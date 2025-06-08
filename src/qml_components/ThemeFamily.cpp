//
// Created by bobini on 20.11.23.
//

#include "ThemeFamily.h"

#include <utility>
#include <QVariant>
#include <QUrl>
namespace qml_components {
auto
ThemeFamily::getPath() const -> QString
{
    return path;
}
auto
ThemeFamily::getScreensVariant() const -> QVariantMap
{
    auto ret = QVariantMap{};
    for (const auto& [key, value] : screens.asKeyValueRange()) {
        ret.insert(key, QVariant::fromValue(value));
    }
    return ret;
}
auto
ThemeFamily::getScreens() const -> QMap<QString, Screen>
{
    return screens;
}
ThemeFamily::ThemeFamily(QString path, QMap<QString, Screen> screens)
  : path(std::move(path)), screens(std::move(screens))
{
}
Screen::Screen(QUrl script, QUrl settings, QUrl settingsScript)
  : script(std::move(script))
  , settings(std::move(settings))
  , settingsScript(std::move(settingsScript))
{
}
auto
Screen::getScript() const -> QUrl
{
    return script;
}
auto
Screen::getSettings() const -> QUrl
{
    return settings;
}
auto
Screen::getSettingsScript() const -> QUrl
{
    return settingsScript;
}
} // namespace qml_components