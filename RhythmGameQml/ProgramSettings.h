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
    QML_SINGLETON

    // will be empty if not set
    Q_PROPERTY(QUrl chartPath READ getChartPath CONSTANT)

    QUrl chartPath;

    static inline ProgramSettings* instance{};

  public:
    explicit ProgramSettings(QUrl chartPath, QObject* parent = nullptr);
    static void setInstance(ProgramSettings* instance);
    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine)
      -> ProgramSettings*;
    [[nodiscard]] auto getChartPath() const -> QUrl;
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
