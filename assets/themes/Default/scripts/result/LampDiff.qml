import QtQuick

WindowBg {
    id: lampDiff

    Image {
        id: lampText

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        source: root.iniImagesUrl + "parts.png/lamp"
    }
    Image {
        id: oldLamp

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.left: parent.left
        anchors.leftMargin: 48
        source: root.iniImagesUrl + "parts.png/" + root.oldBestClear + "_s"
    }
    Image {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        source: root.iniImagesUrl + "parts.png/arrow"
    }
    Image {
        id: newLamp

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 36
        source: root.iniImagesUrl + "parts.png/" + result.result.clearType
    }
}
