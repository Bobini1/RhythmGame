import QtQuick
import RhythmGameQml

ListView {
    height: 200
    width: 200

    delegate: Rectangle {
        id: listItem

        required property string artist
        required property string title

        height: 25
        width: parent.width

        Text {
            text: listItem.artist + " - " + listItem.title
        }

        // switch to gameplay on click
        MouseArea {
            anchors.fill: parent

            onClicked: root.openChart(".")
        }
    }
    model: ListModel {
        ListElement {
            artist: "Artist 1"
            title: "Title 1"
        }
    }
}
