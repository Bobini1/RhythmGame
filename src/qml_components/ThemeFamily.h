//
// Created by bobini on 20.11.23.
//

#ifndef SKIN_H
#define SKIN_H
#include <QObject>
#include <QMap>
#include <QUrl>

namespace qml_components {
class Screen
{
    Q_GADGET
    Q_PROPERTY(QUrl script READ getScript CONSTANT)
    Q_PROPERTY(QUrl settings READ getSettings CONSTANT)
    Q_PROPERTY(QUrl settingsScript READ getSettingsScript CONSTANT)
    QUrl script;
    QUrl settings;
    QUrl settingsScript;

  public:
    Screen() = default; // necessary for Qt
    Screen(QUrl script, QUrl settings, QUrl icon);
    auto getScript() const -> QUrl;
    auto getSettings() const -> QUrl;
    auto getSettingsScript() const -> QUrl;
};
class ThemeFamily
{
    Q_GADGET
    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(QVariantMap screens READ getScreens CONSTANT)

  public:
    auto getPath() const -> QString;
    auto getScreens() const -> QVariantMap;
    ThemeFamily() = default; // necessary for Qt
    ThemeFamily(QString path, QMap<QString, Screen> screens);

  private:
    QString path;
    QVariantMap screens;
};
} // namespace qml_components
#endif // SKIN_H
