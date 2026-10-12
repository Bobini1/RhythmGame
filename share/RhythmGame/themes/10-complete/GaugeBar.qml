import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts

ColumnLayout {
    id: bar
    required property var gauges

    // [active-gauge]
    readonly property var activeGauge: gauges.find(gauge => gauge.gauge > gauge.threshold)
        || gauges[gauges.length - 1] || null

    Label {
        Layout.fillWidth: true
        text: bar.activeGauge
            ? qsTr("%1: %2%").arg(bar.activeGauge.name).arg(bar.activeGauge.gauge.toFixed(1)) : ""
        elide: Text.ElideRight
    }
    ProgressBar {
        Layout.fillWidth: true
        from: 0
        to: bar.activeGauge?.gaugeMax || 100
        value: bar.activeGauge?.gauge || 0
    }
    // [active-gauge]
}
