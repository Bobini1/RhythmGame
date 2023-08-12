import QtQuick

ListView {
    height: 200
    model: chartList
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
}
