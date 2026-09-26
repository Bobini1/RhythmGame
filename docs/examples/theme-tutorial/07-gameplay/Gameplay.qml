pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Pane {
    id: screen
    required property GameplayContext gameplay

    // [flow]
    StandardGameplayFlow {
        id: flow
        gameplay: screen.gameplay
    }
    // [flow]

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // [stage]
        Label {
            Layout.fillWidth: true
            text: screen.gameplay.chartData.title
            font.pixelSize: 24
            elide: Text.ElideRight
        }
        Label {
            visible: screen.gameplay.isCourse
            text: qsTr("Stage %1 of %2").arg(screen.gameplay.stageIndex + 1)
                .arg(screen.gameplay.stageCount)
        }
        // [stage]

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // [players]
            Repeater {
                model: screen.gameplay.players
                PlayerField {
                    required property Player modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    player: modelData
                }
            }
            // [players]
        }

        Label {
            Layout.fillWidth: true
            visible: flow.retryChoosing
            text: qsTr("Release START for the same pattern, SELECT for a new pattern, or both to cancel.")
            wrapMode: Text.Wrap
        }
        Button {
            text: qsTr("Exit")
            enabled: !flow.retryChoosing
            onClicked: flow.exit()
        }
    }

    // [arena-gameplay]
    StandardArenaGameplayOverlay {
        parent: screen
        gameplay: screen.gameplay
        z: 10
    }
    // [arena-gameplay]

}
