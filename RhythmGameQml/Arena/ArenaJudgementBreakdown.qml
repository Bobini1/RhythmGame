pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property int totalNotes
    required property int perfect
    required property int great
    required property int good
    required property int bad
    required property int poor
    required property int emptyPoor
    property bool expanded: false
    readonly property int safeTotalNotes: Math.max(0, totalNotes)
    readonly property real wideGridThreshold: typography.scaled(210)
    readonly property real metricLabelWidth: Math.ceil(Math.max(
        metricLabelFontMetrics.advanceWidth(qsTr("PG")),
        metricLabelFontMetrics.advanceWidth(qsTr("GR")),
        metricLabelFontMetrics.advanceWidth(qsTr("GD")),
        metricLabelFontMetrics.advanceWidth(qsTr("BD")),
        metricLabelFontMetrics.advanceWidth(qsTr("PR")),
        metricLabelFontMetrics.advanceWidth(qsTr("EP"))))

    spacing: root.expanded ? 6 : 0

    ArenaTypography {
        id: typography
    }

    FontMetrics {
        id: metricLabelFontMetrics

        font.bold: true
        font.pixelSize: typography.supportingPixelSize
    }

    component MetricCell: Rectangle {
        id: metricCell

        required property color accentColor
        required property string label
        required property int value

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: metricContent.implicitHeight + 8
        Layout.preferredWidth: 1
        color: "#241b2230"
        radius: 2

        RowLayout {
            id: metricContent

            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            Text {
                Layout.minimumWidth: root.metricLabelWidth
                Layout.preferredWidth: root.metricLabelWidth
                color: metricCell.accentColor
                font.bold: true
                font.pixelSize: typography.supportingPixelSize
                text: metricCell.label
                textFormat: Text.PlainText

                Accessible.ignored: true
            }

            Text {
                Layout.minimumWidth: 0
                color: "white"
                elide: Text.ElideLeft
                font.bold: true
                font.pixelSize: typography.bodyPixelSize
                text: String(metricCell.value)
                textFormat: Text.PlainText

                Accessible.ignored: true
            }

            Item {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
        }
    }

    Rectangle {
        id: distributionBar

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: typography.scaled(4)
        Accessible.ignored: true
        clip: true
        color: "#46505d"
        radius: 2

        Row {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                color: "#7bdcff"
                height: distributionBar.height
                width: root.safeTotalNotes > 0
                    ? distributionBar.width * Math.max(0, root.perfect)
                        / root.safeTotalNotes
                    : 0
            }

            Rectangle {
                color: "#ffd75e"
                height: distributionBar.height
                width: root.safeTotalNotes > 0
                    ? distributionBar.width * Math.max(0, root.great)
                        / root.safeTotalNotes
                    : 0
            }

            Rectangle {
                color: "#ff9c54"
                height: distributionBar.height
                width: root.safeTotalNotes > 0
                    ? distributionBar.width * Math.max(0, root.good)
                        / root.safeTotalNotes
                    : 0
            }

            Rectangle {
                color: "#746dff"
                height: distributionBar.height
                width: root.safeTotalNotes > 0
                    ? distributionBar.width * Math.max(0, root.bad)
                        / root.safeTotalNotes
                    : 0
            }

            Rectangle {
                color: "#ff4d67"
                height: distributionBar.height
                width: root.safeTotalNotes > 0
                    ? distributionBar.width * Math.max(0, root.poor)
                        / root.safeTotalNotes
                    : 0
            }
        }
    }

    Loader {
        Layout.fillWidth: true
        Layout.minimumHeight: 0
        Layout.minimumWidth: 0
        active: root.expanded
        sourceComponent: judgmentGridComponent
        visible: active
    }

    Component {
        id: judgmentGridComponent

        GridLayout {
            columnSpacing: 6
            columns: width >= root.wideGridThreshold ? 3 : 2
            rowSpacing: 4

            MetricCell {
                accentColor: "#7bdcff"
                label: qsTr("PG")
                value: Math.max(0, root.perfect)
            }

            MetricCell {
                accentColor: "#ffd75e"
                label: qsTr("GR")
                value: Math.max(0, root.great)
            }

            MetricCell {
                accentColor: "#ff9c54"
                label: qsTr("GD")
                value: Math.max(0, root.good)
            }

            MetricCell {
                accentColor: "#746dff"
                label: qsTr("BD")
                value: Math.max(0, root.bad)
            }

            MetricCell {
                accentColor: "#ff4d67"
                label: qsTr("PR")
                value: Math.max(0, root.poor)
            }

            MetricCell {
                accentColor: "#b485e8"
                label: qsTr("EP")
                value: Math.max(0, root.emptyPoor)
            }
        }
    }
}
