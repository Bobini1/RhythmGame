import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    id: screen

    color: "white"

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabView

            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height

            TabButton {
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
                text: qsTr("Global Settings")
            }
            TabButton {
                text: qsTr("Key config")
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
            GlobalSettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
            KeySettings {
                Layout.preferredHeight: parent.height - tabView.height
            }
        }
    }
    Shortcut {
        enabled: screen.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            if (sceneStack.depth === 1)
                sceneStack.replace(0, globalRoot.mainComponent);
        }
    }
}
