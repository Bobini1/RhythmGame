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

        ScrollView {
            id: resultsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: resultsScroll.availableWidth
                spacing: 24

                // [course-players]
                Repeater {
                    model: screen.result.players

                    ColumnLayout {
                        id: playerResult
                        required property int index
                        required property CourseResultPlayer modelData

                        Layout.fillWidth: true
                        ScoreLine {
                            Layout.fillWidth: true
                            playerLabel: playerResult.modelData.profile.vars.generalVars.name
                            score: playerResult.modelData.score
                        }
                        ResultStats {
                            Layout.fillWidth: true
                            score: playerResult.modelData.score
                        }
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
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Continue")
            enabled: resultInput.acceptsInput && resultInput.confirmEnabled
            onClicked: resultInput.confirm()
        }
    }
}
