//
// Created by PC on 21/06/2025.
//

#include "CourseRunner.h"

namespace gameplay_logic {
CourseRunner::CourseRunner(QList<resource_managers::ChartDataFactory::ChartComponents> chartComponents,
                          qml_components::ChartLoader* chartLoader,
                          QObject* parent)
                              : QObject(parent),
                                chartComponents(std::move(chartComponents)),
                                chartLoader(chartLoader)
{
    for (const auto& chartData : this->chartComponents) {
        chartData.chartData->setParent(this);
    }
    chartLoader->loadCourseChart(
        chartComponents,
        chartLoader->getProfileList()->getMainProfile(),
        chartLoader->getProfileList()->getMainProfile()->isAutoPlayEnabled(),
        nullptr, // No replayed score for course
        std::nullopt // No second player
    );
}
auto
CourseRunner::getChartDatas() const -> QList<ChartData*>
{
    auto ret = QList<ChartData*>{};
    ret.reserve(chartComponents.size());
    for (const auto& chartData : chartComponents) {
        ret.append(chartData.chartData.get());
    }
    return ret;
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
} // gameplay_logic