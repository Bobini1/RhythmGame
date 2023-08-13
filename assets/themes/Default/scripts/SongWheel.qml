import QtQuick
import RhythmGameQml

ListView {
    height: 200
    model: ListView {
        
    }
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
    }
}
