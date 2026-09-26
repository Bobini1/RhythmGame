import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// [score-line]
ColumnLayout {
    id: line

    required property string playerLabel
    required property var score

    Label {
        Layout.fillWidth: true
        text: line.playerLabel
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: line.score
            ? qsTr("%1 / %2 points, %3")
                .arg(line.score.result.points)
                .arg(line.score.result.maxPoints)
                .arg(line.score.result.clearType)
            : qsTr("No result")
        wrapMode: Text.Wrap
    }
}
// [score-line]
