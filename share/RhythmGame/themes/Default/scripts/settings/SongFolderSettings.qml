import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    ScrollView {
        id: rootScrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            id: rootColumnLayout

            x: Math.max(0, (rootScrollView.availableWidth - width) / 2)
            width: Math.min(1220, rootScrollView.availableWidth)
            spacing: 14

            FolderDialog {
                id: folderDialog

                title: qsTr("Add song folder")

                onAccepted: {
                    Rg.rootSongFoldersConfig.folders.add(folderDialog.selectedFolder.toString());
                }
            }

            SettingsPageHeader {
                id: pageHeader
                title: qsTr("Song directories")
                subtitle: qsTr("Manage root song folders and background scanning.")
            }

            RowLayout {
                id: rootRowLayout

                Layout.fillWidth: true
                Layout.minimumHeight: Math.max(0, rootScrollView.availableHeight - pageHeader.implicitHeight - rootColumnLayout.spacing)
                spacing: 14

                WorkbenchPanel {
                    id: songListFrame

                    title: qsTr("Song directories")
                    subtitle: qsTr("Folders scanned for BMS charts.")
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1

                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        ListView {
                            id: songList

                            clip: true
                            model: Rg.rootSongFoldersConfig.folders
                            spacing: 5

                            delegate: WorkbenchListRow {
                                id: folderRow

                                property var rootFolder: display

                                width: ListView.view ? ListView.view.width : 0
                                primaryText: folderRow.rootFolder ? folderRow.rootFolder.name : ""

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

                    RowLayout {
                        Layout.fillWidth: true

                        ActionButton {
                            Layout.fillWidth: true
                            text: qsTr("Add song folder")
                            tone: ActionButton.Primary

                            onClicked: {
                                folderDialog.open();
                            }
                        }

                        ActionButton {
                            Layout.fillWidth: true
                            text: qsTr("Scan all")
                            tone: ActionButton.Secondary

                            onClicked: {
                                for (let i = 0; i < Rg.rootSongFoldersConfig.folders.rowCount(); i++) {
                                    Rg.rootSongFoldersConfig.scanningQueue.scan(Rg.rootSongFoldersConfig.folders.at(i));
                                }
                            }
                        }
                    }
                }

                WorkbenchPanel {
                    id: scanningQueueFrame

                    title: qsTr("Scanning queue")
                    subtitle: qsTr("Folders waiting for or currently undergoing scan.")
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1

                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        ListView {
                            id: scanningQueueList

                            clip: true
                            model: Rg.rootSongFoldersConfig.scanningQueue
                            spacing: 5

                            delegate: WorkbenchListRow {
                                id: scanItemRow

                                property string name: display.name

                                width: ListView.view ? ListView.view.width : 0
                                primaryText: scanItemRow.name
                                metaText: index === 0 ? qsTr("Scanning") : qsTr("Queued")

                                BusyIndicator {
                                    running: index === 0
                                    visible: index === 0
                                    Layout.alignment: Qt.AlignVCenter
                                    width: 24
                                    height: 24
                                }

                                ActionButton {
                                    id: removeScanItemButton

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
                        id: logText

                        Layout.alignment: Qt.AlignBottom
                        Layout.fillWidth: true
                        text: Rg.rootSongFoldersConfig.scanningQueue.currentScannedFolder
                    }
                }
            }
        }
    }
}
