import QtQuick

Item {
    id: sectionHeader

    property alias text: title.text
    property alias description: subtitle.text
    property bool showTopLine: true

    height: content.implicitHeight + 16
    width: ListView.view ? ListView.view.width : 414

    PopupEditorColors {
        id: popupColors
    }

    Column {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 3

        Rectangle {
            color: popupColors.divider
            height: 1
            visible: sectionHeader.showTopLine
            width: parent.width
        }

        Text {
            id: title

            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 15
            textFormat: Text.PlainText
            width: parent.width
        }

        Text {
            id: subtitle

            color: popupColors.subtleText
            elide: Text.ElideRight
            font.pixelSize: 12
            textFormat: Text.PlainText
            visible: text.length > 0
            width: parent.width
        }
    }
}
