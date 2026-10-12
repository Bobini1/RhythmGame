import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts

ColumnLayout {
    id: life
    required property var score

    // [gauge-selection]
    readonly property var gauges: score.gaugeHistory?.gaugeInfo ?? []
    readonly property var gauge: gauges[gaugeChoice.currentIndex] ?? null
    readonly property var history: gauge?.gaugeHistory ?? []
    readonly property real duration: Math.max(1, score.result.length,
        history[history.length - 1]?.offsetFromStart ?? 0)

    Label { text: qsTr("Life graph"); font.bold: true }
    ComboBox {
        id: gaugeChoice
        Layout.fillWidth: true
        model: life.gauges.map(gauge => gauge.name)
        currentIndex: {
            const matching = life.gauges.findIndex(gauge => gauge.name === life.score.result.clearType);
            return matching >= 0 ? matching : life.gauges.length - 1;
        }
        enabled: count > 0
        Accessible.name: qsTr("Gauge")
    }
    // [gauge-selection]

    // [gauge-points]
    readonly property var points: {
        if (history.length === 0 || !gauge || gauge.maxGauge <= 0) return [];
        const points = [];
        let previous = 1 - history[0].gauge / gauge.maxGauge;
        points.push(Qt.point(0, previous));
        for (const entry of history) {
            const x = Math.max(0, entry.offsetFromStart / duration);
            const y = 1 - entry.gauge / gauge.maxGauge;
            points.push(Qt.point(x, previous), Qt.point(x, y));
            previous = y;
        }
        points.push(Qt.point(1, previous));
        return points;
    }
    // [gauge-points]

    Rectangle {
        id: plot
        Layout.fillWidth: true
        Layout.preferredHeight: 160
        color: "#18232e"

        Rectangle {
            width: plot.width
            height: 1
            y: life.gauge ? 1 + (1 - life.gauge.threshold / life.gauge.maxGauge) * (plot.height - 2) : 0
            color: "#708090"
            visible: life.history.length > 0
        }

        // [gauge-path]
        Canvas {
            id: curve
            anchors.fill: parent
            anchors.margins: 1
            readonly property var points: life.points
            onPointsChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const context = getContext("2d");
                context.reset();
                if (points.length === 0) return;
                context.strokeStyle = "#a8dcc9";
                context.lineWidth = 2;
                context.beginPath();
                context.moveTo(points[0].x * width, points[0].y * height);
                for (const point of points)
                    context.lineTo(point.x * width, point.y * height);
                context.stroke();
            }
        }
        // [gauge-path]

        Label {
            anchors.fill: parent
            anchors.margins: 8
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: life.history.length === 0
            text: qsTr("Gauge history unavailable")
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("0 s") }
        Label {
            Layout.fillWidth: true
            text: qsTr("%1 s").arg(Math.ceil(life.duration / 1000000000))
            horizontalAlignment: Text.AlignRight
        }
    }
    Label {
        Layout.fillWidth: true
        text: life.gauge ? qsTr("Gauge 0 to %1, clear threshold %2")
            .arg(life.gauge.maxGauge).arg(life.gauge.threshold) : ""
        wrapMode: Text.Wrap
    }
}
