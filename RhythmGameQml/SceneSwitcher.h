//
// Created by bobini on 11.08.23.
//

#ifndef RHYTHMGAME_SCENESWITCHER_H
#define RHYTHMGAME_SCENESWITCHER_H
#include "resource_managers/QmlScriptFinder.h"
#include <QObject>
#include <qqml.h>
namespace qml_components {

class SceneSwitcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    static std::function<std::filesystem::path(std::string)>
      qmlScriptFinderStatic;
    std::function<std::filesystem::path(std::string)> qmlScriptFinder;

  public:
    static void setQmlScriptFinder(
      std::function<std::filesystem::path(std::string)> qmlScriptFinder);
    explicit SceneSwitcher(
      std::function<std::filesystem::path(std::string)> qmlScriptFinder);
    static auto create(QQmlEngine*, QJSEngine* engine) -> SceneSwitcher*;
    Q_INVOKABLE void switchToMain();
    Q_INVOKABLE void switchToGameplay();
    Q_INVOKABLE void switchToSongWheel();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENESWITCHER_H
