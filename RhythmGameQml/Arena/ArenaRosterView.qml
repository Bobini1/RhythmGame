pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

FocusScope {
    id: root

    required property var session
    required property bool moderationEnabled
    property bool compact: false
    readonly property int memberCount: memberList.count
    readonly property int connectedCount: connectionCounter.connectedCount
    readonly property int reservedCount: Math.max(0, memberCount - connectedCount)
    readonly property real contentHeight: memberList.contentHeight

    signal kickRequested(string memberId)

    ArenaTypography {
        id: typography
    }

    function markerText(owner, self, memberId): string {
        let markers = [];
        if (self) {
            markers.push(qsTr("you"));
        }
        if (owner) {
            markers.push(qsTr("owner"));
        }
        if (root.session && String(root.session.selectedByMemberId || "") === memberId) {
            markers.push(qsTr("selected"));
        }
        return markers.join(" · ");
    }

    function roundStateText(roundState): string {
        switch (roundState) {
        case "waiting":
            return qsTr("Waiting");
        case "probing":
            return qsTr("Checking chart");
        case "loading":
            return qsTr("Loading");
        case "loaded":
            return qsTr("Loaded");
        case "playing":
            return qsTr("Playing");
        default:
            return "";
        }
    }

    function statusText(connected, ready, inventoryState, inventoryRevision, availabilityAppliedRevision, roundState): string {
        let states = [];
        states.push(connected ? qsTr("Connected") : qsTr("Reserved"));
        if (inventoryState === "missing") {
            states.push(qsTr("Library unavailable"));
        } else if (inventoryState === "syncing") {
            states.push(qsTr("Syncing library"));
        } else if (availabilityAppliedRevision < inventoryRevision) {
            states.push(qsTr("Updating availability"));
        }
        const roundText = root.roundStateText(roundState);
        if (roundText.length > 0) {
            states.push(roundText);
        } else {
            states.push(ready ? qsTr("Ready") : qsTr("Not ready"));
        }
        return states.join(" · ");
    }

    function statusColor(connected, ready, inventoryState, inventoryRevision, availabilityAppliedRevision, roundState): color {
        if (!connected || inventoryState === "missing") {
            return "#ef6a62";
        }
        if (roundState === "probing" || roundState === "loading"
                || roundState === "loaded" || roundState === "playing") {
            return "#63d47b";
        }
        if (inventoryState === "syncing"
                || availabilityAppliedRevision < inventoryRevision) {
            return "#f0c75e";
        }
        if (ready) {
            return "#63d47b";
        }
        return "#f0c75e";
    }

    ListView {
        id: memberList

        objectName: "arenaRosterList"
        Accessible.role: Accessible.List
        anchors.fill: parent
        activeFocusOnTab: true
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        model: root.session ? root.session.members : null
        reuseItems: true
        keyNavigationEnabled: true
        spacing: root.compact ? 2 : 4

        function ensureCurrentItem(): void {
            if (memberList.count === 0) {
                memberList.currentIndex = -1;
            } else if (memberList.currentIndex < 0 || memberList.currentIndex >= memberList.count) {
                memberList.currentIndex = 0;
            }
        }

        Component.onCompleted: memberList.ensureCurrentItem()
        onCountChanged: memberList.ensureCurrentItem()

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: Rectangle {
            id: memberDelegate

            required property int index
            required property string memberId
            required property string displayName
            required property string avatarUrl
            required property bool connected
            required property bool owner
            required property bool self
            required property int lobbyWins
            required property bool ready
            required property string inventoryState
            required property var inventoryRevision
            required property var availabilityAppliedRevision
            required property string roundState

            objectName: "arenaRosterMember-" + memberDelegate.memberId
            Accessible.name: memberDelegate.displayName
            Accessible.role: Accessible.ListItem
            border.color: ListView.isCurrentItem && ListView.view.activeFocus ? "#8ec5ff" : "transparent"
            border.width: ListView.isCurrentItem && ListView.view.activeFocus ? 2 : 0
            color: memberDelegate.self ? "#263b5070" : (memberDelegate.index % 2 === 0 ? "#161b2230" : "#0d1b2230")
            height: Math.max(root.compact ? 56 : 68, memberContent.implicitHeight + 8)
            radius: 3
            width: ListView.view.width

            RowLayout {
                id: memberContent

                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 6
                anchors.bottomMargin: 4
                anchors.topMargin: 4
                spacing: 6

                ArenaAvatar {
                    objectName: "arenaRosterAvatar-" + memberDelegate.memberId
                    Layout.preferredHeight: root.compact ? 48 : 56
                    Layout.preferredWidth: root.compact ? 48 : 56
                    avatarUrl: memberDelegate.avatarUrl
                    connected: memberDelegate.connected
                    displayName: memberDelegate.displayName
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Rectangle {
                            Layout.preferredHeight: 14
                            Layout.preferredWidth: 14
                            Accessible.ignored: true
                            border.color: "#99ffffff"
                            border.width: 1
                            color: root.statusColor(
                                memberDelegate.connected,
                                memberDelegate.ready,
                                memberDelegate.inventoryState,
                                Number(memberDelegate.inventoryRevision),
                                Number(memberDelegate.availabilityAppliedRevision),
                                memberDelegate.roundState)
                            radius: 7

                            ToolTip.text: statusLabel.text
                            ToolTip.visible: statusHover.hovered

                            HoverHandler {
                                id: statusHover
                            }
                        }

                        Text {
                            id: nameLabel

                            objectName: "arenaRosterName-" + memberDelegate.memberId
                            Accessible.ignored: true
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: memberDelegate.self || memberDelegate.owner
                            font.pixelSize: typography.bodyPixelSize
                            text: memberDelegate.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            id: winsLabel

                            objectName: "arenaRosterWins-" + memberDelegate.memberId
                            Accessible.ignored: true
                            color: "#d6deea"
                            font.pixelSize: typography.supportingPixelSize
                            text: qsTr("%n win(s)", "Arena lobby wins", memberDelegate.lobbyWins)
                            textFormat: Text.PlainText
                        }

                        Button {
                            objectName: "arenaRosterKick-" + memberDelegate.memberId
                            enabled: !root.session || root.session.reconnecting !== true
                            focusPolicy: Qt.NoFocus
                            font.pixelSize: typography.supportingPixelSize
                            Layout.minimumHeight: 32
                            Layout.minimumWidth: 48
                            text: qsTr("Kick")
                            visible: root.session && root.moderationEnabled && root.session.isOwner === true && !memberDelegate.self
                            onClicked: root.kickRequested(memberDelegate.memberId)
                        }
                    }

                    Text {
                        id: markerLabel

                        objectName: "arenaRosterMarkers-" + memberDelegate.memberId
                        Accessible.ignored: true
                        Layout.fillWidth: true
                        color: "#ffe39b"
                        elide: Text.ElideRight
                        font.pixelSize: typography.supportingPixelSize
                        text: root.markerText(memberDelegate.owner, memberDelegate.self, memberDelegate.memberId)
                        textFormat: Text.PlainText
                        visible: text.length > 0
                    }

                    Text {
                        id: statusLabel

                        objectName: "arenaRosterStatus-" + memberDelegate.memberId
                        Accessible.ignored: true
                        Layout.fillWidth: true
                        color: memberDelegate.connected ? "#c9d2df" : "#ffb2a8"
                        elide: Text.ElideRight
                        font.pixelSize: typography.supportingPixelSize
                        text: root.statusText(memberDelegate.connected, memberDelegate.ready, memberDelegate.inventoryState, Number(memberDelegate.inventoryRevision), Number(memberDelegate.availabilityAppliedRevision), memberDelegate.roundState)
                        textFormat: Text.PlainText
                        visible: !root.compact
                    }
                }
            }
        }
    }

    Instantiator {
        id: connectionCounter

        property int connectedCount: 0

        model: root.session ? root.session.members : null

        delegate: QtObject {
            id: connectionDelegate

            required property bool connected
            property bool countedConnected: connected

            Component.onCompleted: {
                if (connectionDelegate.countedConnected) {
                    connectionCounter.connectedCount += 1;
                }
            }
            Component.onDestruction: {
                if (connectionDelegate.countedConnected) {
                    connectionCounter.connectedCount -= 1;
                }
            }
            onConnectedChanged: {
                if (connectionDelegate.connected === connectionDelegate.countedConnected) {
                    return;
                }
                connectionCounter.connectedCount += connectionDelegate.connected ? 1 : -1;
                connectionDelegate.countedConnected = connectionDelegate.connected;
            }
        }
    }
}
