//
// Created by bobini on 05.04.24.
//

#ifndef RHYTHMGAME_VARS_H
#define RHYTHMGAME_VARS_H

#include <QObject>
#include <QQmlPropertyMap>

namespace qml_components {
class ProfileList;
} // namespace qml_components

namespace resource_managers {
class Profile;

class Vars : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      QQmlPropertyMap* globalVars READ getGlobalVars NOTIFY globalVarsChanged)
    Q_PROPERTY(
      QQmlPropertyMap* themeVars READ getThemeVars NOTIFY themeVarsChanged)
    std::unique_ptr<QQmlPropertyMap> globalVars;
    std::unique_ptr<QQmlPropertyMap> themeVars;
    qml_components::ProfileList* profileList;
    QMetaObject::Connection currentThemeConfigConnection;

    void onProfileChanged(resource_managers::Profile* profile);
    void onThemeConfigChanged(const QString& key, const QVariant& value);

  public:
    explicit Vars(qml_components::ProfileList* profileList,
                  QObject* parent = nullptr);
    auto getGlobalVars() -> QQmlPropertyMap*;
    auto getThemeVars() -> QQmlPropertyMap*;

  signals:
    void globalVarsChanged();
    void themeVarsChanged();
};
} // namespace resource_managers

#endif // RHYTHMGAME_VARS_H
