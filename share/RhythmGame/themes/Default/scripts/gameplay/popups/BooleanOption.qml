import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: booleanOption

    required property string prop
    property string description: prop
    required property var src
    height: Math.max(44, row.implicitHeight + 8)
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
            Layout.fillWidth: true
            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            text: booleanOption.description
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
        }

        CheckBox {
            id: checkBox

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.preferredWidth: 238
            checked: booleanOption.src[booleanOption.prop]

            onCheckedChanged: {
                booleanOption.src[booleanOption.prop] = checked;
            }
        }
    }
}
