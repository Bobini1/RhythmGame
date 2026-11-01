import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

Pane {
    id: screen

    // [context]
    required property GameplayContext gameplay
    // [context]

    // [decide-flow]
    StandardDecideFlow {
        id: flow
        gameplay: screen.gameplay
        pointerEnabled: false
    }
    // [decide-flow]

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, parent.width - 48)
        spacing: 16

        // [title]
        Label {
            Layout.fillWidth: true
            text: screen.gameplay.course
                ? screen.gameplay.course.name : screen.gameplay.chartData.title
            font.pixelSize: 32
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        // [title]

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Start")
            onClicked: flow.start()
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Back")
            onClicked: flow.cancel()
        }
    }
}
