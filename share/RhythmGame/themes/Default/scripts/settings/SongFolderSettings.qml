import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    ScrollView {
        id: rootScrollView
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: Math.min(1200, parent.width)
        contentWidth: Math.max(800, width)
        contentHeight: Math.max(rootRowLayout.implicitHeight, parent.height)

        RowLayout {
            id: rootRowLayout
            anchors.fill: parent
            anchors.margins: 5

            FolderDialog {
                id: folderDialog

                title: qsTr("Add song folder")

                onAccepted: {
                    Rg.rootSongFoldersConfig.folders.add(folderDialog.selectedFolder.toString());
                }
            }
            Frame {
                id: songListFrame

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                ColumnLayout {
                    anchors.fill: parent

                    Label {
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
                            model: Rg.rootSongFoldersConfig.folders

                            delegate: RowLayout {
                                id: folderRow

                                property var rootFolder: display

                                width: parent ? parent.width : 0

                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: TextEdit.Wrap
                                    text: folderRow.rootFolder ? folderRow.rootFolder.name : ""
                                }
                                Button {
                                    text: qsTr("Remove")

                                    onClicked: {
                                        Rg.rootSongFoldersConfig.folders.remove(index);
                                    }
                                }
                                Button {
                                    text: qsTr("Scan")

                                    onClicked: {
                                        Rg.rootSongFoldersConfig.scanningQueue.scan(folderRow.rootFolder);
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
                                for (let i = 0; i < Rg.rootSongFoldersConfig.folders.rowCount(); i++) {
                                    Rg.rootSongFoldersConfig.scanningQueue.scan(Rg.rootSongFoldersConfig.folders.at(i));
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

                    Label {
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
                            model: Rg.rootSongFoldersConfig.scanningQueue

                            delegate: RowLayout {
                                id: scanItemRow

                                property string name: display.name

                                Layout.fillWidth: true
                                width: parent ? parent.width : 0

                                Label {
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
                                        Rg.rootSongFoldersConfig.scanningQueue.remove(index);
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
                        text: Rg.rootSongFoldersConfig.scanningQueue.currentScannedFolder
                    }
                }
            }
        }
    }
}