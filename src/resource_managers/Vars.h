//
// Created by bobini on 05.04.24.
//

#ifndef RHYTHMGAME_VARS_H
#define RHYTHMGAME_VARS_H

#include "qml_components/ThemeFamily.h"

#include <QObject>
#include <QQmlPropertyMap>
#include <filesystem>

namespace qml_components {
class ProfileList;
} // namespace qml_components

namespace resource_managers {
class Profile;

class Vars final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      QQmlPropertyMap* globalVars READ getGlobalVars NOTIFY globalVarsChanged)
    Q_PROPERTY(
      QQmlPropertyMap* themeVars READ getThemeVars NOTIFY themeVarsChanged)
    QQmlPropertyMap globalVars;
    QQmlPropertyMap themeVars;
    const Profile* profile;
    QMap<QString, qml_components::ThemeFamily> availableThemeFamilies;
    QHash<QString, QHash<QString, QHash<QString, QVariant>>> loadedThemeVars;

    void onThemeConfigChanged(const QString& key, const QVariant& value);
    void populateThemePropertyMap(
      QQmlPropertyMap& themeVars,
      QHash<QString, QHash<QString, QHash<QString, QVariant>>> themeVarsData,
      const std::filesystem::path& themeVarsPath,
      const QQmlPropertyMap& themeConfig);

  public:
    explicit Vars(
      const Profile* profile,
      QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
      QObject* parent = nullptr);
    auto getGlobalVars() -> QQmlPropertyMap*;
    auto getThemeVars() -> QQmlPropertyMap*;
    Q_INVOKABLE QList<QString> getSelectableFilesForDirectory(
      QString directory) const;

  signals:
    void globalVarsChanged();
    void themeVarsChanged();
};
} // namespace resource_managers

#endif // RHYTHMGAME_VARS_H
