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

    std::function<std::filesystem::path(std::string)> qmlScriptFinder;

  public:
    explicit SceneSwitcher(
      std::function<std::filesystem::path(std::string)> qmlScriptFinder);

    Q_INVOKABLE void switchToMain();
    Q_INVOKABLE void switchToGameplay();
    Q_INVOKABLE void switchToSongWheel();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENESWITCHER_H
