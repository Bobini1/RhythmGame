import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: songFolderSettings

    function folderLabel(folder) {
        if (!folder) {
            return "";
        }
        if (typeof folder === "string") {
            return folder;
        }
        if (typeof folder !== "object") {
            return "";
        }

        const value = folder.name || folder.folder || folder.path;
        return value ? String(value) : "";
    }

    FolderDialog {
        id: folderDialog

        title: qsTr("Add song folder")

        onAccepted: {
            Rg.rootSongFoldersConfig.folders.add(folderDialog.selectedFolder.toString());
        }
    }

    SettingsWorkspaceScaffold {
        id: pageScaffold

        anchors.fill: parent
        SettingsPageHeader {
            id: pageHeader

            title: qsTr("Song directories")
            subtitle: qsTr("Manage root song folders and background scanning.")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            WorkbenchPanel {
                id: songListFrame

                title: qsTr("Song folders")
                subtitle: qsTr("Folders scanned for BMS charts.")
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ActionButton {
                        text: qsTr("Add folder")
                        tone: ActionButton.Primary
                        Layout.fillWidth: true

                        onClicked: {
                            folderDialog.open();
                        }
                    }

                    ActionButton {
                        text: qsTr("Scan all")
                        tone: ActionButton.Secondary
                        Layout.fillWidth: true
                        enabled: Rg.rootSongFoldersConfig.folders.rowCount() > 0

                        onClicked: {
                            for (let i = 0; i < Rg.rootSongFoldersConfig.folders.rowCount(); i++) {
                                Rg.rootSongFoldersConfig.scanningQueue.scan(Rg.rootSongFoldersConfig.folders.at(i));
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.minimumHeight: 220

                    EmptyState {
                        anchors.centerIn: parent
                        visible: Rg.rootSongFoldersConfig.folders.rowCount() === 0
                        width: Math.min(360, parent.width - 32)
                        title: qsTr("No song folders")
                        subtitle: qsTr("Add a folder that contains your BMS charts.")
                    }

                    ListView {
                        anchors.fill: parent
                        visible: Rg.rootSongFoldersConfig.folders.rowCount() > 0
                        clip: true
                        model: Rg.rootSongFoldersConfig.folders
                        spacing: 5
                        ScrollBar.vertical: ScrollBar {}

                        delegate: WorkbenchListRow {
                            id: folderRow

                            property var rootFolder: model.display

                            width: ListView.view ? ListView.view.width : 0
                            primaryText: songFolderSettings.folderLabel(rootFolder)
                            secondaryText: qsTr("Root song source")

                            ActionButton {
                                text: qsTr("Scan")
                                tone: ActionButton.Secondary

                                onClicked: {
                                    Rg.rootSongFoldersConfig.scanningQueue.scan(folderRow.rootFolder);
                                }
                            }

                            ActionButton {
                                text: qsTr("Remove")
                                tone: ActionButton.Danger

                                onClicked: {
                                    Rg.rootSongFoldersConfig.folders.remove(index);
                                }
                            }
                        }
                    }
                }
            }

            WorkbenchPanel {
                id: scanningQueueFrame

                title: qsTr("Scan activity")
                subtitle: qsTr("Folders waiting for or currently undergoing scan.")
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.minimumHeight: 220

                    EmptyState {
                        anchors.centerIn: parent
                        visible: Rg.rootSongFoldersConfig.scanningQueue.rowCount() === 0
                        width: Math.min(360, parent.width - 32)
                        title: qsTr("Scanner idle")
                        subtitle: qsTr("Scan one folder or all folders to see progress here.")
                    }

                    ListView {
                        anchors.fill: parent
                        visible: Rg.rootSongFoldersConfig.scanningQueue.rowCount() > 0
                        clip: true
                        model: Rg.rootSongFoldersConfig.scanningQueue
                        spacing: 5
                        ScrollBar.vertical: ScrollBar {}

                        delegate: WorkbenchListRow {
                            id: scanItemRow

                            property string name: songFolderSettings.folderLabel(model.display)

                            width: ListView.view ? ListView.view.width : 0
                            primaryText: scanItemRow.name
                            metaText: index === 0 ? qsTr("Scanning") : qsTr("Queued")

                            BusyIndicator {
                                running: index === 0
                                visible: index === 0
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }

                            ActionButton {
                                text: qsTr("Cancel")
                                tone: ActionButton.Tertiary

                                onClicked: {
                                    Rg.rootSongFoldersConfig.scanningQueue.remove(index);
                                }
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: Rg.rootSongFoldersConfig.scanningQueue.currentScannedFolder.length > 0
                    elide: Text.ElideMiddle
                    text: Rg.rootSongFoldersConfig.scanningQueue.currentScannedFolder
                }
            }
        }
    }
}
