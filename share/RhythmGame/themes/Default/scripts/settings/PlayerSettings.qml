import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami


Kirigami.ScrollablePage {
    id: profileListFrame

    Kirigami.Dialog {
        id: confirmDeletion
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

    Layout.fillWidth: true

    title: qsTr("Select Profile")
    footer: Button {
        text: qsTr("Add profile")

        onClicked: {
            Rg.profileList.createProfile();
        }
    }
    ListView {
        id: profileList

        model: Rg.profileList.profiles.slice().sort((a, b) => {
            return a.vars.generalVars.name.localeCompare(b.vars.generalVars.name);
        });
        spacing: 5

        delegate: ItemDelegate {
            id: userRow

            property var profile: modelData
            
            highlighted: Rg.profileList.mainProfile === profile
            height: removeButton.height + 10

            onClicked: {
                Rg.profileList.mainProfile = userRow.profile;  // Select profile
            }

            anchors.left: parent.left
            anchors.right: parent.right

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                height: childrenRect.height
                anchors.verticalCenter: parent.verticalCenter

                anchors.margins: 30

                Label {
                    elide: Text.ElideRight
                    text: userRow.profile.vars.generalVars.name
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                }

                Label {
                    id: scoreText
                    text: qsTr("Scores: %1").arg(userRow.profile.scoreDb.getTotalScoreCount())
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                }

                Button {
                    id: removeButton
                    text: qsTr("Remove")

                    onClicked: {
                        confirmDeletion.profile = userRow.profile;
                        confirmDeletion.open();
                    }
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                }
            }

        }
    }
    Component.onCompleted: {
        applicationWindow().pageStack.push(profileEdit);
    }
    Component {
        id: profileEdit
        Kirigami.Page {
            id: profileFrame

            Layout.fillWidth: true

            title: qsTr("Edit Profile")
            Frame {
                id: avatarFrame
                anchors.horizontalCenter: parent.horizontalCenter
                width: 200
                height: 200
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
                color: palette.text
                width: avatarFrame.width
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.top: avatarFrame.bottom
                anchors.topMargin: 16
                anchors.horizontalCenter: avatarFrame.horizontalCenter

                onTextChanged: {
                    Rg.profileList.mainProfile.vars.generalVars.name = text;
                }
            }
        }
    }
}