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
            FolderDialog {
                id: replayFolderDialog
                title: qsTr("Select beatoraja replay folder")

                onAccepted: {
                    replayImportSection.selectedFolder = selectedFolder
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

                ColumnLayout {
                    id: profileColumnLayout
                    anchors.fill: parent
                    spacing: 0

                    Label {
                        id: profileText
                        font.pixelSize: 20
                        text: qsTr("Edit Profile")
                        Layout.bottomMargin: 16
                    }
                    Frame {
                        id: avatarFrame
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: profileColumnLayout.width / 2
                        Layout.maximumWidth: profileColumnLayout.width / 2
                        Layout.preferredHeight: profileColumnLayout.width / 2
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
                        Layout.preferredWidth: profileColumnLayout.width / 2
                        Layout.maximumWidth: profileColumnLayout.width / 2
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        onTextChanged: {
                            Rg.profileList.mainProfile.vars.generalVars.name = text;
                        }
                    }
                    ColumnLayout {
                        id: loginSection
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: profileColumnLayout.width / 2
                        Layout.maximumWidth: profileColumnLayout.width / 2
                        Layout.topMargin: 16
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
                            op.finishedChanged.connect(function() {
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

                    Loader {
                        id: authLoader
                        sourceComponent: {
                            switch (loginSection.profile.loginState) {
                            case Profile.NotLoggedIn:
                            case Profile.LoginFailed:
                                return loggedOutComponent;
                            case Profile.LoggingIn:
                                return loggingInComponent;
                            case Profile.LoggedIn:
                                return loggedInComponent;
                            }
                        }
                        Layout.preferredHeight: authLoader.item ? authLoader.item.implicitHeight : 0
                        Layout.fillWidth: true
                    }

                    Component {
                        id: loggingInComponent
                        ColumnLayout {
                            spacing: 8
                            BusyIndicator {
                                running: true
                                visible: true
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                                width: 32
                                height: 32
                            }
                        }
                    }

                    Component {
                        id: loggedInComponent
                        ColumnLayout {
                            spacing: 8
                            Label {
                                text: qsTr("Logged in as %1").arg(loginSection.profile.onlineUserData.username)
                                font.pixelSize: 16
                                Layout.fillWidth: true
                            }
                            Button {
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
                                    visible: loginSection.syncing
                                    Layout.alignment: Qt.AlignVCenter
                                    width: 24
                                    height: 24
                                }
                            }
                        }
                    }

                    Component {
                        id: loggedOutComponent
                        ColumnLayout {
                            spacing: 8
                            TextField {
                                id: emailField
                                placeholderText: qsTr("Email")
                                Layout.fillWidth: true
                                height: 32
                                onAccepted: {
                                    passwordField.forceActiveFocus();
                                }
                            }
                            TextField {
                                id: passwordField
                                placeholderText: qsTr("Password")
                                echoMode: TextInput.Password
                                Layout.fillWidth: true
                                height: 32
                                onAccepted: {
                                    loginSection.profile.login(emailField.text, passwordField.text);
                                }
                            }
                            Button {
                                text: qsTr("Login")
                                Layout.fillWidth: true
                                palette.button: loginSection.profile.loginState === Profile.LoginFailed ? "DarkRed" : undefined
                                palette.buttonText: loginSection.profile.loginState === Profile.LoginFailed ? "white" : undefined
                                onClicked: loginSection.profile.login(emailField.text, passwordField.text)
                            }
                        }
                    }
                }
                    ColumnLayout {
                        id: replayImportSection
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: profileColumnLayout.width / 2
                        Layout.maximumWidth: profileColumnLayout.width / 2
                        Layout.topMargin: 24
                        Layout.fillHeight: true
                        Layout.bottomMargin: 16
                        spacing: 8

                    property url selectedFolder: ""
                    readonly property var op: loginSection.profile.replayImportOperation
                    readonly property bool importing: op !== null && !op.finished

                    ListModel {
                        id: errorsModel
                    }

                    // Clear errors whenever a fresh import operation is started.
                    Connections {
                        target: loginSection.profile
                        function onReplayImportOperationChanged() {
                            errorsModel.clear()
                        }
                    }

                    // Track per-score errors and the finished signal from the
                    // current operation. When op is null these connections are
                    // simply inactive.
                    Connections {
                        target: replayImportSection.op
                        function onError(message) {
                            errorsModel.append({ message: message })
                        }
                        function onFinishedChanged() {
                            rootScrollView.updateScoreCounts++
                        }
                    }

                    Label {
                        text: qsTr("Import beatoraja replays")
                        font.pixelSize: 18
                        Layout.fillWidth: true
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: replayImportSection.selectedFolder === ""
                            ? qsTr("No folder selected")
                            : replayImportSection.selectedFolder.toString()
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: qsTr("Choose folder")
                            Layout.fillWidth: true
                            enabled: !replayImportSection.importing

                            onClicked: {
                                replayFolderDialog.open();
                            }
                        }

                        Button {
                            text: qsTr("Import scores")
                            Layout.fillWidth: true
                            enabled: replayImportSection.selectedFolder !== "" &&
                                     !replayImportSection.importing

                            onClicked: {
                                loginSection.profile.importReplays(
                                    replayImportSection.selectedFolder.toString())
                            }
                        }

                        BusyIndicator {
                            running: replayImportSection.importing
                            visible: replayImportSection.importing
                            Layout.alignment: Qt.AlignVCenter
                            width: 24
                            height: 24
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        visible: replayImportSection.op !== null
                        value: replayImportSection.op !== null
                               ? replayImportSection.op.done / Math.max(replayImportSection.op.total, 1)
                               : 0
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        visible: replayImportSection.op !== null
                        text: qsTr("Imported: %1, errors: %2, skipped: %3, total: %4")
                            .arg(replayImportSection.op ? replayImportSection.op.imported : 0)
                            .arg(replayImportSection.op ? replayImportSection.op.errored : 0)
                            .arg(replayImportSection.op ? replayImportSection.op.skipped : 0)
                            .arg(replayImportSection.op ? replayImportSection.op.total : 0)
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 100
                        visible: errorsModel.count > 0
                        model: errorsModel
                        spacing: 2
                        clip: true
                        ScrollBar.vertical: ScrollBar {}

                        delegate: Label {
                            required property string message
                            width: ListView.view.width
                            wrapMode: Text.Wrap
                            color: "tomato"
                            text: message
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                        visible: errorsModel.count === 0
                    }
                }
                }
            }
        }
    }
}
