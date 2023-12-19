import QtQuick 2.15
import RhythmGameQml
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls.Basic 2.12

Rectangle {
    id: screen

    readonly property bool active: StackView.status === StackView.Active

    color: "darkslategray"

    ListModel {
        id: songFolders

        Component.onCompleted: {
            for (let folder of RootSongFoldersConfig.folders) {
                songFolders.append({
                    "text": folder
                });
            }
        }
    }

    FolderDialog {
        id: folderDialog

        function getSelectedFolders() {
            let folders = [];
            for (let item = 0; item < songFolders.count; item++) {
                folders.push(songFolders.get(item).text);
            }
            return folders;
        }

        title: qsTr("Add song folder")
        onAccepted: {
            songFolders.append({
                "text": globalRoot.urlToPath(folderDialog.selectedFolder.toString())
            });
            RootSongFoldersConfig.folders = getSelectedFolders();
        }
    }

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
