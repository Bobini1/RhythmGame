#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace gameplay_logic {
class ChartRunner;
class CourseRunner;
class Player;
} // namespace gameplay_logic

class Lr2GameplayFrameState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* chart READ chart WRITE setChart NOTIFY chartChanged)
    Q_PROPERTY(qreal progressPosition READ progressPosition NOTIFY progressPositionChanged)
    Q_PROPERTY(qreal position1 READ position1 NOTIFY position1Changed)
    Q_PROPERTY(qreal position2 READ position2 NOTIFY position2Changed)
    Q_PROPERTY(qreal gaugeValue1 READ gaugeValue1 NOTIFY gaugeValue1Changed)
    Q_PROPERTY(qreal gaugeValue2 READ gaugeValue2 NOTIFY gaugeValue2Changed)
    Q_PROPERTY(QString activeGaugeName1 READ activeGaugeName1 NOTIFY activeGaugeName1Changed)
    Q_PROPERTY(QString activeGaugeName2 READ activeGaugeName2 NOTIFY activeGaugeName2Changed)
    Q_PROPERTY(int rhythmTimerSkinTime READ rhythmTimerSkinTime NOTIFY rhythmTimerSkinTimeChanged)

public:
    explicit Lr2GameplayFrameState(QObject* parent = nullptr);

    QObject* chart() const;
    void setChart(QObject* chart);

    qreal progressPosition() const;
    qreal position1() const;
    qreal position2() const;
    qreal gaugeValue1() const;
    qreal gaugeValue2() const;
    QString activeGaugeName1() const;
    QString activeGaugeName2() const;
    int rhythmTimerSkinTime() const;

    Q_INVOKABLE void refresh(int frameSkinTime);
    Q_INVOKABLE void reset();

signals:
    void chartChanged();
    void progressPositionChanged();
    void position1Changed();
    void position2Changed();
    void gaugeValue1Changed();
    void gaugeValue2Changed();
    void activeGaugeName1Changed();
    void activeGaugeName2Changed();
    void rhythmTimerSkinTimeChanged();

private:
    struct GaugeSnapshot {
        qreal value = 0.0;
        QString name;
    };

    void cacheChartObjects();
    static GaugeSnapshot activeGaugeForPlayer(const gameplay_logic::Player* player);
    void setProgressPosition(qreal value);
    void setPosition1(qreal value);
    void setPosition2(qreal value);
    void setGaugeValue1(qreal value);
    void setGaugeValue2(qreal value);
    void setActiveGaugeName1(const QString& value);
    void setActiveGaugeName2(const QString& value);
    void setGaugeState1(const GaugeSnapshot& state);
    void setGaugeState2(const GaugeSnapshot& state);
    void setRhythmTimerSkinTime(int value);
    static bool sameReal(qreal lhs, qreal rhs);

    QPointer<QObject> m_chartObject;
    QPointer<gameplay_logic::ChartRunner> m_chart;
    QPointer<gameplay_logic::CourseRunner> m_course;
    QPointer<gameplay_logic::Player> m_player1;
    QPointer<gameplay_logic::Player> m_player2;
    bool m_useDoublePlayLanes = false;
    int m_lastProgressFrameSkinTime = -1;
    qreal m_progressPosition = 0.0;
    qreal m_position1 = 0.0;
    qreal m_position2 = 0.0;
    qreal m_gaugeValue1 = 0.0;
    qreal m_gaugeValue2 = 0.0;
    QString m_activeGaugeName1;
    QString m_activeGaugeName2;
    int m_rhythmTimerSkinTime = -1;
};
