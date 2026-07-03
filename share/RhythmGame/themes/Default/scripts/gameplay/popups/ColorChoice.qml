import QtQuick
import RhythmGameQml
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: colorChoice

    required property string prop
    property string description: prop
    required property var src
    height: Math.max(48, row.implicitHeight + 10)
    width: ListView.view ? ListView.view.width : 414

    PopupEditorColors {
        id: popupColors
    }

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 10

        Text {
            id: text

            Layout.fillWidth: true
            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            text: colorChoice.description
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            id: rect

            Layout.preferredHeight: 30
            Layout.preferredWidth: 172
            border {
                color: popupColors.controlBorder
                width: 1
            }
            color: colorChoice.src[colorChoice.prop]
            radius: 4

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    rect.oldColor = colorChoice.src[colorChoice.prop]
                    colorDialog.open()
                }
            }

            property color oldColor

            ColorDialog {
                id: colorDialog

                selectedColor: colorChoice.src[colorChoice.prop]

                Binding {
                    target: colorChoice.src
                    property: colorChoice.prop
                    value: colorDialog.selectedColor
                }

                onRejected: {
                    colorChoice.src[colorChoice.prop] = rect.oldColor
                }
            }
        }
    }
}
