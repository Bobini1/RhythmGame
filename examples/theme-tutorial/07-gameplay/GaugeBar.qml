import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts

ColumnLayout {
    id: bar
    required property var gauges

    // [active-gauge]
    readonly property var activeGauge: gauges.find(gauge => gauge.gauge > gauge.threshold)
        || gauges[gauges.length - 1]

    Label {
        Layout.fillWidth: true
        text: qsTr("%1: %2%").arg(bar.activeGauge.name).arg(bar.activeGauge.gauge.toFixed(1))
        elide: Text.ElideRight
    }
    ProgressBar {
        Layout.fillWidth: true
        from: 0
        to: bar.activeGauge.gaugeMax
        value: bar.activeGauge.gauge
    }
    // [active-gauge]
}
