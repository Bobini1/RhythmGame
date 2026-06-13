#include "Lr2GameplayFrameState.h"

#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/CourseRunner.h"
#include "gameplay_logic/rules/BmsGauge.h"

#include <algorithm>
#include <cmath>

using gameplay_logic::ChartRunner;
using gameplay_logic::CourseRunner;
using gameplay_logic::Player;

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

qreal Lr2GameplayFrameState::gaugeValue1() const {
    return m_gaugeValue1;
}

qreal Lr2GameplayFrameState::gaugeValue2() const {
    return m_gaugeValue2;
}

QString Lr2GameplayFrameState::activeGaugeName1() const {
    return m_activeGaugeName1;
}

QString Lr2GameplayFrameState::activeGaugeName2() const {
    return m_activeGaugeName2;
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
    Player* lanePlayer2 = m_player2
        ? m_player2.data()
        : (m_useDoublePlayLanes ? m_player1.data() : nullptr);
    const bool useLiveSample = m_chart && m_chart->getStatus() == ChartRunner::Running;
    const auto liveOffset = useLiveSample
        ? m_chart->currentOffsetFromStart()
        : std::chrono::nanoseconds::zero();

    const qreal elapsed = progressPlayer
        ? std::max<qreal>(
            0.0,
            useLiveSample
                ? static_cast<qreal>(liveOffset.count())
                : static_cast<qreal>(progressPlayer->getElapsed()))
        : 0.0;
    const qreal length = progressPlayer
        ? std::max<qreal>(0.0, static_cast<qreal>(progressPlayer->getChartLength()))
        : 0.0;
    const qreal progress = length > 0.0 ? std::clamp(elapsed / length, 0.0, 1.0) : 0.0;
    if (m_lastProgressFrameSkinTime < 0
            || frameSkinTime < m_lastProgressFrameSkinTime
            || frameSkinTime - m_lastProgressFrameSkinTime >= 16
            || progress <= 0.0
            || progress >= 1.0) {
        m_lastProgressFrameSkinTime = frameSkinTime;
        setProgressPosition(progress);
    }

    setPosition1(lanePlayer1
        ? (useLiveSample ? lanePlayer1->positionAt(liveOffset) : lanePlayer1->getPosition())
        : 0.0);
    setPosition2(lanePlayer2
        ? (useLiveSample ? lanePlayer2->positionAt(liveOffset) : lanePlayer2->getPosition())
        : 0.0);
    setGaugeState1(activeGaugeForPlayer(lanePlayer1));
    setGaugeState2(activeGaugeForPlayer(lanePlayer2));

    Player* rhythmPlayer = progressPlayer ? progressPlayer : m_player2.data();
    int rhythmTimer = -1;
    if (rhythmPlayer) {
        const qreal beatPosition = useLiveSample
            ? rhythmPlayer->beatPositionAt(liveOffset)
            : rhythmPlayer->getBeatPosition();
        if (beatPosition > 0.0) {
            const qreal rhythm = (beatPosition - std::floor(beatPosition)) * 1000.0;
            rhythmTimer = std::max(0, static_cast<int>(std::llround(frameSkinTime - rhythm)));
        } else {
            const qreal bpm = std::max<qreal>(1.0, rhythmPlayer->getBpm());
            const qreal elapsedMs = std::max<qreal>(
                0.0,
                useLiveSample
                    ? static_cast<qreal>(liveOffset.count())
                    : static_cast<qreal>(rhythmPlayer->getElapsed())) / 1000000.0;
            const qreal beatMs = 60000.0 / bpm;
            const qreal rhythm = beatMs > 0.0
                ? std::fmod(elapsedMs, beatMs) * 1000.0 / beatMs
                : 0.0;
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
    setGaugeState1({});
    setGaugeState2({});
    setRhythmTimerSkinTime(-1);
}

void Lr2GameplayFrameState::cacheChartObjects() {
    m_chart = qobject_cast<ChartRunner*>(m_chartObject);
    m_course = nullptr;
    if (m_chart) {
        m_player1 = m_chart->getPlayer1();
        m_player2 = m_chart->getPlayer2();
        m_useDoublePlayLanes = gameplay_logic::isDp(m_chart->getKeymode());
        return;
    }

    m_course = qobject_cast<CourseRunner*>(m_chartObject);
    if (m_course) {
        m_player1 = m_course->getPlayer1();
        m_player2 = m_course->getPlayer2();
        m_useDoublePlayLanes = gameplay_logic::isDp(m_course->getKeymode());
        return;
    }

    m_player1 = nullptr;
    m_player2 = nullptr;
    m_useDoublePlayLanes = false;
}

Lr2GameplayFrameState::GaugeSnapshot Lr2GameplayFrameState::activeGaugeForPlayer(const Player* player) {
    if (!player || !player->getScore()) {
        return {};
    }

    const auto gauges = player->getScore()->getGauges();
    if (gauges.empty()) {
        return {};
    }

    const gameplay_logic::rules::BmsGauge* selected = nullptr;
    for (const auto* gauge : gauges) {
        if (!gauge) {
            continue;
        }
        if (gauge->getGauge() > gauge->getThreshold()) {
            selected = gauge;
            break;
        }
    }
    if (!selected) {
        selected = gauges.back();
    }
    return selected
        ? GaugeSnapshot{ selected->getGauge(), selected->getName().toUpper() }
        : GaugeSnapshot{};
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

void Lr2GameplayFrameState::setGaugeValue1(qreal value) {
    if (sameReal(m_gaugeValue1, value)) {
        return;
    }
    m_gaugeValue1 = value;
    emit gaugeValue1Changed();
}

void Lr2GameplayFrameState::setGaugeValue2(qreal value) {
    if (sameReal(m_gaugeValue2, value)) {
        return;
    }
    m_gaugeValue2 = value;
    emit gaugeValue2Changed();
}

void Lr2GameplayFrameState::setActiveGaugeName1(const QString& value) {
    if (m_activeGaugeName1 == value) {
        return;
    }
    m_activeGaugeName1 = value;
    emit activeGaugeName1Changed();
}

void Lr2GameplayFrameState::setActiveGaugeName2(const QString& value) {
    if (m_activeGaugeName2 == value) {
        return;
    }
    m_activeGaugeName2 = value;
    emit activeGaugeName2Changed();
}

void Lr2GameplayFrameState::setGaugeState1(const GaugeSnapshot& state) {
    setGaugeValue1(state.value);
    setActiveGaugeName1(state.name);
}

void Lr2GameplayFrameState::setGaugeState2(const GaugeSnapshot& state) {
    setGaugeValue2(state.value);
    setActiveGaugeName2(state.name);
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
