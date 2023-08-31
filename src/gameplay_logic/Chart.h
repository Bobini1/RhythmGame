//
// Created by bobini on 18.08.23.
//

#ifndef RHYTHMGAME_CHART_H
#define RHYTHMGAME_CHART_H

#include <QObject>
#include <QtQmlIntegration>
#include "BmsGameReferee.h"
#include "input/KeyboardInputTranslatorToBms.h"
#include "ChartData.h"
namespace gameplay_logic {

class Chart : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int elapsed READ getElapsed NOTIFY elapsedChanged)
    Q_PROPERTY(bool over READ isOver NOTIFY overChanged)
    Q_PROPERTY(ChartData* chartData READ getChartData CONSTANT)
    Q_PROPERTY(BmsScore* score READ getScore CONSTANT)
    Q_PROPERTY(double position READ getPosition NOTIFY positionChanged)

    QTimer propertyUpdateTimer;
    std::chrono::time_point<std::chrono::steady_clock> startTimepoint;
    gameplay_logic::BmsGameReferee gameReferee;
    input::KeyboardInputTranslatorToBms inputTranslator;
    std::unordered_map<std::string, sounds::OpenALSound> sounds;
    ChartData* chartData;
    BmsScore* score;
    int elapsed = 0;
    double position = 0;
    bool over = false;
    std::span<const BpmChange> bpmChanges;

    void updateElapsed();
    void updateBpm();

  public:
    explicit Chart(gameplay_logic::BmsGameReferee gameReferee,
                   ChartData* chartData,
                   BmsScore* score,
                   std::unordered_map<std::string, sounds::OpenALSound> sounds,
                   QObject* parent = nullptr);

    Q_INVOKABLE void start();

    Q_INVOKABLE void passKey(int key);

    auto isOver() const -> bool;

    [[nodiscard]] auto getElapsed() const -> int;

    [[nodiscard]] auto getChartData() const -> ChartData*;

    [[nodiscard]] auto getScore() const -> BmsScore*;

    [[nodiscard]] auto getPosition() const -> double;

  signals:
    void elapsedChanged(int delta);
    void positionChanged(double delta);
    void overChanged();
    void bpmChanged(BpmChange bpmChange);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHART_H
