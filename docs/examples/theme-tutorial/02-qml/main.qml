import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: screen

    // [binding]
    readonly property color accent: warmColors.checked ? "#e8bc89" : "#a8dcc9"
    color: warmColors.checked ? "#30251c" : "#18232e"

    Behavior on color {
        ColorAnimation { duration: 180 }
    }
    // [binding]

    StandardMainActions { id: actions; enabled: screen.enabled }

    // [layout]
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, Math.min(400, screen.width - 32))
        spacing: 12

        Image {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(120, screen.height / 5)
            source: "images/notes.svg"
            sourceSize: Qt.size(320, 120)
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Choose what to open")
            color: screen.accent
            font.pixelSize: 26
            wrapMode: Text.Wrap
        }
        // [layout]

        // [use-component]
        MenuButton {
            Layout.fillWidth: true
            label: qsTr("Song select")
            focus: true
            onClicked: actions.openSelect()
        }
        MenuButton {
            Layout.fillWidth: true
            label: qsTr("Settings")
            onClicked: actions.openSettings()
        }
        MenuButton {
            Layout.fillWidth: true
            label: qsTr("Quit")
            onClicked: actions.quit()
        }
        // [use-component]

        // [change-state]
        CheckBox {
            id: warmColors
            text: qsTr("Use warm colors")
            palette.windowText: screen.accent
        }
        // [change-state]
    }
}
