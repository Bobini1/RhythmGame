import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: lifeBar

    required property string gaugeImage
    required property var score
    required property bool verticalGauge

    Component {
        id: horizontalLayout

        RowLayout {
            Gauge {
                id: gauge

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 10
                gaugeImage: lifeBar.gaugeImage
                gauges: lifeBar.score.gauges
            }
            LifeNumber {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                Layout.preferredWidth: hundredPercentWidth
                score: lifeBar.score
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

                    anchors.centerIn: parent
                    gaugeImage: lifeBar.gaugeImage
                    height: parent.width
                    rotation: 270
                    transformOrigin: Item.Center
                    width: parent.height
                    gauges: lifeBar.score.gauges
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
        sourceComponent: lifeBar.verticalGauge ? verticalLayout : horizontalLayout
    }
}
