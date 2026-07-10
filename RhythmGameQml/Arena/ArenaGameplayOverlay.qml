pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var session
    property bool expanded: false
    property int unreadCount: 0

    function stateText(connected, state) : string {
        if (!connected) {
            return qsTr("Disconnected");
        }
        switch (state) {
        case "playing":
            return qsTr("Playing");
        case "finished":
            return qsTr("Finished");
        case "dnf":
            return qsTr("DNF");
        default:
            return qsTr("Waiting");
        }
    }

    function rankText(rank) : string {
        return rank > 0 ? String(rank) : "—";
    }

    function scoreText(hasScore, exScore) : string {
        return hasScore ? String(exScore) : "—";
    }

    function progressText(progressPermille) : string {
        return qsTr("%1%").arg(Math.round(progressPermille / 10));
    }

    function gaugeText(gaugeType, gaugeValueMilli) : string {
        return qsTr("%1 · %2%").arg(gaugeTypeText(gaugeType))
            .arg((gaugeValueMilli / 1000).toFixed(1));
    }

    function gaugeTypeText(value) : string {
        switch (String(value || "")) {
        case "fc": return qsTr("Full combo");
        case "exhard": return qsTr("EX Hard");
        case "hard": return qsTr("Hard");
        case "normal": return qsTr("Normal");
        case "easy": return qsTr("Easy");
        case "aeasy": return qsTr("Assist Easy");
        default: return qsTr("Unknown");
        }
    }

    function clearTypeText(value) : string {
        switch (String(value || "")) {
        case "max": return qsTr("MAX");
        case "perfect": return qsTr("Perfect");
        case "fc": return qsTr("Full combo");
        case "exhard": return qsTr("EX Hard");
        case "hard": return qsTr("Hard");
        case "normal": return qsTr("Normal");
        case "easy": return qsTr("Easy");
        case "aeasy": return qsTr("Assist Easy");
        case "failed": return qsTr("Failed");
        default: return qsTr("Unknown");
        }
    }

    function dnfReasonText(value) : string {
        switch (String(value || "")) {
        case "aborted": return qsTr("Aborted");
        case "result_unavailable": return qsTr("Result unavailable");
        case "left": return qsTr("Left the room");
        case "kicked": return qsTr("Kicked");
        case "grace_expired": return qsTr("Reconnect grace expired");
        case "play_deadline": return qsTr("Play deadline expired");
        default: return qsTr("Unknown");
        }
    }

    function outcomeText(clearType, lobbyWinsAfter, dnfReason) : string {
        const parts = [];
        if (String(dnfReason || "").length > 0) {
            parts.push(dnfReasonText(dnfReason));
        } else if (String(clearType || "").length > 0) {
            parts.push(qsTr("%1 clear").arg(clearTypeText(clearType)));
        }
        if (Number(lobbyWinsAfter) >= 0) {
            parts.push(qsTr("%n win(s)", "Arena lobby wins",
                            Number(lobbyWinsAfter)));
        }
        return parts.join(qsTr(" · "));
    }

    border.color: "#70ffffff"
    border.width: 1
    color: "#e6101218"
    implicitHeight: 360
    implicitWidth: 420
    radius: 6

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                color: "white"
                font.bold: true
                font.pixelSize: 18
                text: qsTr("Arena")
            }

            Button {
                objectName: "arenaGameplayExpand"
                text: root.expanded ? qsTr("Compact") : qsTr("Expand")
                onClicked: root.expanded = !root.expanded
            }

            Button {
                text: root.session.gameplayChatOpen === true
                    ? qsTr("Close chat")
                    : (root.unreadCount > 0
                       ? qsTr("Chat (%1)").arg(root.unreadCount)
                       : qsTr("Chat"))
                onClicked: root.session.toggleGameplayChat()
            }
        }

        Text {
            objectName: "arenaGameplayOptions"
            Layout.fillWidth: true
            color: "#d8ffffff"
            text: qsTr("Options: %1").arg(root.session.arenaOptionsSummary || "")
            textFormat: Text.PlainText
            visible: root.expanded
                && String(root.session.arenaOptionsSummary || "").length > 0
            wrapMode: Text.Wrap
        }

        ListView {
            id: standingsView

            objectName: "arenaGameplayStandings"

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 80
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.session.liveStandings
            reuseItems: true
            spacing: 4

            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: standingDelegate

                required property int index
                required property string memberId
                required property string displayName
                required property bool connected
                required property string competitionState
                required property int rank
                required property bool hasScore
                required property var exScore
                required property int progressPermille
                required property int maxCombo
                required property int badPoorCount
                required property int perfect
                required property int great
                required property int good
                required property int bad
                required property int poor
                required property int emptyPoor
                required property string gaugeType
                required property int gaugeValueMilli
                required property string clearType
                required property int lobbyWinsAfter
                required property string dnfReason

                readonly property bool localMember:
                    memberId === String(root.session.selfMemberId || "")
                readonly property bool opponentTarget:
                    root.session.opponentTarget !== null
                    && root.session.opponentTarget !== undefined
                    && memberId === String(
                        root.session.opponentTarget.memberId || "")

                objectName: "arenaStandingRow" + index
                color: index % 2 === 0 ? "#241b2230" : "#141b2230"
                height: standingContent.implicitHeight + 10
                radius: 3
                width: ListView.view.width

                ColumnLayout {
                    id: standingContent

                    spacing: 2
                    anchors {
                        fill: parent
                        leftMargin: 8
                        rightMargin: 8
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            objectName: "arenaStandingRank"
                            Layout.preferredWidth: 22
                            color: "#b9ffffff"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            text: root.rankText(standingDelegate.rank)
                        }

                        Text {
                            objectName: "arenaStandingLocalMark"
                            color: "#9ee6ff"
                            font.bold: true
                            text: qsTr("YOU")
                            textFormat: Text.PlainText
                            visible: standingDelegate.localMember
                        }

                        Text {
                            objectName: "arenaStandingTargetMark"
                            color: "#cbb8ff"
                            font.bold: true
                            text: qsTr("RIVAL")
                            textFormat: Text.PlainText
                            visible: standingDelegate.opponentTarget
                        }

                        Text {
                            objectName: "arenaStandingName"
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            text: standingDelegate.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            objectName: "arenaStandingScore"
                            color: "#ffe38a"
                            font.bold: true
                            text: qsTr("EX %1").arg(root.scoreText(
                                standingDelegate.hasScore,
                                standingDelegate.exScore))
                        }

                        Text {
                            objectName: "arenaStandingProgress"
                            Layout.preferredWidth: 42
                            color: "#d8ffffff"
                            horizontalAlignment: Text.AlignRight
                            text: root.progressText(standingDelegate.progressPermille)
                        }

                        Text {
                            objectName: "arenaStandingState"
                            Layout.preferredWidth: 82
                            color: standingDelegate.connected ? "#b9ffffff" : "#ff9b9b"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                            text: root.stateText(standingDelegate.connected,
                                                 standingDelegate.competitionState)
                        }
                    }

                    ColumnLayout {
                        objectName: "arenaStandingDetails"
                        Layout.fillWidth: true
                        spacing: 1
                        visible: root.expanded

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("BP %1 · Combo %2")
                                .arg(standingDelegate.badPoorCount)
                                .arg(standingDelegate.maxCombo)
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("PG %1 · GR %2 · GD %3")
                                .arg(standingDelegate.perfect)
                                .arg(standingDelegate.great)
                                .arg(standingDelegate.good)
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: qsTr("BD %1 · PR %2 · EP %3")
                                .arg(standingDelegate.bad)
                                .arg(standingDelegate.poor)
                                .arg(standingDelegate.emptyPoor)
                        }

                        Text {
                            objectName: "arenaStandingGauge"
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: root.gaugeText(standingDelegate.gaugeType,
                                                 standingDelegate.gaugeValueMilli)
                            textFormat: Text.PlainText
                        }
                    }

                    Text {
                        objectName: "arenaStandingOutcome"
                        Layout.fillWidth: true
                        color: standingDelegate.dnfReason.length > 0
                            ? "#ffb0b0" : "#b9ffffff"
                        elide: Text.ElideRight
                        text: root.outcomeText(standingDelegate.clearType,
                                               standingDelegate.lobbyWinsAfter,
                                               standingDelegate.dnfReason)
                        textFormat: Text.PlainText
                        visible: text.length > 0
                    }
                }
            }
        }
    }
}
