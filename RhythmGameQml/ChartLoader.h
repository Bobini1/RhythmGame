//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include <QObject>
#include <QtQmlIntegration>
#include "ChartData.h"
namespace qml_components {

class ChartLoader : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    static inline ChartLoader* instance;

  public:
    explicit ChartLoader(QObject* parent = nullptr);

    Q_INVOKABLE qml_components::ChartData* loadChart(const QString& filename);

    static void setInstance(ChartLoader* newInstance);

    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine)
      -> ChartLoader*;
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
