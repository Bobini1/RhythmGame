import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    Component {
        id: horizontalLayout

        RowLayout {
            Gauge {
                id: gauge

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 10
            }
            LifeNumber {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                Layout.preferredWidth: hundredPercentWidth
            }
        }
    }
    Component {
        id: verticalLayout

        ColumnLayout {
            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.verticalStretchFactor: 10

                Gauge {
                    id: gauge
                    width: parent.height
                    height: parent.width
                    transformOrigin: Item.Center
                    anchors.centerIn: parent

                    rotation: 270
                }
            }
            LifeNumber {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.verticalStretchFactor: 1
            }
        }
    }
    Loader {
        active: true
        anchors.fill: parent
        sourceComponent: root.vars.verticalGauge ? verticalLayout : horizontalLayout
    }
}
