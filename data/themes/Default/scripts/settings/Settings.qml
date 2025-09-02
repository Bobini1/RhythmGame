import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    id: settings

    color: "white"

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: backButton.height
            spacing: 1

            Button {
                id: backButton
                text: "⏎"
                font.bold: true
                background: Rectangle {
                    color: "FireBrick"
                }
                palette.buttonText: "white"
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
            Layout.fillWidth: true
            Layout.fillHeight: true

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
    }
    Shortcut {
        enabled: settings.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
}
