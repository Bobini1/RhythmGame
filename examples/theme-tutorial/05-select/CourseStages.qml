import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

ScrollView {
    id: root

    required property var courseData

    // [course-stage-title]
    function titleFor(stage) {
        const chart = stage instanceof ChartData
            ? stage : Rg.tables.search(stage)[0]?.entry;
        if (chart?.title)
            return chart.title + (chart.subtitle ? " " + chart.subtitle : "");
        return stage instanceof ChartData ? qsTr("Untitled chart") : stage;
    }
    // [course-stage-title]

    contentWidth: availableWidth

    ColumnLayout {
        width: root.availableWidth
        spacing: 4

        Repeater {
            model: root.courseData ? root.courseData.loadCharts() : []

            delegate: Label {
                id: stageRow

                required property int index
                required property var modelData
                readonly property bool missing: !(modelData instanceof ChartData)
                readonly property string title: root.titleFor(modelData)

                Layout.fillWidth: true
                text: qsTr("%1. %2").arg(index + 1).arg(stageRow.missing
                    ? qsTr("%1 (not installed)").arg(stageRow.title)
                    : stageRow.title)
                textFormat: Text.PlainText
                color: stageRow.missing ? "#e89c8a" : palette.text
                wrapMode: Text.Wrap
            }
        }
    }
}
