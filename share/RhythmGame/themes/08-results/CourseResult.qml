import QtQuick
import QtQuick.Controls.FluentWinUI3
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

        // [course-stages]
        Repeater {
            model: screen.result.course.chartIds

            Label {
                required property int index
                required property string modelData
                readonly property ChartData chart: screen.result.charts[index]
                readonly property var tableEntry: chart ? null
                    : Rg.tables.search(modelData)[0]?.entry

                Layout.fillWidth: true
                text: (index + 1) + ". " + (chart?.title || tableEntry?.title || modelData)
                wrapMode: Text.Wrap
            }
        }
        // [course-stages]

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
