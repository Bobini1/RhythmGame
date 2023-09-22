//
// Created by bobini on 17.08.23.
//

#ifndef RHYTHMGAME_PROGRAMSETTINGS_H
#define RHYTHMGAME_PROGRAMSETTINGS_H

#include <QObject>
#include <QtQmlIntegration>
#include <QJSEngine>
#include <QQmlEngine>
namespace qml_components {

class ProgramSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // will be empty if not set
    Q_PROPERTY(QString chartPath READ getChartPath CONSTANT)

    QString chartPath;

  public:
    explicit ProgramSettings(QString chartPath, QObject* parent = nullptr);
    [[nodiscard]] auto getChartPath() const -> QString;
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
