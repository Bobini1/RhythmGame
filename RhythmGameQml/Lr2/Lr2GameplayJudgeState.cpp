#include "Lr2GameplayJudgeState.h"

#include "gameplay_logic/Judgement.h"

#include <algorithm>
#include <cmath>

using gameplay_logic::Judgement;

Lr2GameplayJudgeState::Lr2GameplayJudgeState(QObject* parent)
  : QObject(parent)
{
}

int
Lr2GameplayJudgeState::lastJudgeTiming1() const
{
    return m_sides[0].lastJudgeTiming;
}

int
Lr2GameplayJudgeState::lastJudgeTiming2() const
{
    return m_sides[1].lastJudgeTiming;
}

int
Lr2GameplayJudgeState::judgeNowValue1() const
{
    return m_sides[0].judgeNowValue;
}

int
Lr2GameplayJudgeState::judgeNowValue2() const
{
    return m_sides[1].judgeNowValue;
}

void
Lr2GameplayJudgeState::record(int scoreSide,
                              int judgement,
                              qint64 deviationNanos,
                              int lane)
{
    const auto side = sideIndex(scoreSide);
    const int bucket = timingBucket(judgement);
    if (bucket < 0) {
        return;
    }

    SideState& state = m_sides[side];
    auto& counts = deviationNanos < 0 ? state.earlyCounts : state.lateCounts;
    ++counts[static_cast<std::size_t>(bucket)];

    const qreal timingValue = -static_cast<qreal>(deviationNanos) / 1'000'000.0;
    const int timing = static_cast<int>(std::floor(timingValue + 0.5));
    if (judgement >= static_cast<int>(Judgement::Bad) &&
        judgement <= static_cast<int>(Judgement::Perfect)) {
        setLastJudgeTiming(side, timing);
        ++state.timingCount;
        state.timingSum += timing;
        state.timingSumSquares += static_cast<qint64>(timing) * timing;
    }

    setJudgeNowValue(side, nowJudgeValue(judgement));
    const int value = laneJudgeValue(judgement, timing);
    if (value > 0 && lane >= 0 && lane < laneCount) {
        state.laneValues[static_cast<std::size_t>(lane)] = value;
    }
}

void
Lr2GameplayJudgeState::reset()
{
    const auto previous = m_sides;
    m_sides = {};
    if (previous[0].lastJudgeTiming != 0) {
        emit lastJudgeTiming1Changed();
    }
    if (previous[1].lastJudgeTiming != 0) {
        emit lastJudgeTiming2Changed();
    }
    if (previous[0].judgeNowValue != 0) {
        emit judgeNowValue1Changed();
    }
    if (previous[1].judgeNowValue != 0) {
        emit judgeNowValue2Changed();
    }
}

int
Lr2GameplayJudgeState::judgeValueForId(int numberId) const
{
    if (numberId == 520 || numberId == 522) {
        return m_sides[0].judgeNowValue;
    }
    if (numberId == 521) {
        return m_sides[1].judgeNowValue;
    }

    std::size_t side = 0;
    int offset = -1;
    if (numberId >= 500 && numberId <= 519) {
        side = static_cast<std::size_t>((numberId - 500) / 10);
        offset = (numberId - 500) % 10;
    } else if (numberId >= 1510 && numberId <= 1599) {
        offset = numberId - 1510 + 10;
    } else if (numberId >= 1610 && numberId <= 1699) {
        side = 1;
        offset = numberId - 1610 + 10;
    }
    return offset >= 0 && offset < laneCount
             ? m_sides[side].laneValues[static_cast<std::size_t>(offset)]
             : 0;
}

int
Lr2GameplayJudgeState::timingNumber(int numberId, int scoreSide) const
{
    const auto side = sideIndex(scoreSide);
    if (numberId >= 410 && numberId <= 419) {
        const int bucket = (numberId - 410) / 2;
        const bool early = (numberId - 410) % 2 == 0;
        return timingCount(side, bucket, early);
    }
    if (numberId == 421) {
        return timingCount(side, 5, true);
    }
    if (numberId == 422) {
        return timingCount(side, 5, false);
    }
    if (numberId == 423 || numberId == 424) {
        const bool early = numberId == 423;
        int total = 0;
        for (int bucket = 0; bucket <= 3; ++bucket) {
            total += timingCount(side, bucket, early);
        }
        return total;
    }
    return 0;
}

qreal
Lr2GameplayJudgeState::timingMean(int scoreSide) const
{
    const SideState& state = m_sides[sideIndex(scoreSide)];
    return state.timingCount > 0
             ? static_cast<qreal>(state.timingSum) / state.timingCount
             : 0.0;
}

qreal
Lr2GameplayJudgeState::timingStdDev(int scoreSide) const
{
    const SideState& state = m_sides[sideIndex(scoreSide)];
    if (state.timingCount <= 0) {
        return 0.0;
    }
    const qreal mean = static_cast<qreal>(state.timingSum) / state.timingCount;
    const qreal variance =
      static_cast<qreal>(state.timingSumSquares) / state.timingCount -
      mean * mean;
    return std::sqrt(std::max<qreal>(0.0, variance));
}

std::size_t
Lr2GameplayJudgeState::sideIndex(int scoreSide)
{
    return scoreSide == 2 ? std::size_t{ 1 } : std::size_t{ 0 };
}

int
Lr2GameplayJudgeState::timingBucket(int judgement)
{
    switch (static_cast<Judgement>(judgement)) {
        case Judgement::Perfect:
            return 0;
        case Judgement::Great:
            return 1;
        case Judgement::Good:
            return 2;
        case Judgement::Bad:
            return 3;
        default:
            return -1;
    }
}

int
Lr2GameplayJudgeState::nowJudgeValue(int judgement)
{
    switch (static_cast<Judgement>(judgement)) {
        case Judgement::Perfect:
            return 1;
        case Judgement::Great:
            return 2;
        case Judgement::Good:
            return 3;
        case Judgement::Bad:
            return 4;
        case Judgement::Poor:
            return 5;
        case Judgement::EmptyPoor:
            return 6;
        default:
            return 0;
    }
}

int
Lr2GameplayJudgeState::laneJudgeValue(int judgement, int timing)
{
    switch (static_cast<Judgement>(judgement)) {
        case Judgement::Perfect:
            return 1;
        case Judgement::Great:
            return timing > 0 ? 2 : 3;
        case Judgement::Good:
            return timing > 0 ? 4 : 5;
        case Judgement::Bad:
            return timing > 0 ? 6 : 7;
        default:
            return 0;
    }
}

int
Lr2GameplayJudgeState::timingCount(std::size_t side,
                                   int bucket,
                                   bool early) const
{
    if (bucket < 0 ||
        bucket >= static_cast<int>(m_sides[side].earlyCounts.size())) {
        return 0;
    }
    const auto& counts =
      early ? m_sides[side].earlyCounts : m_sides[side].lateCounts;
    return counts[static_cast<std::size_t>(bucket)];
}

void
Lr2GameplayJudgeState::setLastJudgeTiming(std::size_t side, int value)
{
    if (m_sides[side].lastJudgeTiming == value) {
        return;
    }
    m_sides[side].lastJudgeTiming = value;
    if (side == 0) {
        emit lastJudgeTiming1Changed();
    } else {
        emit lastJudgeTiming2Changed();
    }
}

void
Lr2GameplayJudgeState::setJudgeNowValue(std::size_t side, int value)
{
    if (m_sides[side].judgeNowValue == value) {
        return;
    }
    m_sides[side].judgeNowValue = value;
    if (side == 0) {
        emit judgeNowValue1Changed();
    } else {
        emit judgeNowValue2Changed();
    }
}
