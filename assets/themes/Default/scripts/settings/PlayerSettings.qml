import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs

RowLayout {
    Dialog {
        id: confirmDeletion
        anchors.centerIn: parent
        property Profile profile: null
        title: "Delete " + (profile ? profile.vars.globalVars.name : "") + "?"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true

        onAccepted: ProfileList.removeProfile(profile)
    }
    FileDialog {
        id: fileDialog
        currentFolder: ProgramSettings.avatarFolder
        onAccepted: {
            ProfileList.mainProfile.vars.globalVars.avatar = selectedFile;
        }
    }
    Frame {
        id: profileListFrame

        Layout.fillHeight: true
        Layout.preferredWidth: parent.width / 2

        ColumnLayout {
            anchors.fill: parent

            Text {
                font.pixelSize: 20
                text: qsTr("Profiles")
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            ScrollView {
                Layout.fillHeight: true
                Layout.fillWidth: true

                ListView {
                    id: profileList

                    clip: true
                    model: ProfileList.profiles

                    delegate: RowLayout {
                        id: folderRow

                        property var profile: modelData

                        width: parent ? parent.width : 0

                        TextEdit {
                            readOnly: true
                            Layout.fillWidth: true
                            wrapMode: TextEdit.Wrap
                            text: folderRow.profile.vars.globalVars.name
                        }
                        Button {
                            text: qsTr("Remove")

                            onClicked: {
                                confirmDeletion.profile = folderRow.profile;
                                confirmDeletion.open();
                            }
                        }
                        Button {
                            text: qsTr("Set as main")
                            enabled: ProfileList.mainProfile !== folderRow.profile

                            onClicked: {
                                ProfileList.mainProfile = folderRow.profile;
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
                        ProfileList.createProfile();
                    }
                }
            }
        }
    }
    Frame {
        id: profileFrame

        Layout.fillHeight: true
        Layout.fillWidth: true

        TextEdit {
            id: profileText
            readOnly: true
            text: "Selected Profile"
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter
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
                source: ProgramSettings.avatarFolder + ProfileList.mainProfile.vars.globalVars.avatar
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
            text: ProfileList.mainProfile.vars.globalVars.name
            font.pixelSize: 24
            color: "black"
            width: avatarFrame.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.top: avatarFrame.bottom
            anchors.topMargin: 16
            anchors.horizontalCenter: parent.horizontalCenter

            onTextChanged: {
                ProfileList.mainProfile.vars.globalVars.name = text;
            }
        }
    }
}