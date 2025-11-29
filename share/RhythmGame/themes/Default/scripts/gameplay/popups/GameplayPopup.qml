import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

Popup {
    id: popup

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    height: Math.min(500, column.contentHeight + padding * 2)
    width: 520
    padding: 2
    property alias model: column.model

    function setPosition(globalPos) {
        x = Math.min(globalPos.x, contentContainer.width - width * scale);
        y = Math.min(globalPos.y, contentContainer.height - height * scale);
    }

    background: Rectangle {
        border.color: "white"
        border.width: 2
        color: "black"
        opacity: 0.9
    }

    transformOrigin: Item.TopLeft
    scale: scaledRoot.scale
    contentItem: ScrollView {
        id: scrollView
        clip: true
        padding: 6

        ListView {
            id: column

            width: popup.width
        }
    }
}