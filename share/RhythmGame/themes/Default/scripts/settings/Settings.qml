import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    id: settings
    
    header: RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: backButton.height
        spacing: 1

        Button {
            id: backButton
            text: "⏎"
            font.bold: true
            palette {
                button: settings.palette.accent
                buttonText: settings.palette.brightText
            }
            font.pixelSize: 20
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.preferredWidth: tabButton.height
            Layout.preferredHeight: tabButton.height
            onClicked: {
                sceneStack.pop();
            }
        }
        TabBar {
            id: tabView
            Layout.fillWidth: true

            TabButton {
                id: tabButton
                text: qsTr("Player settings")
            }
            TabButton {
                text: qsTr("Song directories")
            }
            TabButton {
                text: qsTr("Tables")
            }
            TabButton {
                text: qsTr("Themes")
            }
            TabButton {
                text: qsTr("General Settings")
            }
            TabButton {
                text: qsTr("Key config")
            }
        }
    }
    StackLayout {
        id: stackView
        anchors.fill: parent

        currentIndex: tabView.currentIndex

        PlayerSettings {
        }
        SongFolderSettings {
        }
        TableSettings {
        }
        ThemeSettings {
        }
        GeneralSettings {
        }
        KeySettings {
        }
    }
    Shortcut {
        enabled: settings.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
}
