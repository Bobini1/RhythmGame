import RhythmGameQml
import QtQuick

Item {
    id: noteImage

    property bool active: false
    property string name: "beginner"

    height: 74
    width: 105

    Component {
        id: activeComponent

        Item {
            anchors.fill: parent

            Image {
                id: circle

                anchors.bottom: parent.bottom
                anchors.bottomMargin: -3
                source: root.iniImagesUrl + "parts.png/" + noteImage.name
            }
            Image {
                id: note

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_note"
            }
            Image {
                id: text

                anchors.bottom: parent.bottom
                source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_text"
            }
        }
    }
    Component {
        id: inactiveComponent

        Image {
            id: circle

            anchors.bottom: parent.bottom
            source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_inactive"
        }
    }
    Loader {
        id: loader

        sourceComponent: noteImage.active ? activeComponent : inactiveComponent
    }
}

