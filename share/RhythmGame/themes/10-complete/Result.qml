import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

Pane {
    id: screen

    // [result-contract]
    required property ResultContext result
    // [result-contract]

    // [result-input]
    StandardResultInput {
        id: resultInput
        result: screen.result
        enabled: screen.enabled
        confirmEnabled: !(screen.result.arenaActive && Rg.arenaSession.chatOpen)
    }
    // [result-input]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: screen.result.chartData.title
            font.pixelSize: 32
            wrapMode: Text.Wrap
        }

        // [score-delegate]
        Repeater {
            model: screen.result.players

            ColumnLayout {
                id: playerResult
                required property int index
                required property ResultPlayer modelData

                Layout.fillWidth: true

                ScoreLine {
                    Layout.fillWidth: true
                    playerLabel: playerResult.modelData.profile.vars.generalVars.name
                    score: playerResult.modelData.score
                }

                // [result-ranking]
                StandardRanking {
                    id: playerRanking
                    chartData: screen.result.chartData
                    profile: playerResult.modelData.profile
                    score: playerResult.modelData.score
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: playerRanking.loading ? qsTr("Loading ranking...")
                            : playerRanking.position > 0
                                ? qsTr("Rank %1 of %2").arg(playerRanking.position)
                                    .arg(playerRanking.playerCount)
                                : qsTr("No online rank")
                    }
                    Button {
                        text: qsTr("Open ranking")
                        enabled: playerRanking.available && !playerRanking.opening
                        onClicked: playerRanking.open()
                    }
                }
                // [result-ranking]
            }
        }
        // [score-delegate]

        Item {
            Layout.fillHeight: true
        }

        // [result-retry]
        RowLayout {
            Button {
                text: qsTr("Retry")
                enabled: resultInput.retryAvailable
                onClicked: resultInput.retry(false)
            }
            Button {
                text: qsTr("Same pattern")
                enabled: resultInput.retryAvailable
                onClicked: resultInput.retry(true)
            }
        }
        // [result-retry]

        // [result-button]
        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Continue")
            enabled: resultInput.acceptsInput && resultInput.confirmEnabled
            onClicked: resultInput.confirm()
        }
        // [result-button]
    }

    // [arena-result]
    StandardArenaResultOverlay {
        id: arenaOverlay
        parent: screen
        result: screen.result
    }
    // [arena-result]

}
