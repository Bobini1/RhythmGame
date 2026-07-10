pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var session
    required property string placementKind
    required property string resolvedSkinId
    required property string layoutVariant
    property bool expanded: false

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
        return qsTr("%1 · %2%").arg(gaugeType).arg((gaugeValueMilli / 10).toFixed(1));
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
                text: root.expanded ? qsTr("Compact") : qsTr("Expand")
                onClicked: root.expanded = !root.expanded
            }

            Button {
                text: root.session.gameplayChatOpen === true
                    ? qsTr("Close chat")
                    : qsTr("Chat")
                onClicked: root.session.toggleGameplayChat()
            }
        }

        Text {
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
                            Layout.preferredWidth: 22
                            color: "#b9ffffff"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            text: root.rankText(standingDelegate.rank)
                        }

                        Text {
                            Layout.fillWidth: true
                            color: "white"
                            elide: Text.ElideRight
                            font.bold: true
                            text: standingDelegate.displayName
                            textFormat: Text.PlainText
                        }

                        Text {
                            color: "#ffe38a"
                            font.bold: true
                            text: qsTr("EX %1").arg(root.scoreText(
                                standingDelegate.hasScore,
                                standingDelegate.exScore))
                        }

                        Text {
                            Layout.preferredWidth: 42
                            color: "#d8ffffff"
                            horizontalAlignment: Text.AlignRight
                            text: root.progressText(standingDelegate.progressPermille)
                        }

                        Text {
                            Layout.preferredWidth: 82
                            color: standingDelegate.connected ? "#b9ffffff" : "#ff9b9b"
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                            text: root.stateText(standingDelegate.connected,
                                                 standingDelegate.competitionState)
                        }
                    }

                    ColumnLayout {
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
                            Layout.fillWidth: true
                            color: "#d8ffffff"
                            text: root.gaugeText(standingDelegate.gaugeType,
                                                 standingDelegate.gaugeValueMilli)
                            textFormat: Text.PlainText
                        }
                    }
                }
            }
        }

        Loader {
            id: chatLoader

            Layout.fillWidth: true
            Layout.preferredHeight: active
                ? Math.min(250, Math.max(120, root.height * 0.42))
                : 0
            active: root.session.gameplayChatOpen === true
            focus: active
            sourceComponent: gameplayChatComponent
        }
    }

    Component {
        id: gameplayChatComponent

        ArenaGameplayChat {
            session: root.session
        }
    }
}
