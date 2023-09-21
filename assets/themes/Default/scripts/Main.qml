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
            Layout.preferredHeight: 50
            text: qsTr("Song Wheel")

            onClicked: {
                sceneStack.push(globalRoot.songWheelComponent);
            }
        }
        Button {
            Layout.preferredHeight: 50
            text: qsTr("Settings")

            onClicked: {
                sceneStack.push(globalRoot.settingsComponent);
            }
        }
        Button {
            Layout.preferredHeight: 50
            text: qsTr("Quit")

            onClicked: {
                Qt.quit();
            }
        }
    }
}
