import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls.Basic 2.12

Rectangle {
    id: screen

    readonly property bool active: StackView.status === StackView.Active

    color: "white"

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabView

            Layout.fillWidth: true
            height: childrenRect.height

            TabButton {
                text: qsTr("Song directories")
            }
            TabButton {
                text: qsTr("Themes")
            }
            TabButton {
                text: qsTr("Key config")
            }
        }
        StackLayout {
            id: stackView

            currentIndex: tabView.currentIndex

            SongFolderSettings {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height - tabView.height
            }
            ThemeSettings {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height - tabView.height
            }
            KeySettings {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height - tabView.height
            }
        }
    }
    Shortcut {
        enabled: active
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            if (sceneStack.depth === 1)
                sceneStack.replace(0, globalRoot.mainComponent);
        }
    }
}
