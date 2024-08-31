import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

Popup {
    id: popup

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    height: 500
    width: 520
    padding: 2
    property alias model: column.model

    function setPosition(globalPos) {
        x = Math.min(globalPos.x, contentContainer.width - width * scale);
        y = Math.min(globalPos.y, contentContainer.height - height * scale);
    }

    background: Rectangle {
        border.color: "white"
        border.width: parent.padding
        color: "black"
        opacity: 0.9
    }

    transformOrigin: Item.TopLeft
    scale: scaledRoot.scale
    contentItem: ScrollView {
        clip: true
        leftPadding: 6

        ListView {
            id: column

            width: popup.width
        }
    }
}