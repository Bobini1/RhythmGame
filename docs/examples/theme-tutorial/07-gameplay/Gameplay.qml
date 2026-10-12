import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

Pane {
    id: gameplayScreen
    required property GameplayContext gameplay

    // [flow]
    StandardGameplayFlow {
        id: flow
        gameplay: gameplayScreen.gameplay
    }
    // [flow]

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // [stage]
        Label {
            Layout.fillWidth: true
            Layout.maximumWidth: (gameplayScreen.availableWidth - playLayout.spacing) / 2
            text: gameplayScreen.gameplay.chartData.title
            font.pixelSize: 24
            elide: Text.ElideRight
        }
        Label {
            visible: !!gameplayScreen.gameplay.course
            text: qsTr("Stage %1 of %2").arg(gameplayScreen.gameplay.stageIndex + 1)
                .arg(gameplayScreen.gameplay.stageCount)
        }
        // [stage]

        // [play-status]
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: {
                    switch (gameplayScreen.gameplay.status) {
                    case ChartRunner.Loading: return qsTr("Loading...");
                    case ChartRunner.Ready: return qsTr("Ready");
                    case ChartRunner.Running: return qsTr("Playing");
                    case ChartRunner.Finished: return qsTr("Finished");
                    }
                }
            }
            Label {
                text: qsTr("%1 BPM").arg(gameplayScreen.gameplay.players[0].bpm.toFixed(1))
            }
            Label {
                text: qsTr("%1 / %2 s")
                    .arg(Math.floor(Math.max(0, gameplayScreen.gameplay.players[0].elapsed) / 1000000000))
                    .arg(Math.ceil(gameplayScreen.gameplay.players[0].chartLength / 1000000000))
            }
            Item { Layout.fillWidth: true }
        }
        ProgressBar {
            Layout.fillWidth: true
            Layout.maximumWidth: (gameplayScreen.availableWidth - playLayout.spacing) / 2
            from: 0
            to: Math.max(1, gameplayScreen.gameplay.players[0].chartLength)
            value: Math.max(0, gameplayScreen.gameplay.players[0].elapsed)
            Accessible.name: qsTr("Chart progress")
        }
        // [play-status]

        // [half-screen]
        RowLayout {
            id: playLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            uniformCellSizes: true
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                spacing: 16

                // [players]
                Repeater {
                    model: gameplayScreen.gameplay.players
                    PlayerField {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 1
                        gameplay: gameplayScreen.gameplay
                        playerIndex: index
                    }
                }
                // [players]
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0

                // [bga]
                StandardGameplayBga {
                    anchors.fill: parent
                    gameplay: gameplayScreen.gameplay
                }
                // [bga]
            }
        }
        // [half-screen]

        // [speed-controls]
        Label {
            Layout.fillWidth: true
            text: qsTr("START + scratch: note speed. SELECT + scratch: cover. Double-press START: toggle lane cover.")
            wrapMode: Text.Wrap
        }
        // [speed-controls]

        Label {
            Layout.fillWidth: true
            visible: flow.retryChoosing
            text: qsTr("Release START for the same random, SELECT for a new random, or both to cancel.")
            wrapMode: Text.Wrap
        }
        // [retry-buttons]
        RowLayout {
            Button {
                text: qsTr("Exit")
                enabled: !flow.retryChoosing
                onClicked: flow.exit()
            }
            Button {
                text: qsTr("Retry")
                enabled: flow.retryAvailable
                onClicked: flow.retry(false)
            }
            Button {
                text: qsTr("Same random")
                enabled: flow.retryAvailable
                onClicked: flow.retry(true)
            }
        }
        // [retry-buttons]
    }

    // [arena-gameplay]
    StandardArenaGameplayOverlay {
        parent: gameplayScreen
        gameplay: gameplayScreen.gameplay
    }
    // [arena-gameplay]

}
