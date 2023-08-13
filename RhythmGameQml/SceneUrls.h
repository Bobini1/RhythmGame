//
// Created by bobini on 11.08.23.
//

#ifndef RHYTHMGAME_SCENEURLS_H
#define RHYTHMGAME_SCENEURLS_H
#include "ViewManager.h"
#include "resource_managers/models/ThemeConfig.h"
#include <QObject>
#include <qqml.h>
#include <QQuickView>
#include <map>
#include <functional>
#include <string>
namespace qml_components {

class SceneUrls : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QUrl mainSceneUrl READ mainSceneUrl NOTIFY mainSceneUrlChanged)
    Q_PROPERTY(QUrl gameplaySceneUrl READ gameplaySceneUrl NOTIFY
                 gameplaySceneUrlChanged)
    Q_PROPERTY(QUrl songWheelUrlScene READ songWheelSceneUrl NOTIFY
                 songWheelSceneUrlChanged)

    static inline SceneUrls* instance;
    std::function<resource_managers::models::ThemeConfig()> themeConfigFactory;
    resource_managers::models::ThemeConfig themeConfig;

  public:
    explicit SceneUrls(std::function<resource_managers::models::ThemeConfig()>
                         themeConfigFactory);

    [[nodiscard]] auto mainSceneUrl() const -> QUrl;
    [[nodiscard]] auto gameplaySceneUrl() const -> QUrl;
    [[nodiscard]] auto songWheelSceneUrl() const -> QUrl;

    Q_INVOKABLE void refreshThemeConfig();

    static void setInstance(SceneUrls* newInstance);

    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine)
      -> SceneUrls*;

  signals:
    void mainSceneUrlChanged();
    void gameplaySceneUrlChanged();
    void songWheelSceneUrlChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENEURLS_H
