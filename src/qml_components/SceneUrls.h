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

    Q_PROPERTY(QUrl mainScene READ mainScene NOTIFY mainSceneChanged)
    Q_PROPERTY(QUrl gameplayScene READ gameplayScene NOTIFY
                 gameplaySceneChanged)
    Q_PROPERTY(QUrl songWheelScene READ songWheelScene NOTIFY
                 songWheelSceneChanged)
    Q_PROPERTY(QUrl settingsScene READ settingsScene NOTIFY
                 settingsSceneChanged)
    Q_PROPERTY(
      QUrl resultScene READ resultScene NOTIFY resultSceneChanged)

    std::function<resource_managers::models::ThemeConfig()> themeConfigFactory;
    resource_managers::models::ThemeConfig themeConfig;

  public:
    explicit SceneUrls(std::function<resource_managers::models::ThemeConfig()>
                         themeConfigFactory);

    [[nodiscard]] auto mainScene() const -> QUrl;
    [[nodiscard]] auto gameplayScene() const -> QUrl;
    [[nodiscard]] auto songWheelScene() const -> QUrl;
    [[nodiscard]] auto settingsScene() const -> QUrl;
    [[nodiscard]] auto resultScene() const -> QUrl;

    Q_INVOKABLE void refreshThemeConfig();

  signals:
    void mainSceneChanged();
    void gameplaySceneChanged();
    void songWheelSceneChanged();
    void settingsSceneChanged();
    void resultSceneChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCENEURLS_H
