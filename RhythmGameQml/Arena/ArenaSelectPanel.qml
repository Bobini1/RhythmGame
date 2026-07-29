pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    required property var session
    property Item navigationFocusTarget: null
    property string readyShortcutDescription: ""
    readonly property bool arenaNativeSelectPresentation: true
    readonly property alias announcementCount: statusAnnouncer.announcementCount
    readonly property alias lastAnnouncementKey: statusAnnouncer.lastAnnouncementKey
    readonly property alias lastAnnouncementText: statusAnnouncer.lastAnnouncementText
    readonly property alias dragHandle: selectHeader
    readonly property bool chatOpen: root.session && root.session.chatOpen === true
    readonly property bool preparingRound: root.session
        ? String(root.session.currentRoundId || "").length > 0 : false
    readonly property real rosterColumnWidth: 360
    readonly property real normalBodyMinimumWidth: rosterColumnWidth + 8 + 220
    readonly property bool thinDetailsMode: !chatOpen
        && width - 20 < normalBodyMinimumWidth
    readonly property bool chatInputActive: root.chatOpen
        && detailsLoader.status === Loader.Ready
        && detailsLoader.item !== null
        && detailsLoader.item.inputActiveFocus === true
    readonly property string readyDisabledReason: {
        if (!root.session) {
            return "";
        }
        if (root.session.roundsAvailable === false) {
            return qsTr("Update required to play in this room.");
        }
        if (root.session.availabilitySyncing === true) {
            return qsTr("Song libraries are still being compared.");
        }
        if (root.preparingRound) {
            return qsTr("The synchronized round is already being prepared.");
        }
        if (root.session.ready !== true && root.session.canReady !== true) {
            return qsTr("Choose a chart available to everyone before becoming ready.");
        }
        return "";
    }
    readonly property string readyToolTipText: {
        if (root.readyDisabledReason.length === 0) {
            return root.readyShortcutDescription;
        }
        if (root.readyShortcutDescription.length === 0) {
            return root.readyDisabledReason;
        }
        return root.readyDisabledReason + "\n" + root.readyShortcutDescription;
    }

    signal chatSelected(bool chat)

    Accessible.name: root.session
        ? (root.session.roomName || qsTr("Arena room")) : qsTr("Arena room")
    Accessible.role: Accessible.Grouping

    function restoreNavigationFocus(): void {
        Qt.callLater(function () {
            if (!root.chatInputActive && root.navigationFocusTarget)
                root.navigationFocusTarget.forceActiveFocus();
        });
    }

    ArenaTypography {
        id: typography
    }

    Rectangle {
        anchors.fill: parent
        border.color: "#74859a"
        border.width: 1
        color: "#ed111821"
        radius: 5
    }

    MouseArea {
        objectName: "arenaSelectWheelSink"
        anchors.fill: parent
        acceptedButtons: Qt.NoButton

        onWheel: wheel => wheel.accepted = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            id: selectHeader

            readonly property real contentHeight: Math.max(
                40, roomTitle.implicitHeight + 12, tabs.implicitHeight)

            objectName: "arenaSelectHeader"
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.minimumHeight: contentHeight
            Layout.preferredHeight: contentHeight
            spacing: 4

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 24

                Accessible.ignored: true

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Repeater {
                        model: 3

                        Rectangle {
                            color: "#8b96a6"
                            height: 2
                            width: 18
                        }
                    }
                }
            }

            Text {
                id: roomTitle

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                objectName: "arenaSelectRoomTitle"
                color: "white"
                elide: Text.ElideRight
                font.bold: true
                font.pixelSize: typography.scaled(18)
                text: root.session
                    ? (root.session.roomName || qsTr("Arena room"))
                    : qsTr("Arena room")
                textFormat: Text.PlainText
                verticalAlignment: Text.AlignVCenter
            }

            ArenaPanelTabs {
                id: tabs

                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: false
                Layout.preferredHeight: implicitHeight
                chatAccessibleName: qsTr("Show Arena chat")
                detailsAccessibleName: qsTr("Show Arena room details")
                session: root.session

                onTabSelected: chat => root.chatSelected(chat)
            }

            HoverHandler {
                enabled: !tabs.hovered
                cursorShape: Qt.SizeAllCursor
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumHeight: actionStripContent.implicitHeight + 14
            Layout.preferredHeight: actionStripContent.implicitHeight + 14
            color: "#241b2230"
            radius: 3

            RowLayout {
                id: actionStripContent

                anchors.fill: parent
                anchors.margins: 7
                spacing: 8

                ArenaSelectionSummary {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    compact: true
                    fillRemainder: false
                    session: root.session
                    titleOnly: true
                }

                CheckBox {
                    id: readyButton

                    readonly property bool ready: root.session
                        && (root.session.ready === true || root.preparingRound)

                    objectName: "arenaSelectReady"
                    Accessible.description: root.readyToolTipText
                    Accessible.name: text
                    checkState: ready ? Qt.Checked : Qt.Unchecked
                    enabled: root.session
                        && root.session.roundsAvailable !== false
                        && !root.preparingRound
                        && (ready || root.session.canReady === true)
                    focusPolicy: Qt.NoFocus
                    font.pixelSize: typography.bodyPixelSize
                    nextCheckState: function() {
                        return checkState;
                    }
                    text: qsTr("Ready")

                    ToolTip.text: root.readyToolTipText
                    ToolTip.visible: hovered
                        && root.readyToolTipText.length > 0

                    onClicked: {
                        if (root.session)
                            root.session.setReady(!ready);
                    }
                }
            }
        }

        ArenaSelectionSummary {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            compact: true
            fillRemainder: false
            session: root.session
            showTitle: false
            visible: root.thinDetailsMode
        }

        GridLayout {
            id: bodyLayout

            objectName: "arenaSelectBody"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            clip: true
            columnSpacing: 8
            columns: root.chatOpen || root.thinDetailsMode ? 1 : 2
            rowSpacing: 8

            ArenaRosterView {
                objectName: "arenaSelectRoster"
                Layout.column: 0
                Layout.fillHeight: true
                Layout.fillWidth: root.thinDetailsMode
                Layout.maximumWidth: root.thinDetailsMode
                    ? Number.POSITIVE_INFINITY : root.rosterColumnWidth
                Layout.minimumHeight: 0
                Layout.minimumWidth: root.thinDetailsMode
                    ? 0 : root.rosterColumnWidth
                Layout.preferredWidth: root.thinDetailsMode
                    ? -1 : root.rosterColumnWidth
                Layout.row: 0
                compact: true
                moderationEnabled: true
                session: root.session
                visible: !root.chatOpen
                onKickRequested: memberId => {
                    if (root.session)
                        root.session.kickMember(memberId);
                }
            }

            Loader {
                id: detailsLoader

                Layout.column: root.chatOpen ? 0 : 1
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: 0
                Layout.minimumWidth: root.chatOpen ? 0 : 220
                Layout.row: 0
                sourceComponent: root.chatOpen
                    ? chatComponent : summaryComponent
                visible: root.chatOpen || !root.thinDetailsMode
            }
        }

        ArenaChatActivityRail {
            Layout.fillWidth: true
            session: root.session
        }

    }

    ArenaStatusAnnouncer {
        id: statusAnnouncer

        active: root.visible
        errorMessageKey: root.session ? String(root.session.errorMessageKey || "") : ""
        reconnecting: root.session ? root.session.reconnecting === true : false
        roundLaunchCancellationStatusKey: root.session
            ? String(root.session.roundLaunchCancellationStatusKey || "") : ""
        target: root
    }

    Component {
        id: summaryComponent

        ScrollView {
            id: detailsScroll

            objectName: "arenaSelectSelection"
            clip: true
            contentWidth: availableWidth
            readonly property Flickable scrollFlickable: contentItem as Flickable
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            function scrollByWheel(wheel): void {
                const flickable = detailsScroll.scrollFlickable;
                let delta = wheel.pixelDelta.y;
                if (delta === 0) {
                    delta = wheel.angleDelta.y / 3;
                }
                if (wheel.inverted) {
                    delta = -delta;
                }
                const minimumY = flickable.originY;
                const maximumY = Math.max(minimumY,
                    minimumY + flickable.contentHeight - flickable.height);
                flickable.contentY = Math.max(minimumY,
                    Math.min(maximumY, flickable.contentY - delta));
            }

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                target: null
                onWheel: wheel => detailsScroll.scrollByWheel(wheel)
            }

            ArenaSelectionSummary {
                id: selectionSummary

                compact: true
                showTitle: false
                session: root.session
                width: detailsScroll.availableWidth
            }
        }
    }

    Component {
        id: chatComponent

        ArenaChatView {
            objectName: "arenaSelectChat"
            chatModel: root.session ? root.session.chat : null
            focusFallback: root.navigationFocusTarget
            inputEnabled: true
            session: root.session
        }
    }
}
