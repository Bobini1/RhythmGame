import QtQuick 2.15
import RhythmGameQml
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls.Basic 2.12

Pane {
    id: screen

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
        }
        StackLayout {
            id: stackView

            currentIndex: tabView.currentIndex

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height - tabView.height

                Frame {
                    id: songListFrame

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent

                        ScrollView {
                            Layout.fillHeight: true
                            Layout.fillWidth: true

                            ListView {
                                id: songList

                                clip: true
                                model: songFolders

                                delegate: RowLayout {
                                    Layout.fillWidth: true

                                    Text {
                                        Layout.fillWidth: true
                                        color: "white"
                                        text: modelData
                                    }
                                    Button {
                                        id: selectFoldersButton

                                        text: qsTr("Remove")

                                        onClicked: {
                                            songFolders.remove(index);
                                            RootSongFoldersConfig.folders = folderDialog.getSelectedFolders();
                                        }
                                    }
                                }
                            }
                        }
                        RowLayout {
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Add song folder")

                                onClicked: {
                                    folderDialog.open();
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Scan new")

                                onClicked: {
                                    RootSongFoldersConfig.scanNew();
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Scan all")

                                onClicked: {
                                    RootSongFoldersConfig.scanAll();
                                }
                            }
                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Clear database")

                                onClicked: {
                                    RootSongFoldersConfig.clear();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Shortcut {
        enabled: true
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            sceneStack.pop();
        }
    }
}
