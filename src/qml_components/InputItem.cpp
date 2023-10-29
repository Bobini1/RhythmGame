//
// Created by bobini on 30.09.23.
//

#include <QQuickItem>
#include "InputItem.h"
#include <QQuickPaintedItem>
qml_components::InputItem::InputItem(QQuickItem* parent)
  : QQuickItem(parent)
{
    // set flags to accept key inputs
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    setFlag(QQuickItem::ItemIsFocusScope, true);
    setFocus(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

auto
qml_components::InputItem::getChart() const -> gameplay_logic::Chart*
{
    return chart;
}
void
qml_components::InputItem::setChart(gameplay_logic::Chart* newChart)
{
    if (newChart == chart) {
        return;
    }
    chart = newChart;
    emit chartChanged();
}
void
qml_components::InputItem::focusInEvent(QFocusEvent* event)
{
    forceActiveFocus();
    QQuickItem::focusInEvent(event);
}
void
qml_components::InputItem::keyPressEvent(QKeyEvent* event)
{
    if (chart == nullptr) {
        return;
    }
    chart->passKey(event, gameplay_logic::Chart::EventType::KeyPress);
}
void
qml_components::InputItem::keyReleaseEvent(QKeyEvent* event)
{
    if (chart == nullptr) {
        return;
    }
    chart->passKey(event, gameplay_logic::Chart::EventType::KeyRelease);
}
