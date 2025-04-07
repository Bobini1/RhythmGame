import RhythmGameQml
import QtQuick

Item {
    id: noteImage

    property bool active: false
    property string name: "beginner"
    property int playLevel

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
                asynchronous: true
                source: root.iniImagesUrl + "parts.png/" + noteImage.name
            }
            Image {
                id: note

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                asynchronous: true
                source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_note"
            }
            Image {
                id: text

                anchors.bottom: parent.bottom
                asynchronous: true
                source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_text"
            }
            TextureText {
                function getDiffColor(diff) {
                    switch (diff) {
                        case "beginner":
                            return "green";
                        case "normal":
                            return "blue";
                        case "hyper":
                            return "orange";
                        case "another":
                            return "red";
                        default:
                            return "purple";
                    }
                }
                anchors.horizontalCenter: circle.horizontalCenter
                anchors.top: circle.bottom
                number: noteImage.playLevel || 0
                srcBeforeDecimal: root.iniImagesUrl + "parts.png/l_" + getDiffColor(noteImage.name) + "_"
            }
        }
    }
    Component {
        id: inactiveComponent

        Image {
            id: circle

            anchors.bottom: parent.bottom
            asynchronous: true
            source: root.iniImagesUrl + "parts.png/" + noteImage.name + "_inactive"
        }
    }
    Loader {
        id: loader

        sourceComponent: noteImage.active ? activeComponent : inactiveComponent
    }
}

