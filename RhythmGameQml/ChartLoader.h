//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include <QObject>
#include <QtQmlIntegration>
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/Chart.h"

namespace qml_components {

class ChartLoader : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    static inline ChartLoader* instance;

  public:
    explicit ChartLoader(QObject* parent = nullptr);

    Q_INVOKABLE gameplay_logic::Chart* loadChart(const QString& filename);

    static void setInstance(ChartLoader* newInstance);

    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine)
      -> ChartLoader*;
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
