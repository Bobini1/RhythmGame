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
        anchors.leftMargin: 48
        source: root.iniImagesUrl + "parts.png/" + lampDiff.oldBestClear + "_s"
    }
    Image {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
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
        anchors.rightMargin: 36
        source: root.iniImagesUrl + "parts.png/" + (clearTypesForCourse[lampDiff.clearType] || lampDiff.clearType)
    }
}
