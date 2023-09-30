//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_INPUTITEM_H
#define RHYTHMGAME_INPUTITEM_H

#include <QQuickItem>
#include "gameplay_logic/Chart.h"
namespace qml_components {
class InputItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(gameplay_logic::Chart* chart READ getChart WRITE setChart NOTIFY
                 chartChanged)
    gameplay_logic::Chart* chart = nullptr;

  public:
    explicit InputItem(QQuickItem* parent = nullptr);
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    auto getChart() const -> gameplay_logic::Chart*;
    void setChart(gameplay_logic::Chart* chart);

  signals:
    void chartChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_INPUTITEM_H
