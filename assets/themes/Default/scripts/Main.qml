import QtQuick 2.15
import RhythmGameQml
import QtQuick.Controls.Basic 2.15
import QtQuick.Layouts

Pane {
    id: screen

    ColumnLayout {
        id: column

        width: parent.width * 0.4

        Button {
            id: button

            Layout.preferredHeight: 50
            text: qsTr("Song Wheel")

            onClicked: {
                sceneStack.push(root.songWheelComponent);
            }
        }
        Button {
            id: button1

            Layout.preferredHeight: 50
            text: qsTr("Settings")

            onClicked: {
                sceneStack.push(root.settingsComponent);
            }
        }
        Button {
            id: button2

            Layout.preferredHeight: 50
            text: qsTr("Button")
        }
    }
}
