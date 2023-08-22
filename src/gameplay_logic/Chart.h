//
// Created by bobini on 18.08.23.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <QObject>
#include <QtQmlIntegration>
#include "BmsGameReferee.h"
#include "input/KeyboardInputTranslatorToBms.h"
namespace gameplay_logic {

class Chart : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(bool over READ isOver NOTIFY overChanged)

    QTimer propertyUpdateTimer;
    std::chrono::time_point<std::chrono::steady_clock> startTimepoint;
    quint64 startTimestamp{};
    gameplay_logic::BmsGameReferee gameReferee;
    input::KeyboardInputTranslatorToBms inputTranslator;
    int elapsed = 0;
    bool over = false;

    void updateElapsed();

  public:
    explicit Chart(gameplay_logic::BmsGameReferee gameReferee,
                   QObject* parent = nullptr);

    Q_INVOKABLE void start();

    Q_INVOKABLE void passKey(QKeyEvent* event);

    auto isOver() const -> bool;

    [[nodiscard]] auto getElapsed() const -> int;

  signals:
    void elapsedChanged();
    void overChanged();
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
