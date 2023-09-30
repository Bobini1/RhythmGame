import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    height: 600
    width: 800

    Text {
        id: text

        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 50
        text: result.result.points
    }
    Shortcut {
        enabled: true
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            sceneStack.pop();
        }
    }
}