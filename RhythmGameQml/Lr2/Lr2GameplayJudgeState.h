#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <cstddef>

class Lr2GameplayJudgeState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
      int lastJudgeTiming1 READ lastJudgeTiming1 NOTIFY lastJudgeTiming1Changed)
    Q_PROPERTY(
      int lastJudgeTiming2 READ lastJudgeTiming2 NOTIFY lastJudgeTiming2Changed)
    Q_PROPERTY(
      int judgeNowValue1 READ judgeNowValue1 NOTIFY judgeNowValue1Changed)
    Q_PROPERTY(
      int judgeNowValue2 READ judgeNowValue2 NOTIFY judgeNowValue2Changed)

  public:
    explicit Lr2GameplayJudgeState(QObject* parent = nullptr);

    int lastJudgeTiming1() const;
    int lastJudgeTiming2() const;
    int judgeNowValue1() const;
    int judgeNowValue2() const;

    Q_INVOKABLE void record(int scoreSide,
                            int judgement,
                            qint64 deviationNanos,
                            int lane);
    Q_INVOKABLE void reset();
    Q_INVOKABLE int judgeValueForId(int numberId) const;
    Q_INVOKABLE int timingNumber(int numberId, int scoreSide) const;
    Q_INVOKABLE qreal timingMean(int scoreSide) const;
    Q_INVOKABLE qreal timingStdDev(int scoreSide) const;

  signals:
    void lastJudgeTiming1Changed();
    void lastJudgeTiming2Changed();
    void judgeNowValue1Changed();
    void judgeNowValue2Changed();

  private:
    static constexpr int laneCount = 100;

    struct SideState
    {
        std::array<int, 6> earlyCounts{};
        std::array<int, 6> lateCounts{};
        std::array<int, laneCount> laneValues{};
        qint64 timingSum = 0;
        qint64 timingSumSquares = 0;
        int timingCount = 0;
        int lastJudgeTiming = 0;
        int judgeNowValue = 0;
    };

    static std::size_t sideIndex(int scoreSide);
    static int timingBucket(int judgement);
    static int nowJudgeValue(int judgement);
    static int laneJudgeValue(int judgement, int timing);
    int timingCount(std::size_t side, int bucket, bool early) const;
    void setLastJudgeTiming(std::size_t side, int value);
    void setJudgeNowValue(std::size_t side, int value);

    std::array<SideState, 2> m_sides;
};
