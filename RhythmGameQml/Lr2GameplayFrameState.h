#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace gameplay_logic {
class ChartRunner;
class Player;
} // namespace gameplay_logic

class Lr2GameplayFrameState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* chart READ chart WRITE setChart NOTIFY chartChanged)
    Q_PROPERTY(qreal progressPosition READ progressPosition NOTIFY progressPositionChanged)
    Q_PROPERTY(qreal position1 READ position1 NOTIFY position1Changed)
    Q_PROPERTY(qreal position2 READ position2 NOTIFY position2Changed)
    Q_PROPERTY(int rhythmTimerSkinTime READ rhythmTimerSkinTime NOTIFY rhythmTimerSkinTimeChanged)

public:
    explicit Lr2GameplayFrameState(QObject* parent = nullptr);

    QObject* chart() const;
    void setChart(QObject* chart);

    qreal progressPosition() const;
    qreal position1() const;
    qreal position2() const;
    int rhythmTimerSkinTime() const;

    Q_INVOKABLE void refresh(int frameSkinTime);
    Q_INVOKABLE void reset();

signals:
    void chartChanged();
    void progressPositionChanged();
    void position1Changed();
    void position2Changed();
    void rhythmTimerSkinTimeChanged();

private:
    void cacheChartObjects();
    void setProgressPosition(qreal value);
    void setPosition1(qreal value);
    void setPosition2(qreal value);
    void setRhythmTimerSkinTime(int value);
    static bool sameReal(qreal lhs, qreal rhs);

    QPointer<QObject> m_chartObject;
    QPointer<gameplay_logic::ChartRunner> m_chart;
    QPointer<gameplay_logic::Player> m_player1;
    QPointer<gameplay_logic::Player> m_player2;
    bool m_useDoublePlayLanes = false;
    int m_lastProgressFrameSkinTime = -1;
    qreal m_progressPosition = 0.0;
    qreal m_position1 = 0.0;
    qreal m_position2 = 0.0;
    int m_rhythmTimerSkinTime = -1;
};
