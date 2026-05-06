#include "Lr2GameplayFrameState.h"

#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/ChartData.h"

#include <algorithm>
#include <cmath>

using gameplay_logic::ChartData;
using gameplay_logic::ChartRunner;
using gameplay_logic::Player;

namespace {
Player* playerFromProperty(QObject* chart, const char* name) {
    if (!chart) {
        return nullptr;
    }
    return qobject_cast<Player*>(chart->property(name).value<QObject*>());
}

bool doublePlayKeymode(int keymode) {
    return keymode == static_cast<int>(ChartData::Keymode::K10)
        || keymode == static_cast<int>(ChartData::Keymode::K14);
}
} // namespace

Lr2GameplayFrameState::Lr2GameplayFrameState(QObject* parent) : QObject(parent) {}

QObject* Lr2GameplayFrameState::chart() const {
    return m_chartObject;
}

void Lr2GameplayFrameState::setChart(QObject* chart) {
    if (m_chartObject == chart) {
        return;
    }

    m_chartObject = chart;
    cacheChartObjects();
    emit chartChanged();

    if (!m_chartObject) {
        reset();
    }
}

qreal Lr2GameplayFrameState::progressPosition() const {
    return m_progressPosition;
}

qreal Lr2GameplayFrameState::position1() const {
    return m_position1;
}

qreal Lr2GameplayFrameState::position2() const {
    return m_position2;
}

int Lr2GameplayFrameState::rhythmTimerSkinTime() const {
    return m_rhythmTimerSkinTime;
}

void Lr2GameplayFrameState::refresh(int frameSkinTime) {
    if (!m_chartObject) {
        reset();
        return;
    }

    if (!m_player1 && !m_player2) {
        cacheChartObjects();
    }

    Player* progressPlayer = m_player1;
    Player* lanePlayer1 = m_player1;
    Player* lanePlayer2 = m_player2 ? m_player2.data() : (m_useDoublePlayLanes ? m_player1.data() : nullptr);

    const qreal elapsed = progressPlayer ? std::max<qreal>(0.0, progressPlayer->getElapsed()) : 0.0;
    const qreal length = progressPlayer ? std::max<qreal>(0.0, progressPlayer->getChartLength()) : 0.0;
    const qreal progress = length > 0.0 ? std::clamp(elapsed / length, 0.0, 1.0) : 0.0;
    if (m_lastProgressFrameSkinTime < 0
            || frameSkinTime < m_lastProgressFrameSkinTime
            || frameSkinTime - m_lastProgressFrameSkinTime >= 16
            || progress <= 0.0
            || progress >= 1.0) {
        m_lastProgressFrameSkinTime = frameSkinTime;
        setProgressPosition(progress);
    }

    setPosition1(lanePlayer1 ? lanePlayer1->getPosition() : 0.0);
    setPosition2(lanePlayer2 ? lanePlayer2->getPosition() : 0.0);

    Player* rhythmPlayer = progressPlayer ? progressPlayer : m_player2.data();
    int rhythmTimer = -1;
    if (rhythmPlayer) {
        const qreal beatPosition = rhythmPlayer->getBeatPosition();
        if (beatPosition > 0.0) {
            const qreal rhythm = (beatPosition - std::floor(beatPosition)) * 1000.0;
            rhythmTimer = std::max(0, static_cast<int>(std::llround(frameSkinTime - rhythm)));
        } else {
            const qreal bpm = std::max<qreal>(1.0, rhythmPlayer->getBpm());
            const qreal elapsedMs = std::max<qreal>(0.0, rhythmPlayer->getElapsed()) / 1000000.0;
            const qreal beatMs = 60000.0 / bpm;
            const qreal rhythm = beatMs > 0.0 ? std::fmod(elapsedMs, beatMs) * 1000.0 / beatMs : 0.0;
            rhythmTimer = std::max(0, static_cast<int>(std::llround(frameSkinTime - rhythm)));
        }
    }
    setRhythmTimerSkinTime(rhythmTimer);
}

void Lr2GameplayFrameState::reset() {
    m_lastProgressFrameSkinTime = -1;
    setProgressPosition(0.0);
    setPosition1(0.0);
    setPosition2(0.0);
    setRhythmTimerSkinTime(-1);
}

void Lr2GameplayFrameState::cacheChartObjects() {
    m_chart = qobject_cast<ChartRunner*>(m_chartObject);
    if (m_chart) {
        m_player1 = m_chart->getPlayer1();
        m_player2 = m_chart->getPlayer2();
        m_useDoublePlayLanes = gameplay_logic::isDp(m_chart->getKeymode());
        return;
    }

    m_player1 = playerFromProperty(m_chartObject, "player1");
    m_player2 = playerFromProperty(m_chartObject, "player2");
    m_useDoublePlayLanes = m_chartObject && doublePlayKeymode(m_chartObject->property("keymode").toInt());
}

void Lr2GameplayFrameState::setProgressPosition(qreal value) {
    if (sameReal(m_progressPosition, value)) {
        return;
    }
    m_progressPosition = value;
    emit progressPositionChanged();
}

void Lr2GameplayFrameState::setPosition1(qreal value) {
    if (sameReal(m_position1, value)) {
        return;
    }
    m_position1 = value;
    emit position1Changed();
}

void Lr2GameplayFrameState::setPosition2(qreal value) {
    if (sameReal(m_position2, value)) {
        return;
    }
    m_position2 = value;
    emit position2Changed();
}

void Lr2GameplayFrameState::setRhythmTimerSkinTime(int value) {
    if (m_rhythmTimerSkinTime == value) {
        return;
    }
    m_rhythmTimerSkinTime = value;
    emit rhythmTimerSkinTimeChanged();
}

bool Lr2GameplayFrameState::sameReal(qreal lhs, qreal rhs) {
    return std::abs(lhs - rhs) < 0.000001;
}
