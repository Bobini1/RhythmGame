import QtQuick
import QtQuick.Controls
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

            ScoreLine {
                required property int index
                required property ResultPlayer modelData

                Layout.fillWidth: true
                playerLabel: modelData.profile.vars.generalVars.name
                score: modelData.score
            }
        }
        // [score-delegate]

        Item {
            Layout.fillHeight: true
        }

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
        z: 10
    }
    // [arena-result]

}
