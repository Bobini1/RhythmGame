import QtQuick
import QtQuick.Controls.Basic

ApplicationWindow {
    id: root

    required property string installedChartPath
    required property string initializationError
    required property string buildInputSha256
    readonly property bool ready: initializationError.length === 0

    visible: true
    width: 900
    height: 520
    title: qsTr("RhythmGame web playtest")
    color: "#111318"

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 760)
        spacing: 18

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            color: root.ready ? "#d6ff63" : "#ff6b6b"
            font.pixelSize: 28
            font.bold: true
            text: root.ready
                  ? qsTr("Chart package installed")
                  : qsTr("Initialization failed")
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: "#f2f4f8"
            font.pixelSize: 17
            text: root.ready
                  ? qsTr("Selected chart: %1").arg(root.installedChartPath)
                  : root.initializationError
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: "#9299a6"
            font.pixelSize: 14
            text: root.ready
                  ? qsTr("The gameplay and audio runtime connects in the next build step.")
                  : qsTr("The chart was not made available to the application.")
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            color: "#667080"
            font.pixelSize: 11
            text: qsTr("Build input: %1").arg(root.buildInputSha256)
        }
    }
}
