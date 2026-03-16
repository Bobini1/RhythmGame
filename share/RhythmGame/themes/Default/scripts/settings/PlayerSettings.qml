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
        contentWidth: Math.max(600, width)
        contentHeight: Math.max(rootRowLayout.implicitHeight, parent.height)

        property int updateScoreCounts: 0

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

                    Label {
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

                            delegate: ItemDelegate {
                                id: folderRow

                                property var profile: modelData
                                
                                highlighted: Rg.profileList.mainProfile === profile
                                height: removeButton.height + 10

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        Rg.profileList.mainProfile = folderRow.profile;  // Select profile
                                    }
                                }

                                width: parent ? parent.width : 0

                                Label {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 16
                                    anchors.right: scoreText.left
                                    anchors.rightMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                    text: folderRow.profile.vars.generalVars.name
                                }

                                Label {
                                    id: scoreText
                                    anchors.right: removeButton.left
                                    anchors.rightMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    property var scoreCount: (rootScrollView.updateScoreCounts, folderRow.profile.scoreDb.getTotalScoreCount())
                                    text: qsTr("Scores: %1").arg(scoreCount)
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
                                Rg.profileList.mainProfile = Rg.profileList.createProfile();
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

                Label {
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
                    id: nameField
                    text: Rg.profileList.mainProfile.vars.generalVars.name
                    font.pixelSize: 24
                    color: palette.text
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
                ColumnLayout {
                    id: loginSection
                    anchors.top: nameField.bottom
                    anchors.topMargin: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: avatarFrame.width
                    spacing: 8

                    property var profile: Rg.profileList.mainProfile
                    property bool syncing: false
                    property int pendingOps: 0
                    property bool syncError: false
                    property bool loginError: false

                    function runSync() {
                        loginSection.syncing = true;
                        loginSection.syncError = false;
                        loginSection.pendingOps = 2;

                        function attachOp(op) {
                            op.error.connect(function(msg) {
                                console.warn("Sync error:", msg);
                                loginSection.syncError = true;
                            });
                            op.finished.connect(function() {
                                loginSection.pendingOps = Math.max(0, loginSection.pendingOps - 1);
                                if (loginSection.pendingOps === 0) {
                                    loginSection.syncing = false;
                                    if (!loginSection.syncError)
                                        rootScrollView.updateScoreCounts++;
                                }
                            });
                        }

                        attachOp(loginSection.profile.downloadScores());
                        attachOp(loginSection.profile.uploadScores());
                    }

                    Label {
                        text: qsTr("Online Login")
                        font.pixelSize: 18
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: loginSection.profile.loginState === Profile.LoggedIn
                        text: qsTr("Logged in as %1").arg(loginSection.profile.onlineUsername)
                        font.pixelSize: 16
                        Layout.fillWidth: true
                    }

                    Button {
                        visible: loginSection.profile.loginState === Profile.LoggedIn
                        text: qsTr("Logout")
                        Layout.fillWidth: true
                        onClicked: {
                            loginSection.profile.logout();
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            id: syncButton
                            visible: loginSection.profile.loginState === Profile.LoggedIn
                            enabled: !loginSection.syncing
                            text: qsTr("Sync scores")
                            Layout.fillWidth: true
                            palette.button: loginSection.syncError ? "red" : undefined
                            palette.buttonText: loginSection.syncError ? "white" : undefined
                            onClicked: {
                                loginSection.runSync();
                            }
                        }
                        BusyIndicator {
                            running: loginSection.syncing
                            visible: running
                            Layout.alignment: Qt.AlignVCenter
                            width: 24
                            height: 24
                        }
                    }

                    TextField {
                        id: emailField
                        visible: loginSection.profile.loginState !== Profile.LoggedIn
                        placeholderText: qsTr("Email")
                        Layout.fillWidth: true
                    }

                    TextField {
                        id: passwordField
                        visible: loginSection.profile.loginState !== Profile.LoggedIn
                        placeholderText: qsTr("Password")
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                    }

                    Button {
                        visible: loginSection.profile.loginState !== Profile.LoggedIn
                        text: qsTr("Login")
                        Layout.fillWidth: true
                        palette.button: loginSection.profile.loginState === Profile.LoginFailed ? "DarkRed" : undefined
                        palette.buttonText: loginSection.profile.loginState === Profile.LoginFailed ? "white" : undefined
                        onClicked: loginSection.profile.login(emailField.text, passwordField.text)
                    }
                }
            }
        }
    }
}