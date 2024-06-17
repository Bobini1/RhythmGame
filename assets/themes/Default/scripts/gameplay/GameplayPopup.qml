import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

Popup {
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    height: 500
    width: 500
    property alias model: column.model

    background: Rectangle {
        border.color: "white"
        border.width: 2
        color: "black"
        opacity: 0.9
    }
    contentItem: ScrollView {
        anchors.fill: parent
        clip: true

        ListView {
            id: column

            width: childrenRect.width

            model: ObjectModel {
                id: objectModel

                ImageSelection {
                    id: notes

                    propertyId: "notes"
                }
            }
        }
    }
}