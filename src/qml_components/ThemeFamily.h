//
// Created by bobini on 20.11.23.
//

#ifndef SKIN_H
#define SKIN_H
#include <QMap>
#include <QUrl>

namespace qml_components {
class Screen
{
    Q_GADGET
    Q_PROPERTY(QUrl script READ getScript CONSTANT)
    Q_PROPERTY(QUrl settings READ getSettings CONSTANT)
    Q_PROPERTY(QString settingsData READ getSettingsData CONSTANT)
    Q_PROPERTY(QUrl settingsScript READ getSettingsScript CONSTANT)
    Q_PROPERTY(bool aliased READ isAliased CONSTANT)
    Q_PROPERTY(QString csvPath READ getCsvPath CONSTANT)
    QUrl script;
    QUrl settings;
    QString settingsData;
    QUrl settingsScript;
    bool aliased;
    QString csvPath;

  public:
    Screen() = default; // necessary for Qt
    Screen(QUrl script,
           QUrl settings,
           QString settingsData,
           QUrl settingsScript,
           bool aliased = false,
           QString csvPath = {});
    auto getScript() const -> QUrl;
    auto getSettings() const -> QUrl;
    auto getSettingsData() const -> QString;
    auto getSettingsScript() const -> QUrl;
    auto isAliased() const -> bool;
    auto getCsvPath() const -> QString;
};
class ThemeFamily
{
    Q_GADGET
    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(QVariantMap screens READ getScreensVariant CONSTANT)
    Q_PROPERTY(QMap<QString, QUrl> translations READ getTranslations CONSTANT)

  public:
    auto getPath() const -> QString;
    auto getScreensVariant() const -> QVariantMap;
    auto getScreens() const -> const QMap<QString, Screen>&;
    ThemeFamily() = default; // necessary for Qt
    ThemeFamily(QString path,
                QMap<QString, Screen> screens,
                QMap<QString, QUrl> translations);
    auto getTranslations() const -> QMap<QString, QUrl>;

  private:
    QString path;
    QMap<QString, Screen> screens;
    QMap<QString, QUrl> translations;
};
} // namespace qml_components
#endif // SKIN_H
