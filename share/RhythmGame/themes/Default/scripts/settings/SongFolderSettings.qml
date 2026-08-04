import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

import "SettingsColors.js" as SettingsColors

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

    FileDialog {
        id: archiveDialog

        title: qsTr("Add song archive")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Song archives (*.zip *.7z *.rar *.tar *.tgz *.tbz *.tbz2 *.txz *.lha *.lzh *.cab *.xar *.cpio *.iso *.ar *.gz *.bz2 *.xz *.zst)")
        ]

        onAccepted: {
            Rg.rootSongFoldersConfig.folders.add(archiveDialog.selectedFile.toString());
        }
    }

    SettingsWorkspaceScaffold {
        id: pageScaffold

        anchors.fill: parent
        SettingsPageHeader {
            id: pageHeader

            title: qsTr("Song sources")
            subtitle: qsTr("Manage root song folders, archives, and background scanning.")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            WorkbenchPanel {
                id: songListFrame

                title: qsTr("Song sources")
                subtitle: qsTr("Folders and archives scanned for BMS charts.")
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
                        text: qsTr("Add archive")
                        tone: ActionButton.Primary
                        Layout.fillWidth: true

                        onClicked: {
                            archiveDialog.open();
                        }
                    }

                    ActionButton {
                        text: qsTr("Scan all")
                        tone: ActionButton.Secondary
                        Layout.fillWidth: true
                        enabled: folderList.count > 0

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
                        visible: folderList.count === 0
                        width: Math.min(360, parent.width - 32)
                        title: qsTr("No song sources")
                        subtitle: qsTr("Add a folder or archive that contains your BMS charts.")
                    }

                    ListView {
                        id: folderList

                        anchors.fill: parent
                        visible: count > 0
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
                subtitle: qsTr("Sources waiting for or currently undergoing scan.")
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Item {
                    id: scanActivityArea

                    readonly property bool hasCurrentScan: Rg.rootSongFoldersConfig.scanningQueue.currentScannedFolder.length > 0

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.minimumHeight: 220

                    EmptyState {
                        anchors.centerIn: parent
                        visible: scanQueueList.count === 0 && !scanActivityArea.hasCurrentScan
                        width: Math.min(360, parent.width - 32)
                        title: qsTr("Scanner idle")
                        subtitle: qsTr("Scan one source or all sources to see progress here.")
                    }

                    ListView {
                        id: scanQueueList

                        anchors.fill: parent
                        visible: count > 0
                        clip: true
                        model: Rg.rootSongFoldersConfig.scanningQueue
                        spacing: 5
                        ScrollBar.vertical: ScrollBar {}

                        delegate: WorkbenchListRow {
                            id: scanItemRow

                            property string name: songFolderSettings.folderLabel(model.display)

                            width: ListView.view ? ListView.view.width : 0
                            primaryText: scanItemRow.name

                            Rectangle {
                                color: SettingsColors.chipFill(palette)
                                radius: 6
                                border.width: 1
                                border.color: SettingsColors.alpha(SettingsColors.panelBorder(palette), 0.7)
                                Layout.preferredWidth: 112
                                Layout.maximumWidth: 112
                                Layout.preferredHeight: 28
                                Layout.alignment: Qt.AlignVCenter

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 6

                                    Item {
                                        Layout.preferredWidth: 16
                                        Layout.preferredHeight: 16

                                        BusyIndicator {
                                            anchors.fill: parent
                                            running: index === 0
                                            visible: running
                                        }
                                    }

                                    Label {
                                        text: index === 0 ? qsTr("Scanning") : qsTr("Queued")
                                        color: SettingsColors.alpha(palette.windowText, 0.82)
                                        elide: Text.ElideRight
                                        maximumLineCount: 1
                                        horizontalAlignment: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
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
