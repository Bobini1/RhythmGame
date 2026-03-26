import QtQuick

WindowBg {
    id: lampDiff

    required property var oldBestClear
    required property var clearType

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
        anchors.leftMargin: 40
        source: root.iniImagesUrl + "parts.png/" + lampDiff.oldBestClear + "_s"
    }
    Image {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -12
        source: root.iniImagesUrl + "parts.png/arrow"
    }
    Image {
        id: newLamp

        property var clearTypesForCourse: {
            "DAN": "NORMAL",
            "EXDAN": "HARD",
            "EXHARDDAN": "EXHARD",
        }

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.right: parent.right
        anchors.rightMargin: 32
        source: root.iniImagesUrl + "parts.png/" + (clearTypesForCourse[lampDiff.clearType] || lampDiff.clearType)
    }
}
