import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    RowLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width / 2

        FolderDialog {
            id: folderDialog

            title: qsTr("Add song folder")

            onAccepted: {
                RootSongFoldersConfig.folders.add(folderDialog.selectedFolder.toString());
            }
        }
        Frame {
            id: songListFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            ColumnLayout {
                anchors.fill: parent

                Text {
                    font.pixelSize: 20
                    text: qsTr("Song directories")
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                ScrollView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ListView {
                        id: songList

                        clip: true
                        model: RootSongFoldersConfig.folders

                        delegate: RowLayout {
                            id: folderRow

                            property var rootFolder: display

                            width: parent ? parent.width : 0

                            TextEdit {
                                readOnly: true
                                Layout.fillWidth: true
                                wrapMode: TextEdit.Wrap
                                text: folderRow.rootFolder ? folderRow.rootFolder.name : ""
                            }
                            Button {
                                text: qsTr("Remove")

                                onClicked: {
                                    RootSongFoldersConfig.folders.remove(index);
                                }
                            }
                            Button {
                                text: qsTr("Scan")

                                onClicked: {
                                    RootSongFoldersConfig.scanningQueue.scan(folderRow.rootFolder);
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Add song folder")

                        onClicked: {
                            folderDialog.open();
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Scan all")

                        onClicked: {
                            for (let i = 0; i < RootSongFoldersConfig.folders.rowCount(); i++) {
                                RootSongFoldersConfig.scanningQueue.scan(RootSongFoldersConfig.folders.at(i));
                            }
                        }
                    }
                }
            }
        }
        Frame {
            id: scanningQueueFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            ColumnLayout {
                anchors.fill: parent

                Text {
                    font.pixelSize: 20
                    text: qsTr("Scanning queue")
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                ScrollView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ListView {
                        id: scanningQueueList

                        clip: true
                        model: RootSongFoldersConfig.scanningQueue

                        delegate: RowLayout {
                            id: scanItemRow

                            property string name: display.name

                            Layout.fillWidth: true
                            width: parent ? parent.width : 0

                            Text {
                                Layout.fillWidth: true
                                text: scanItemRow.name
                            }
                            BusyIndicator {
                                Layout.alignment: Qt.AlignRight
                                running: index === 0
                            }
                            Button {
                                id: removeScanItemButton

                                Layout.alignment: Qt.AlignRight
                                text: qsTr("Cancel")

                                onClicked: {
                                    RootSongFoldersConfig.scanningQueue.remove(index);
                                }
                            }
                        }
                    }
                }
                TextEdit {
                    id: logText

                    Layout.alignment: Qt.AlignBottom
                    Layout.fillWidth: true
                    readOnly: true
                    text: RootSongFoldersConfig.scanningQueue.currentScannedFolder
                }
            }
        }
    }
}