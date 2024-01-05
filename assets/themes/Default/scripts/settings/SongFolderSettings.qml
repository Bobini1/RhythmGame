import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts

ColumnLayout {
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
