import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: field
    required property GameplayContext gameplay
    property int playerIndex: 0
    readonly property Player player: gameplay.players[playerIndex]
    readonly property var vars: player.profile.vars.generalVars

    // [columns]
    readonly property var columns: field.player.columns
    // [columns]

    Label {
        Layout.fillWidth: true
        text: qsTr("%1: %2 points, %3 combo").arg(field.player.profile.vars.generalVars.name)
            .arg(field.player.score.points).arg(field.player.score.combo)
        elide: Text.ElideRight
    }

    // [score-target]
    StandardGameplayTarget {
        id: scoreTarget
        gameplay: field.gameplay
        playerIndex: field.playerIndex
    }
    Label {
        Layout.fillWidth: true
        visible: scoreTarget.available
        text: qsTr("Target: %1 points").arg(Math.floor(scoreTarget.points))
        elide: Text.ElideRight
    }
    // [score-target]

    // [visible-range]
    StandardGameplayViewport {
        id: viewport
        player: field.player
        chartData: field.gameplay.chartData
        noteHeight: lanes.height
        visibleHeight: lanes.height - laneCover.height
    }
    // [visible-range]

    onPlayerChanged: judgementTimer.stop()

    // [covers]
    Rectangle {
        id: playArea
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#243341"

        readonly property real liftRatio: field.vars.liftOn
            ? Math.max(0, Math.min(1, field.vars.liftRatio)) : 0

        RowLayout {
            id: lanes
            anchors.fill: parent
            anchors.bottomMargin: Math.round(playArea.height * playArea.liftRatio)
            spacing: 2
            uniformCellSizes: true

            Repeater {
                model: field.columns
                NoteLane {
                    required property int modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    player: field.player
                    columnState: field.player.state.columnStates[modelData]
                    pixelsPerBeat: viewport.pixelsPerBeat
                    scratch: field.player.isScratchColumn(modelData)
                }
            }
        }
        // [bar-lines]
        Item {
            anchors.fill: lanes
            clip: true

            Repeater {
                model: field.player.state.barLinesState
                Rectangle {
                    required property var display
                    width: lanes.width
                    height: 1
                    y: lanes.height - (display.time.position - field.player.position) * viewport.pixelsPerBeat
                    color: "#708090"
                }
            }
        }
        // [bar-lines]
        Rectangle {
            id: laneCover
            anchors.top: lanes.top
            width: lanes.width
            height: field.vars.laneCoverOn
                ? lanes.height * Math.max(0, Math.min(1, field.vars.laneCoverRatio)) : 0
            color: "#243341"
        }
        Rectangle {
            anchors.bottom: lanes.bottom
            width: lanes.width
            height: field.vars.hiddenOn
                ? lanes.height * Math.max(0, Math.min(1, field.vars.hiddenRatio)) : 0
            color: "#243341"
        }
        Rectangle {
            anchors.bottom: lanes.bottom
            width: lanes.width
            height: 2
            color: "white"
        }
    }
    // [covers]

    // [gauge]
    GaugeBar {
        Layout.fillWidth: true
        gauges: field.player.score.gauges
    }
    // [gauge]

    Label {
        Layout.fillWidth: true
        text: qsTr("%1 ms · Green number %2").arg(Math.round(field.vars.noteScreenTimeMillis))
            .arg(Math.round(field.vars.noteScreenTimeMillis * 0.6))
        elide: Text.ElideRight
    }

    // [judgement-feedback]
    Label {
        id: judgementText
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 24
        font.bold: true
        opacity: judgementTimer.running ? 1 : 0

        property int judgement: -1
        readonly property var styles: ({
            [Judgement.Perfect]: { text: qsTr("PGREAT"), color: "#9de6ff" },
            [Judgement.Great]: { text: qsTr("GREAT"), color: "#ffe28a" },
            [Judgement.Good]: { text: qsTr("GOOD"), color: "#a8dcc9" },
            [Judgement.Bad]: { text: qsTr("BAD"), color: "#ef9ad0" },
            [Judgement.Poor]: { text: qsTr("POOR"), color: "#ef7777" },
            [Judgement.EmptyPoor]: { text: qsTr("POOR"), color: "#ef7777" },
            [Judgement.MineHit]: { text: qsTr("MINE"), color: "#ef7777" }
        })
        text: styles[judgement]?.text || ""
        color: styles[judgement]?.color || "white"

        Timer {
            id: judgementTimer
            interval: 500
        }
        Connections {
            target: field.player.score
            enabled: field.enabled
            function onHit(hit) {
                if (!judgementText.styles[hit.judgement]) return;
                judgementText.judgement = hit.judgement;
                judgementTimer.restart();
            }
        }
    }
    // [judgement-feedback]
}
