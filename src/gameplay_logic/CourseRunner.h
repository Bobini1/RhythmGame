//
// Created by PC on 21/06/2025.
//

#ifndef CHARTGROUP_H
#define CHARTGROUP_H
#include "BmsScoreCourse.h"
#include "ChartData.h"

#include <QObject>
#include "ChartRunner.h"

namespace gameplay_logic {

class CoursePlayer final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    QString guid{};
    int combo{};
    int maxCombo{};

  public:
    explicit CoursePlayer(QString guid, QObject* parent = nullptr);
    QString getGuid() const;
    int getCombo() const;
    int getMaxCombo() const;
    void setCombo(int combo);
  signals:
    void comboChanged();
    void maxComboChanged();
};

class CourseRunner final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(resource_managers::Course course READ getCourse CONSTANT)
    Q_PROPERTY(QList<ChartData*> chartDatas READ getChartDatas CONSTANT)
    Q_PROPERTY(int currentChartIndex READ getCurrentChartIndex NOTIFY
                 currentChartIndexChanged)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY bgaChanged)
    Q_PROPERTY(gameplay_logic::ChartRunner::Status status READ getStatus NOTIFY
                 statusChanged)
    Q_PROPERTY(
      gameplay_logic::Player* player1 READ getPlayer1 NOTIFY player1Changed)
    Q_PROPERTY(
      gameplay_logic::Player* player2 READ getPlayer2 NOTIFY player2Changed)
    Q_PROPERTY(gameplay_logic::CoursePlayer* coursePlayer1 READ getCoursePlayer1
                 CONSTANT)
    Q_PROPERTY(gameplay_logic::CoursePlayer* coursePlayer2 READ getCoursePlayer2
                 CONSTANT)
    QList<ChartData*> chartDatas;
    int currentChartIndex{ 0 };
    ChartRunner::Status status{ ChartRunner::Status::Loading };
    QList<BmsScore*> scores1;
    QList<BmsScore*> scores2;
    resource_managers::Course course;
    CoursePlayer* coursePlayer1;
    CoursePlayer* coursePlayer2;
    QString clear1 = QStringLiteral("FAILED");
    QString clear2 = QStringLiteral("FAILED");
    void onStatusChanged();
    void connectChart();

  public:
    using LoadChart = std::function<std::unique_ptr<ChartRunner>()>;

  private:
    LoadChart loadChart;
    std::unique_ptr<ChartRunner> currentChart;

  public:
    explicit CourseRunner(CoursePlayer* coursePlayer1,
                          CoursePlayer* coursePlayer2,
                          resource_managers::Course course,
                          QList<ChartData*> chartDatas,
                          LoadChart loadChart,
                          QObject* parent = nullptr);

    auto getChartDatas() const -> QList<ChartData*>;
    auto getCurrentChartIndex() const -> int;
    auto getBga() const -> qml_components::BgaContainer*;
    auto getStatus() const -> ChartRunner::Status;
    auto getPlayer1() const -> Player*;
    auto getPlayer2() const -> Player*;
    auto getCoursePlayer1() const -> CoursePlayer*;
    auto getCoursePlayer2() const -> CoursePlayer*;
    auto getCourse() const -> const resource_managers::Course&;
    Q_INVOKABLE QList<BmsScore*> proceed();
    Q_INVOKABLE QList<BmsScoreCourse*> finish();
    Q_INVOKABLE void start() const;

  signals:
    void currentChartIndexChanged();
    void bgaChanged();
    void statusChanged();
    void player1Changed();
    void player2Changed();
};

} // namespace gameplay_logic

#endif // CHARTGROUP_H
