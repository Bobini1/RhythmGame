import QtQuick
import QtQuick.Controls.Basic
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
        contentWidth: Math.max(600, width)
        contentHeight: Math.max(rootRowLayout.implicitHeight, parent.height)

        RowLayout {
            id: rootRowLayout
            anchors.fill: parent
            anchors.margins: 5

            Dialog {
                id: confirmDeletion
                anchors.centerIn: parent
                property Profile profile: null
                title: qsTr("Delete %1?").arg(profile ? profile.vars.generalVars.name : "")
                standardButtons: Dialog.Ok | Dialog.Cancel
                modal: true

                onAccepted: Rg.profileList.removeProfile(profile)
            }
            FileDialog {
                id: fileDialog
                currentFolder: Rg.programSettings.avatarFolder
                onAccepted: {
                    Rg.profileList.mainProfile.vars.generalVars.avatar = selectedFile;
                }
            }
            Frame {
                id: profileListFrame

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                ColumnLayout {
                    anchors.fill: parent

                    Text {
                        font.pixelSize: 20
                        text: qsTr("Select Profile")
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        ListView {
                            id: profileList

                            clip: true
                            model: Rg.profileList.profiles.slice().sort((a, b) => {
                                return a.vars.generalVars.name.localeCompare(b.vars.generalVars.name);
                            });
                            spacing: 5

                            delegate: Rectangle {
                                id: folderRow

                                property var profile: modelData

                                color: isSelected ? palette.highlight : "transparent"
                                readonly property bool isSelected:
                                    Rg.profileList.mainProfile === profile
                                height: removeButton.height + 10

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        Rg.profileList.mainProfile = folderRow.profile;  // Select profile
                                    }
                                }

                                width: parent ? parent.width : 0

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 16
                                    anchors.right: scoreText.left
                                    anchors.rightMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                    color: folderRow.isSelected ? palette.highlightedText : "black"
                                    text: folderRow.profile.vars.generalVars.name
                                }

                                TextEdit {
                                    id: scoreText
                                    anchors.right: removeButton.left
                                    anchors.rightMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    readOnly: true
                                    text: qsTr("Scores: %1").arg(folderRow.profile.scoreDb.getTotalScoreCount())
                                    color: palette.brightText
                                }

                                Button {
                                    id: removeButton
                                    text: qsTr("Remove")

                                    anchors.right: parent.right
                                    anchors.rightMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter

                                    onClicked: {
                                        confirmDeletion.profile = folderRow.profile;
                                        confirmDeletion.open();
                                    }
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true

                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Add profile")

                            onClicked: {
                                Rg.profileList.createProfile();
                            }
                        }
                    }
                }
            }
            Frame {
                id: profileFrame

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                Text {
                    id: profileText
                    font.pixelSize: 20
                    text: qsTr("Edit Profile")
                }
                Frame {
                    id: avatarFrame
                    anchors.top: profileText.bottom
                    anchors.topMargin: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width / 2
                    height: parent.width / 2
                    Image {
                        anchors.fill: parent
                        source: Rg.profileList.mainProfile.vars.generalVars.avatar
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit

                        TapHandler {
                            onTapped: {
                                fileDialog.open();
                            }
                        }
                    }
                }
                TextField {
                    text: Rg.profileList.mainProfile.vars.generalVars.name
                    font.pixelSize: 24
                    color: "black"
                    width: avatarFrame.width
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.top: avatarFrame.bottom
                    anchors.topMargin: 16
                    anchors.horizontalCenter: parent.horizontalCenter

                    onTextChanged: {
                        Rg.profileList.mainProfile.vars.generalVars.name = text;
                    }
                }
            }
        }
    }
}