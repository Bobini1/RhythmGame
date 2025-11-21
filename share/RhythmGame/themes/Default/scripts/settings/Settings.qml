import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationItem {
    id: settings

    footer: Kirigami.NavigationTabBar {
        actions: [
            Kirigami.Action {
                icon.name: "arrow-left"
                text: qsTr("Back")
                onTriggered: {
                    globalRoot.pageStack.pop();
                }
            },
            Kirigami.Action {
                icon.name: "account-player"
                text: qsTr("Player settings")
                onTriggered:  settings.pageStack.push(Qt.resolvedUrl("PlayerSettings.qml"))
                checked: true
            },
            Kirigami.Action {
                icon.name: "folder-music"
                text: qsTr("Song directories")
                onTriggered: settings.pageStack.push(Qt.resolvedUrl("SongFolderSettings.qml"))
            },
            Kirigami.Action {
                icon.name: "table"
                text: qsTr("Tables")
                onTriggered: settings.pageStack.push(Qt.resolvedUrl("TableSettings.qml"))
            },
            Kirigami.Action {
                icon.name: "palette"
                text: qsTr("Themes")
                onTriggered: settings.pageStack.push(Qt.resolvedUrl("ThemeSettings.qml"))
            },
            Kirigami.Action {
                icon.name: "settings"
                text: qsTr("General Settings")
                onTriggered: settings.pageStack.push(Qt.resolvedUrl("GeneralSettings.qml"))
            },
            Kirigami.Action {
                icon.name: "keyboard"
                text: qsTr("Key config")
                onTriggered: settings.pageStack.push(Qt.resolvedUrl("KeySettings.qml"))
            }
        ]
    }
    
    Shortcut {
        enabled: settings.enabled
        sequence: "Esc"

        onActivated: {
            globalRoot.pageStack.pop();
        }
    }
}
