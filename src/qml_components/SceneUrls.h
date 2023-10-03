//
// Created by bobini on 11.08.23.
//

#ifndef RHYTHMGAME_SCENEURLS_H
#define RHYTHMGAME_SCENEURLS_H
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

    Q_PROPERTY(QUrl mainSceneUrl READ mainSceneUrl NOTIFY mainSceneUrlChanged)
    Q_PROPERTY(QUrl gameplaySceneUrl READ gameplaySceneUrl NOTIFY
                 gameplaySceneUrlChanged)
    Q_PROPERTY(QUrl songWheelSceneUrl READ songWheelSceneUrl NOTIFY
                 songWheelSceneUrlChanged)
    Q_PROPERTY(QUrl settingsSceneUrl READ settingsSceneUrl NOTIFY
                 settingsSceneUrlChanged)
    Q_PROPERTY(
      QUrl resultSceneUrl READ resultSceneUrl NOTIFY resultSceneUrlChanged)

    std::function<resource_managers::models::ThemeConfig()> themeConfigFactory;
    resource_managers::models::ThemeConfig themeConfig;

  public:
    explicit SceneUrls(std::function<resource_managers::models::ThemeConfig()>
                         themeConfigFactory);

    [[nodiscard]] auto mainSceneUrl() const -> QUrl;
    [[nodiscard]] auto gameplaySceneUrl() const -> QUrl;
    [[nodiscard]] auto songWheelSceneUrl() const -> QUrl;
    [[nodiscard]] auto settingsSceneUrl() const -> QUrl;
    [[nodiscard]] auto resultSceneUrl() const -> QUrl;

    Q_INVOKABLE void refreshThemeConfig();

  signals:
    void mainSceneUrlChanged();
    void gameplaySceneUrlChanged();
    void songWheelSceneUrlChanged();
    void settingsSceneUrlChanged();
    void resultSceneUrlChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENEURLS_H