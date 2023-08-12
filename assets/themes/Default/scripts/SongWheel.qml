import QtQuick

ListView {
    height: 200
    width: 200

    delegate: Rectangle {
        required property string artist
        required property string title

        height: 25
        width: parent.width

        Text {
            text: artist + " - " + title
        }
    }
    model: ListModel {
        ListElement {
            artist: "The Beatles"
            title: "Yesterday"
        }
        ListElement {
            artist: "The Rolling Stones"
            title: "Angie"
        }
        ListElement {
            artist: "The Doors"
            title: "Light My Fire"
        }
    }
}
