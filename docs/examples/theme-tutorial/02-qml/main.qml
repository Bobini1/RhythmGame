import QtQuick
import QtQuick.Controls.FluentWinUI3
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

    StandardMainController { id: actions; enabled: screen.enabled }

    // [layout]
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0
        // [layout]

        // [top-space]
        Item {
            Layout.fillHeight: true
            Layout.preferredHeight: 1
        }
        // [top-space]

        // [menu-layout]
        ColumnLayout {
            id: menu
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: 400
            Layout.maximumHeight: menu.implicitHeight
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            // [menu-layout]

            // [image-layout]
            Image {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 120
                Layout.maximumHeight: 120
                source: "images/notes.svg"
                sourceSize: Qt.size(320, 120)
                fillMode: Image.PreserveAspectFit
                Accessible.ignored: true
            }
            // [image-layout]

            Label {
                Layout.fillWidth: true
                text: "Choose what to open"
                color: screen.accent
                font.pixelSize: 26
                wrapMode: Text.Wrap
            }

            // [use-component]
            MenuButton {
                Layout.fillWidth: true
                label: "Song select"
                focus: true
                onClicked: actions.openSelect()
            }
            MenuButton {
                Layout.fillWidth: true
                label: "Settings"
                onClicked: actions.openSettings()
            }
            MenuButton {
                Layout.fillWidth: true
                label: "Quit"
                onClicked: actions.quit()
            }
            // [use-component]

            // [change-state]
            CheckBox {
                id: warmColors
                text: "Use warm colors"
                palette.windowText: screen.accent
            }
            // [change-state]
        }

        // [bottom-space]
        Item {
            Layout.fillHeight: true
            Layout.preferredHeight: 1
        }
        // [bottom-space]
    }
}
