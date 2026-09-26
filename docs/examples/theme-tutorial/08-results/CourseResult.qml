import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Pane {
    id: screen

    // [course-contract]
    required property CourseResultContext result
    // [course-contract]

    StandardResultInput {
        id: resultInput
        result: screen.result
        enabled: screen.enabled
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: screen.result.course.name
            font.pixelSize: 32
            wrapMode: Text.Wrap
        }

        // [course-players]
        Repeater {
            model: screen.result.players

            ScoreLine {
                required property int index
                required property CourseResultPlayer modelData

                Layout.fillWidth: true
                playerLabel: modelData.profile.vars.generalVars.name
                score: modelData.score
            }
        }

        // [course-players]

        Item {
            Layout.fillHeight: true
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Continue")
            enabled: resultInput.acceptsInput && resultInput.confirmEnabled
            onClicked: resultInput.confirm()
        }
    }
}
