//
// Created by bobini on 11.08.23.
//

#ifndef RHYTHMGAME_SCENESWITCHER_H
#define RHYTHMGAME_SCENESWITCHER_H
#include "resource_managers/QmlScriptFinder.h"
#include "ViewManager.h"
#include <QObject>
#include <qqml.h>
#include <QQuickView>
namespace qml_components {

class SceneSwitcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    std::function<std::filesystem::path(std::string)> qmlScriptFinder;
    ViewManager* view;

    static inline SceneSwitcher* instance;

  public:
    static void setInstance(SceneSwitcher* instance);
    explicit SceneSwitcher(
      ViewManager* view,
      std::function<std::filesystem::path(std::string)> qmlScriptFinder);
    static auto create(QQmlEngine*, QJSEngine* engine) -> SceneSwitcher*;
    Q_INVOKABLE void switchToMain();
    Q_INVOKABLE void switchToGameplay();
    Q_INVOKABLE void switchToSongWheel();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENESWITCHER_H
