import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

import "SettingsColors.js" as SettingsColors

Item {
    id: playerSettings

    property int updateScoreCounts: 0

    function localFileUrl(path) {
        let value = String(path || "").trim();
        if (value.length === 0) {
            return "";
        }
        if (/^file:\/\//i.test(value) || /^[A-Za-z][A-Za-z0-9+.-]*:\/\//.test(value)) {
            return value;
        }
        value = value.replace(/\\/g, "/");
        return value[0] === "/" ? "file://" + encodeURI(value) : "file:///" + encodeURI(value);
    }

    function parentFolder(path) {
        let value = String(path || "").trim().replace(/\\/g, "/");
        while (value.length > 3 && value.endsWith("/")) {
            value = value.slice(0, -1);
        }
        const separator = value.lastIndexOf("/");
        return separator > 0 ? value.slice(0, separator) : value;
    }

    function openProfileFolder(profile) {
        const url = localFileUrl(parentFolder(profile.path));
        return url.length > 0 && Qt.openUrlExternally(url);
    }

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

        title: qsTr("Select replay folder")
        currentFolder: replayFolderSettings.folder

        onAccepted: {
            replayFolderSettings.folder = selectedFolder.toString()
        }
    }

    FileDialog {
        id: scoreDatabaseDialog

        title: qsTr("Select LR2 or beatoraja score database")
        nameFilters: [qsTr("SQLite databases (*.db)"), qsTr("All files (*)")]

        onAccepted: Rg.profileList.mainProfile.importScores(selectedFile.toString())
    }

    SettingsWorkspaceScaffold {
        id: pageScaffold

        anchors.fill: parent
        SettingsPageHeader {
            id: pageHeader

            title: qsTr("Player settings")
            subtitle: qsTr("Choose a profile, manage online login, sync scores, and import data.")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            WorkbenchPanel {
                id: profileListFrame

                title: qsTr("Profiles")
                subtitle: qsTr("%1 profiles").arg(Rg.profileList.profiles.length)
                Layout.alignment: Qt.AlignTop
                Layout.minimumWidth: 320
                Layout.preferredWidth: 360
                Layout.maximumWidth: 420

                ActionButton {
                    Layout.fillWidth: true
                    text: qsTr("Add profile")
                    tone: ActionButton.Primary

                    onClicked: {
                        Rg.profileList.mainProfile = Rg.profileList.createProfile();
                    }
                }

                ListView {
                    id: profileList

                    Layout.fillWidth: true
                    Layout.minimumHeight: 220
                    Layout.preferredHeight: Math.min(
                        Math.max(220, contentHeight),
                        Math.max(220, pageScaffold.availableHeight - pageHeader.implicitHeight - 160))
                    clip: true
                    model: Rg.profileList.profiles.slice().sort((a, b) => {
                        return a.vars.generalVars.name.localeCompare(b.vars.generalVars.name);
                    })
                    spacing: 5
                    ScrollBar.vertical: ScrollBar {}

                    delegate: WorkbenchListRow {
                        id: profileRow

                        property var profile: modelData
                        property var scoreCount: (playerSettings.updateScoreCounts, profileRow.profile.scoreDb.getTotalScoreCount())

                        width: ListView.view ? ListView.view.width : 0
                        selected: Rg.profileList.mainProfile === profile
                        primaryText: profileRow.profile.vars.generalVars.name
                        secondaryText: qsTr("Scores: %1").arg(scoreCount)

                        onClicked: {
                            Rg.profileList.mainProfile = profileRow.profile;
                        }

                        ActionButton {
                            text: qsTr("Remove")
                            tone: ActionButton.Danger

                            onClicked: {
                                confirmDeletion.profile = profileRow.profile;
                                confirmDeletion.open();
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 420
                spacing: 14

                WorkbenchPanel {
                    title: qsTr("Profile")
                    subtitle: Rg.profileList.mainProfile.vars.generalVars.name
                    Layout.fillWidth: true

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        Frame {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: 148
                            Layout.preferredHeight: 148

                            Image {
                                anchors.fill: parent
                                source: Rg.profileList.mainProfile.vars.generalVars.avatar
                                sourceSize.width: 256
                                sourceSize.height: 256
                                asynchronous: true
                                fillMode: Image.PreserveAspectFit

                                TapHandler {
                                    onTapped: {
                                        fileDialog.open();
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            TextField {
                                text: Rg.profileList.mainProfile.vars.generalVars.name
                                font.pixelSize: 22
                                color: palette.text
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter

                                onTextChanged: {
                                    Rg.profileList.mainProfile.vars.generalVars.name = text;
                                }
                            }

                            ActionButton {
                                text: qsTr("Open profile folder")
                                tone: ActionButton.Secondary
                                Layout.alignment: Qt.AlignLeft

                                onClicked: playerSettings.openProfileFolder(Rg.profileList.mainProfile)
                            }
                        }
                    }
                }

                WorkbenchPanel {
                    id: loginSection

                    title: qsTr("Online account")
                    subtitle: qsTr("Sync RhythmGame scores and import connected Bokutachi PBs.")
                    Layout.fillWidth: true

                    property var profile: Rg.profileList.mainProfile
                    property bool syncing: false
                    property int pendingOps: 0
                    property bool syncError: false

                    function runSync() {
                        loginSection.syncing = true;
                        loginSection.syncError = false;
                        loginSection.pendingOps = 3;

                        function attachOp(op) {
                            function completeOp() {
                                loginSection.pendingOps = Math.max(0, loginSection.pendingOps - 1);
                                if (loginSection.pendingOps === 0) {
                                    loginSection.syncing = false;
                                    if (!loginSection.syncError)
                                        playerSettings.updateScoreCounts++;
                                }
                            }

                            op.error.connect(function(msg) {
                                console.warn("Sync error:", msg);
                                loginSection.syncError = true;
                            });
                            if (op.finished) {
                                completeOp();
                                return;
                            }
                            op.finishedChanged.connect(function() {
                                completeOp();
                            });
                        }

                        attachOp(loginSection.profile.downloadScores());
                        attachOp(loginSection.profile.uploadScores());
                        attachOp(loginSection.profile.importBokutachiScores());
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
                        Layout.preferredHeight: authLoader.status === Loader.Ready && authLoader.item ? authLoader.item.implicitHeight : 0
                        Layout.fillWidth: true
                    }

                    Component {
                        id: loggingInComponent

                        RowLayout {
                            spacing: 8

                            BusyIndicator {
                                running: true
                                visible: true
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                            }

                            Label {
                                text: qsTr("Logging in...")
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Component {
                        id: loggedInComponent

                        RowLayout {
                            spacing: 10

                            Label {
                                text: qsTr("Logged in as %1").arg(loginSection.profile.onlineUserData.username)
                                Layout.fillWidth: true
                            }

                            ActionButton {
                                text: qsTr("Logout")
                                tone: ActionButton.Tertiary

                                onClicked: {
                                    loginSection.profile.logout();
                                }
                            }

                            ActionButton {
                                enabled: !loginSection.syncing
                                text: qsTr("Sync scores")
                                tone: loginSection.syncError ? ActionButton.Danger : ActionButton.Primary

                                onClicked: {
                                    loginSection.runSync();
                                }
                            }

                            BusyIndicator {
                                running: loginSection.syncing
                                visible: loginSection.syncing
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }
                        }
                    }

                    Component {
                        id: loggedOutComponent

                        RowLayout {
                            spacing: 8

                            TextField {
                                id: emailField

                                placeholderText: qsTr("Email")
                                Layout.fillWidth: true

                                onAccepted: {
                                    passwordField.forceActiveFocus();
                                }
                            }

                            TextField {
                                id: passwordField

                                placeholderText: qsTr("Password")
                                echoMode: TextInput.Password
                                Layout.fillWidth: true

                                onAccepted: {
                                    loginSection.profile.login(emailField.text, passwordField.text);
                                }
                            }

                            ActionButton {
                                text: qsTr("Login")
                                tone: loginSection.profile.loginState === Profile.LoginFailed ? ActionButton.Danger : ActionButton.Primary

                                onClicked: loginSection.profile.login(emailField.text, passwordField.text)
                            }
                        }
                    }
                }

                WorkbenchPanel {
                    id: scoreImportSection

                    title: qsTr("Import score database")
                    subtitle: qsTr("Import scores from LR2 score.db or beatoraja score.db.")
                    Layout.fillWidth: true

                    readonly property var op: Rg.profileList.mainProfile.scoreImportOperation
                    readonly property bool importing: op !== null && !op.finished

                    Connections {
                        target: scoreImportSection.op

                        function onFinishedChanged() {
                            playerSettings.updateScoreCounts++;
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ActionButton {
                            text: qsTr("Select database...")
                            tone: ActionButton.Primary
                            Layout.preferredWidth: 200
                            enabled: !scoreImportSection.importing

                            onClicked: scoreDatabaseDialog.open()
                        }

                        BusyIndicator {
                            running: scoreImportSection.importing
                            visible: scoreImportSection.importing
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        visible: scoreImportSection.op !== null
                        value: scoreImportSection.op !== null
                               ? scoreImportSection.op.done / Math.max(scoreImportSection.op.total, 1)
                               : 0
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        visible: scoreImportSection.op !== null
                        text: qsTr("Imported: %1, errors: %2, skipped: %3, total: %4")
                            .arg(scoreImportSection.op ? scoreImportSection.op.imported : 0)
                            .arg(scoreImportSection.op ? scoreImportSection.op.errored : 0)
                            .arg(scoreImportSection.op ? scoreImportSection.op.skipped : 0)
                            .arg(scoreImportSection.op ? scoreImportSection.op.total : 0)
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 80
                        Layout.preferredHeight: Math.min(160, contentHeight)
                        visible: scoreImportSection.op !== null && scoreImportSection.op.count > 0
                        model: scoreImportSection.op
                        spacing: 2
                        clip: true
                        ScrollBar.vertical: ScrollBar {}

                        delegate: Label {
                            required property string message

                            width: ListView.view ? ListView.view.width : 0
                            wrapMode: Text.Wrap
                            color: SettingsColors.dangerText(palette)
                            text: message
                        }
                    }
                }

                WorkbenchPanel {
                    id: replayImportSection

                    title: qsTr("Import replays")
                    subtitle: replayFolderSettings.folder === ""
                        ? qsTr("e.g. beatoraja/player/player1/replay or LR2files/Replay/player1")
                        : replayFolderSettings.folder
                    Layout.fillWidth: true

                    readonly property var op: Rg.profileList.mainProfile.replayImportOperation
                    readonly property bool importing: op !== null && !op.finished

                    Settings {
                        id: replayFolderSettings

                        category: "replayImportFolder/" + Rg.profileList.mainProfile.guid
                        property string folder: ""
                        onCategoryChanged: folder = value("folder", "")
                    }

                    Connections {
                        target: replayImportSection.op

                        function onFinishedChanged() {
                            playerSettings.updateScoreCounts++;
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ActionButton {
                            text: qsTr("Import")
                            tone: ActionButton.Primary
                            Layout.preferredWidth: 160
                            enabled: replayFolderSettings.folder !== "" && !replayImportSection.importing

                            onClicked: Rg.profileList.mainProfile.importReplays(replayFolderSettings.folder)
                        }

                        ActionButton {
                            text: replayFolderSettings.folder === "" ? qsTr("Select...") : qsTr("Change...")
                            tone: ActionButton.Secondary
                            enabled: !replayImportSection.importing

                            onClicked: replayFolderDialog.open()
                        }

                        BusyIndicator {
                            running: replayImportSection.importing
                            visible: replayImportSection.importing
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
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
                        Layout.minimumHeight: 100
                        Layout.preferredHeight: Math.min(180, contentHeight)
                        visible: replayImportSection.op !== null && replayImportSection.op.count > 0
                        model: replayImportSection.op
                        spacing: 2
                        clip: true
                        ScrollBar.vertical: ScrollBar {}

                        delegate: Label {
                            required property string message

                            width: ListView.view ? ListView.view.width : 0
                            wrapMode: Text.Wrap
                            color: SettingsColors.dangerText(palette)
                            text: message
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
