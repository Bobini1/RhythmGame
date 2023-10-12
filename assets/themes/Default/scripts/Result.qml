import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    focus: resultFocusScope.active
    height: 600
    width: 800

    Shortcut {
        enabled: resultFocusScope.active
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
            sceneStack.pop();
        }
    }
    Text {
        id: text

        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 50
        text: result.result.points
    }
}