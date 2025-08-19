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
                    color: "red"
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
                Layout.preferredHeight: parent.height - tabView.height
            }
            SongFolderSettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
            TableSettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
            ThemeSettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
            GeneralSettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
            KeySettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
        }
    }
    Shortcut {
        enabled: settings.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            if (sceneStack.depth === 1)
                sceneStack.replace(0, globalRoot.mainComponent);
        }
    }
}
