import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: screen
    color: "#18232e"

    // [main-actions]
    StandardMainActions {
        id: actions
        enabled: screen.enabled
    }
    // [main-actions]

    // [canvas]
    Item {
        id: canvas
        width: 960
        height: 540
        anchors.centerIn: parent
        scale: Math.min(screen.width / canvas.width, screen.height / canvas.height)
        // [canvas]

        ColumnLayout {
            anchors.centerIn: parent
            width: 320
            spacing: 20

            // [image]
            Image {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                source: "images/notes.svg"
                sourceSize: Qt.size(320, 120)
                fillMode: Image.PreserveAspectFit
                Accessible.ignored: true
            }
            // [image]

            Label {
                Layout.fillWidth: true
                text: qsTr("My first theme")
                color: "white"
                font.pixelSize: 32
                horizontalAlignment: Text.AlignHCenter
            }

            // [buttons]
            Button {
                Layout.fillWidth: true
                text: qsTr("Song select")
                focus: true
                onClicked: actions.openSelect()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Settings")
                onClicked: actions.openSettings()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Quit")
                onClicked: actions.quit()
            }
            // [buttons]
        }
    }
}
