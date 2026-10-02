import RhythmGameQml
import QtQuick 2.0

Image {
    id: sortButton

    required property StandardSelectController selection

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: qsTr("Sort: %1").arg(sortButton.selection.sortModeText)
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: sortButton.selection.cycleSortMode(1)
    }
}
