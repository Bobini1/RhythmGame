//
// Created by PC on 21/06/2025.
//

#include "CourseRunner.h"

namespace gameplay_logic {
CoursePlayer::CoursePlayer(QString guid, QObject* parent)
  : QObject(parent)
  , guid(std::move(guid))
{
}
QString
CoursePlayer::getGuid() const
{
    return guid;
}
int
CoursePlayer::getCombo() const
{
    return combo;
}
int
CoursePlayer::getMaxCombo() const
{
    return maxCombo;
}
void
CoursePlayer::setCombo(int newCombo)
{
    if (combo != newCombo) {
        combo = newCombo;
        emit comboChanged();
        if (combo > maxCombo) {
            maxCombo = combo;
            emit maxComboChanged();
        }
    }
}
void
CourseRunner::onStatusChanged()
{
    status = currentChart->getStatus();
    emit statusChanged();
}
void
CourseRunner::connectChart()
{
    connect(currentChart.get(),
            &ChartRunner::statusChanged,
            this,
            &CourseRunner::onStatusChanged);
    connect(currentChart->getPlayer1()->getScore(),
            &BmsLiveScore::comboIncreased,
            this,
            [this] {
                auto combo = coursePlayer1->getCombo();
                combo++;
                coursePlayer1->setCombo(combo);
            });
    connect(currentChart->getPlayer1()->getScore(),
            &BmsLiveScore::comboDropped,
            this,
            [this] { coursePlayer1->setCombo(0); });
    if (coursePlayer2) {
        connect(currentChart->getPlayer2()->getScore(),
                &BmsLiveScore::comboIncreased,
                this,
                [this] {
                    auto combo = coursePlayer2->getCombo();
                    combo++;
                    coursePlayer2->setCombo(combo);
                });
        connect(currentChart->getPlayer2()->getScore(),
                &BmsLiveScore::comboDropped,
                this,
                [this] { coursePlayer2->setCombo(0); });
    }
}
CourseRunner::CourseRunner(CoursePlayer* coursePlayer1,
                           CoursePlayer* coursePlayer2,
                           resource_managers::Course course,
                           QList<ChartData*> chartDatas,
                           LoadChart loadChart,
                           QObject* parent)
  : QObject(parent)
  , chartDatas(std::move(chartDatas))
  , course(std::move(course))
  , coursePlayer1(coursePlayer1)
  , coursePlayer2(coursePlayer2)
  , loadChart(std::move(loadChart))
  , currentChart(this->loadChart())
{
    for (const auto& chartData : this->chartDatas) {
        chartData->setParent(this);
    }
    coursePlayer1->setParent(this);
    if (coursePlayer2) {
        coursePlayer2->setParent(this);
    }
    connectChart();
}
auto
CourseRunner::getChartDatas() const -> QList<ChartData*>
{
    return chartDatas;
}
auto
CourseRunner::getCurrentChartIndex() const -> int
{
    return currentChartIndex;
}
auto
CourseRunner::getBga() const -> qml_components::BgaContainer*
{
    return currentChart ? currentChart->getBga() : nullptr;
}
auto
CourseRunner::getStatus() const -> ChartRunner::Status
{
    return status;
}
auto
CourseRunner::getPlayer1() const -> Player*
{
    return currentChart ? currentChart->getPlayer1() : nullptr;
}
auto
CourseRunner::getPlayer2() const -> Player*
{
    return currentChart ? currentChart->getPlayer2() : nullptr;
}
auto
CourseRunner::getCoursePlayer1() const -> CoursePlayer*
{
    return coursePlayer1;
}
auto
CourseRunner::getCoursePlayer2() const -> CoursePlayer*
{
    return coursePlayer2;
}
auto
CourseRunner::getCourse() const -> const resource_managers::Course&
{
    return course;
}
auto
CourseRunner::proceed() -> QList<BmsScore*>

{
    if (currentChartIndex == chartDatas.size() - 1) {
        return {nullptr};
    }
    const auto* p2 = getPlayer2();
    auto scores = QList<BmsScore*>{};
    if (currentChart) {
        scores = currentChart->finish();
        clear1 = QStringLiteral("FAILED");
        if (scores.size() > 0) {
            scores1.append(scores[0]);
            for (const auto* gauge :
                 currentChart->getPlayer1()->getScore()->getGauges()) {
                if (gauge->getGauge() > gauge->getThreshold()) {
                    clear1 = gauge->getName();
                    break;
                }
            }
        }
        clear2 = QStringLiteral("FAILED");
        if (scores.size() > 1) {
            scores2.append(scores[1]);
            for (const auto* gauge :
                 currentChart->getPlayer2()->getScore()->getGauges()) {
                if (gauge->getGauge() > gauge->getThreshold()) {
                    clear2 = gauge->getName();
                    break;
                }
            }
        }
        for (auto* score : scores) {
            if (score) {
                score->setParent(this);
            }
        }
    }
    currentChart = loadChart();
    emit bgaChanged();
    currentChartIndex++;
    emit currentChartIndexChanged();
    emit player1Changed();
    if (p2 != getPlayer2()) {
        emit player2Changed();
    }
    if (status != currentChart->getStatus()) {
        status = currentChart->getStatus();
        emit statusChanged();
    }
    connectChart();
    return scores;
}
auto
CourseRunner::finish() -> QList<BmsScoreCourse*>
{
    while (currentChartIndex < chartDatas.size() - 1) {
        proceed();
    }
    proceed();
    auto scores = QList<BmsScoreCourse*>{};
    if (scores1.contains(nullptr) || scores2.contains(nullptr)) {
        return { nullptr };
    }
    scores.append(new BmsScoreCourse{ coursePlayer1->getGuid(),
                                      course.getIdentifier(),
                                      std::move(scores1),
                                      clear1,
                                      coursePlayer1->getMaxCombo(),
                                      course.constraints,
                                      course.trophies });
    if (coursePlayer2) {
        scores.append(new BmsScoreCourse{ coursePlayer2->getGuid(),
                                          course.getIdentifier(),
                                          std::move(scores2),
                                          clear2,
                                          coursePlayer2->getMaxCombo(),
                                          course.constraints,
                                          course.trophies });
    }
    return scores;
}
void
CourseRunner::start() const
{
    if (currentChart) {
        currentChart->start();
    }
}
} // gameplay_logic