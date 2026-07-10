import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: root

    required property var session
    required property Profile activeProfile

    signal createRequested(string name, string password)
    signal joinRequested(string roomId, string password)
    signal retryRequested()
    signal exitRequested()

    property string dialogMode: "none"
    property string selectedRoomId: ""
    property string selectedRoomName: ""
    property Item dialogOrigin: null
    readonly property alias announcementCount: statusAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: statusAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: statusAnnouncer.lastAnnouncementText
    readonly property bool admissionInFlight: session.admissionPending
        && !session.loginRequired
    readonly property bool updateRequired: session.directoryReady
        && !session.competitionAvailable
    readonly property bool roomActionsEnabled: session.state === ArenaSession.Browsing
        && !admissionInFlight
        && !updateRequired

    Accessible.name: qsTr("Online Arena")
    Accessible.role: Accessible.Grouping

    function errorText(key) : string {
        switch (key) {
        case "arena.error.authRequired":
            return qsTr("Log in to create or join a room.");
        case "arena.error.alreadyInRoom":
            return qsTr("You are already in a room.");
        case "arena.error.roomNotFound":
            return qsTr("That room no longer exists.");
        case "arena.error.roomPasswordInvalid":
            return qsTr("The room password was rejected.");
        case "arena.error.roomFull":
            return qsTr("That room is full.");
        case "arena.error.roomBanned":
            return qsTr("You cannot rejoin that room.");
        case "arena.error.roomDuplicateIdentity":
            return qsTr("This account already has a seat in that room.");
        case "arena.error.invalidTicket":
            return qsTr("Your login expired. Please try the room request again.");
        case "arena.error.protocolIncompatible":
        case "arena.error.capabilityRequired":
            return qsTr("This Arena server requires a newer game version.");
        case "arena.error.resumeFailed":
            return qsTr("Your reserved seat could not be restored.");
        case "arena.error.serverShuttingDown":
        case "arena.serverGoingAway":
            return qsTr("Arena is restarting. Please try again shortly.");
        case "arena.error.tlsFailed":
            return qsTr("A secure connection to Arena could not be established.");
        case "arena.error.ticketNetwork":
        case "arena.error.ticketMalformedResponse":
            return qsTr("The ranking service could not authorize Arena right now.");
        case "arena.error.connectionFailed":
        case "arena.error.remoteClosed":
        case "arena.error.transport":
            return qsTr("Arena is unavailable right now.");
        case "arena.error.kicked":
            return qsTr("You were removed from the room.");
        case "arena.error.busy":
            return qsTr("Please wait for the current room request to finish.");
        default:
            return qsTr("Arena request failed. Please try again.");
        }
    }

    function phaseText(phase) : string {
        switch (phase) {
        case "selecting":
            return qsTr("Selecting");
        default:
            return qsTr("Unknown");
        }
    }

    function openCreateDialog(origin) : void {
        dialogOrigin = origin;
        selectedRoomId = "";
        selectedRoomName = "";
        dialogMode = "create";
    }

    function openJoinDialog(roomId, roomName, origin) : void {
        dialogOrigin = origin;
        selectedRoomId = roomId;
        selectedRoomName = roomName;
        dialogMode = "join";
    }

    function finishDialog() : void {
        const origin = dialogOrigin;
        Qt.callLater(function() {
            dialogMode = "none";
            selectedRoomId = "";
            selectedRoomName = "";
            dialogOrigin = null;
            if (origin && origin.enabled && origin.visible) {
                origin.forceActiveFocus();
            } else {
                exitButton.forceActiveFocus();
            }
        });
    }

    Component.onCompleted: exitButton.forceActiveFocus()

    Keys.onEscapePressed: event => {
        if (dialogLoader.status === Loader.Ready && dialogLoader.item) {
            dialogLoader.item.close();
        } else {
            root.exitRequested();
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Button {
                id: exitButton

                text: qsTr("Exit Arena")
                onClicked: root.exitRequested()
            }

            Label {
                Layout.fillWidth: true
                font.pixelSize: 28
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Online Arena")
            }

            Button {
                id: createButton

                enabled: root.roomActionsEnabled
                text: qsTr("Create room")
                onClicked: root.openCreateDialog(createButton)
            }
        }

        Frame {
            id: connectionBanner

            Layout.fillWidth: true
            implicitHeight: bannerRow.implicitHeight + topPadding + bottomPadding
            visible: root.session.errorMessageKey.length > 0
                || root.updateRequired
                || root.session.state === ArenaSession.Disconnected
                || root.session.state === ArenaSession.ConnectingAuthenticated
                || root.admissionInFlight

            RowLayout {
                id: bannerRow

                anchors.fill: parent
                spacing: 8

                BusyIndicator {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: 28
                    running: visible && root.visible
                    visible: root.session.errorMessageKey.length === 0
                        && !root.updateRequired
                        && (root.session.state === ArenaSession.Disconnected
                            || root.session.state === ArenaSession.ConnectingAuthenticated
                            || root.admissionInFlight)
                }

                Label {
                    Layout.fillWidth: true
                    text: {
                        if (root.updateRequired) {
                            return qsTr("Update RhythmGame to create or join Arena rooms.");
                        }
                        if (root.session.errorMessageKey.length > 0) {
                            return root.errorText(root.session.errorMessageKey);
                        }
                        if (root.session.state === ArenaSession.ConnectingAuthenticated) {
                            return qsTr("Connecting with your online account...");
                        }
                        if (root.admissionInFlight) {
                            return qsTr("Completing room request...");
                        }
                        return qsTr("Connecting to Arena...");
                    }
                    wrapMode: Text.Wrap
                }

                Button {
                    visible: root.session.state === ArenaSession.Error
                    text: qsTr("Retry")
                    onClicked: root.retryRequested()
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            active: !root.updateRequired
                && root.activeProfile.loginState !== Profile.LoggedIn
            sourceComponent: loginPanelComponent
        }

        Component {
            id: loginPanelComponent

            ArenaLoginPanel {
                profile: root.activeProfile
                actionRequired: root.session.loginRequired
                admissionAllowed: !root.updateRequired

                onLoginSubmitted: (email, password) => root.activeProfile.login(email, password)
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            BusyIndicator {
                anchors.centerIn: parent
                running: visible && root.visible
                visible: !root.session.directoryReady
                    && root.session.rooms.count === 0
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("No Arena rooms are open.")
                visible: root.session.directoryReady
                    && root.session.rooms.count === 0
            }

            ListView {
                id: roomList

                objectName: "arenaRoomList"
                Accessible.name: qsTr("Arena rooms")
                Accessible.role: Accessible.List
                activeFocusOnTab: true
                anchors.fill: parent
                clip: true
                enabled: root.session.state === ArenaSession.Browsing
                model: root.session.rooms
                reuseItems: true
                keyNavigationEnabled: true
                spacing: 8

                function ensureCurrentItem(): void {
                    if (roomList.count === 0) {
                        roomList.currentIndex = -1;
                    } else if (roomList.currentIndex < 0
                               || roomList.currentIndex >= roomList.count) {
                        roomList.currentIndex = 0;
                    }
                }

                Component.onCompleted: roomList.ensureCurrentItem()
                Keys.onPressed: event => {
                    if (event.key !== Qt.Key_Return
                            && event.key !== Qt.Key_Enter
                            && event.key !== Qt.Key_Space) {
                        return;
                    }
                    if (roomList.currentItem) {
                        roomList.currentItem.activate();
                        event.accepted = true;
                    }
                }
                onCountChanged: roomList.ensureCurrentItem()
                visible: root.session.rooms.count > 0

                ScrollBar.vertical: ScrollBar {}

                delegate: Frame {
                    id: roomDelegate

                    required property string roomId
                    required property string name
                    required property string phase
                    required property bool passwordProtected
                    required property int connectedCount
                    required property int reservedCount
                    required property int maximumCount

                    objectName: "arenaRoom-" + roomDelegate.roomId
                    Accessible.description: qsTr("%1, %2. %3 connected, %4 reserved, %5 maximum.")
                        .arg(root.phaseText(roomDelegate.phase))
                        .arg(roomDelegate.passwordProtected ? qsTr("Password required") : qsTr("Public"))
                        .arg(roomDelegate.connectedCount)
                        .arg(roomDelegate.reservedCount)
                        .arg(roomDelegate.maximumCount)
                    Accessible.name: roomDelegate.name
                    Accessible.role: Accessible.ListItem
                    height: row.implicitHeight + topPadding + bottomPadding
                    width: ListView.view.width

                    function activate(): void {
                        if (!joinButton.enabled) {
                            return;
                        }
                        if (roomDelegate.passwordProtected) {
                            root.openJoinDialog(roomDelegate.roomId,
                                                roomDelegate.name,
                                                roomDelegate);
                        } else {
                            root.joinRequested(roomDelegate.roomId, "");
                        }
                    }

                    Accessible.onPressAction: roomDelegate.activate()

                    RowLayout {
                        id: row

                        anchors.fill: parent
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Accessible.ignored: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.bold: true
                                text: roomDelegate.name
                                textFormat: Text.PlainText
                            }

                            Label {
                                Accessible.ignored: true
                                Layout.fillWidth: true
                                text: qsTr("%1 · %2").arg(root.phaseText(roomDelegate.phase)).arg(
                                    roomDelegate.passwordProtected
                                        ? qsTr("Password required")
                                        : qsTr("Public"))
                            }

                            Label {
                                Accessible.ignored: true
                                Layout.fillWidth: true
                                text: qsTr("%1 connected, %2 reserved / %3")
                                    .arg(roomDelegate.connectedCount)
                                    .arg(roomDelegate.reservedCount)
                                    .arg(roomDelegate.maximumCount)
                            }
                        }

                        Button {
                            id: joinButton

                            Accessible.name: qsTr("Join %1").arg(roomDelegate.name)
                            enabled: root.roomActionsEnabled
                                && roomDelegate.connectedCount + roomDelegate.reservedCount
                                    < roomDelegate.maximumCount
                            text: roomDelegate.connectedCount + roomDelegate.reservedCount
                                >= roomDelegate.maximumCount ? qsTr("Full") : qsTr("Join")
                            onClicked: roomDelegate.activate()
                        }
                    }

                    Rectangle {
                        Accessible.ignored: true
                        anchors.fill: parent
                        border.color: ListView.isCurrentItem && ListView.view.activeFocus ? "#2387d9" : "transparent"
                        border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0
                        color: "transparent"
                        radius: 3
                    }
                }
            }
        }
    }

    Loader {
        id: dialogLoader

        anchors.fill: parent
        active: root.dialogMode !== "none"
        sourceComponent: root.dialogMode === "create"
            ? createDialogComponent
            : root.dialogMode === "join" ? joinDialogComponent : null

        onLoaded: {
            if (status === Loader.Ready && item) {
                item.open();
            }
        }
    }

    Component {
        id: createDialogComponent

        Dialog {
            id: createDialog

            anchors.centerIn: parent
            closePolicy: Popup.CloseOnEscape
            modal: true
            standardButtons: Dialog.Ok | Dialog.Cancel
            title: qsTr("Create Arena room")
            width: Math.min(480, root.width - 48)

            onAccepted: root.createRequested(roomNameField.text.trim(),
                                              createPasswordField.text)
            onClosed: root.finishDialog()
            onOpened: {
                roomNameField.clear();
                createPasswordField.clear();
                const ok = standardButton(Dialog.Ok);
                if (ok) {
                    ok.enabled = false;
                }
                roomNameField.forceActiveFocus();
            }

            ColumnLayout {
                id: createDialogContent

                width: parent.width
                spacing: 8

                Label {
                    text: qsTr("Room name")
                }
                TextField {
                    id: roomNameField

                    Accessible.name: qsTr("Arena room name")
                    Layout.fillWidth: true
                    maximumLength: 80
                    selectByMouse: true

                    onTextChanged: {
                        const ok = createDialog.standardButton(Dialog.Ok);
                        if (ok) {
                            ok.enabled = text.trim().length > 0;
                        }
                    }
                }
                Label {
                    text: qsTr("Password (optional)")
                }
                TextField {
                    id: createPasswordField

                    Accessible.name: qsTr("Arena room password")
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    selectByMouse: true
                }
            }
        }
    }

    Component {
        id: joinDialogComponent

        Dialog {
            id: joinDialog

            anchors.centerIn: parent
            closePolicy: Popup.CloseOnEscape
            modal: true
            standardButtons: Dialog.Ok | Dialog.Cancel
            title: qsTr("Join password-protected room")
            width: Math.min(480, root.width - 48)

            onAccepted: root.joinRequested(root.selectedRoomId, joinPasswordField.text)
            onClosed: root.finishDialog()
            onOpened: {
                joinPasswordField.clear();
                joinPasswordField.forceActiveFocus();
            }

            ColumnLayout {
                id: joinDialogContent

                width: parent.width
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: root.selectedRoomName
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
                Label {
                    text: qsTr("Password")
                }
                TextField {
                    id: joinPasswordField

                    Accessible.name: qsTr("Arena room password")
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    selectByMouse: true
                }
            }
        }
    }

    ArenaStatusAnnouncer {
        id: statusAnnouncer

        active: root.visible
        errorMessageKey: String(root.session.errorMessageKey || "")
        reconnecting: root.session.reconnecting === true
        roundLaunchCancellationStatusKey: String(root.session.roundLaunchCancellationStatusKey || "")
        target: root
    }
}
