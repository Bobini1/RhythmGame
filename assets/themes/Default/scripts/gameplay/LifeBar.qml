import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    RowLayout {
        id: lifeBarLayout
        anchors.fill: parent

        Gauge {
            id: gauge

            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 10
            Layout.fillHeight: true
        }
        LifeNumber {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 1
            Layout.fillHeight: true
            Layout.preferredWidth: hundredPercentWidth
        }
    }
}
