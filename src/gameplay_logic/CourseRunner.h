//
// Created by PC on 21/06/2025.
//

#ifndef CHARTGROUP_H
#define CHARTGROUP_H
#include "ChartData.h"

#include <QObject>
#include "ChartRunner.h"
#include "qml_components/ChartLoader.h"

namespace gameplay_logic {

class CourseRunner final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ChartData*> chartDatas READ getChartDatas CONSTANT)
    Q_PROPERTY(int currentChartIndex READ getCurrentChartIndex NOTIFY
                 currentChartIndexChanged)
    Q_PROPERTY(qml_components::BgaContainer* bga READ getBga NOTIFY bgaChanged)
    Q_PROPERTY(Status status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(Player* player1 READ getPlayer1 NOTIFY player1Changed)
    Q_PROPERTY(Player* player2 READ getPlayer2 NOTIFY player2Changed)
    QList<resource_managers::ChartDataFactory::ChartComponents> chartComponents;
    int currentChartIndex{ 0 };
    qml_components::ChartLoader* chartLoader;
    ChartRunner* currentChart = nullptr;
    ChartRunner::Status status{ ChartRunner::Status::Loading };

  public:
    explicit CourseRunner(QList<resource_managers::ChartDataFactory::ChartComponents> chartComponents,
                          qml_components::ChartLoader* chartLoader,
                          QObject* parent = nullptr);

    auto getChartDatas() const -> QList<ChartData*>;
    auto getCurrentChartIndex() const -> int;
    auto getBga() const -> qml_components::BgaContainer*;
    auto getStatus() const -> ChartRunner::Status;
    auto getPlayer1() const -> Player*;
    auto getPlayer2() const -> Player*;

  signals:
    void currentChartIndexChanged();
    void bgaChanged();
    void statusChanged();
    void player1Changed();
    void player2Changed();
};

} // namespace gameplay_logic

#endif // CHARTGROUP_H
