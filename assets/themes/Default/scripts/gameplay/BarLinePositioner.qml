import QtQuick 2.0

Item {
    id: column

    property var barLines: []
    required property real heightMultiplier
    property int erasedBarIndex: 0
    property int visibleBarIndex: 0

    ListModel {
        id: barModel

        Component.onCompleted: {
            for (let i = 0; i < column.barLines.length; i++) {
                barModel.append({
                    "bar": i
                });
            }
        }
    }

    Repeater {
        id: barLineRepeater

        model: barModel

        Rectangle {
            color: "gray"
            height: 1
            width: parent.width
            visible: false
            y: -column.barLines[bar].position * column.heightMultiplier - height / 2
        }
    }
    Connections {
        function onPositionChanged(_) {
            let count = 0;
            let chartPosition = chart.position;
            while (column.erasedBarIndex + count < column.barLines.length) {
                let bar = column.barLines[column.erasedBarIndex + count];
                if (bar.position > chartPosition) {
                    break;
                }
                count++;
            }
            if (count > 0) {
                barModel.remove(0, count);
            }
            column.erasedBarIndex += count;
            column.visibleBarIndex -= count;
            column.visibleBarIndex = Math.max(0, column.visibleBarIndex);
            let visibleBarIndex = column.visibleBarIndex;
            count = 0;
            while (visibleBarIndex + count < barLineRepeater.count) {
                let bar = barLineRepeater.itemAt(visibleBarIndex + count);
                let globalPos = bar.mapToItem(playObjectContainer, 0, bar.height);
                if (globalPos.y > 0) {
                    bar.visible = true;
                    count++;
                } else {
                    break;
                }
            }
            column.visibleBarIndex += count;
        }

        target: chart
    }
}
