import RhythmGameQml
import QtQuick
import "../../common/helpers.js" as Helpers

Item {
    id: imageSelection

    height: content.implicitHeight + 16
    width: ListView.view ? ListView.view.width : 414
    property real itemHeight: 72
    property real itemWidth: 82
    required property string propertyId
    required property var src
    property string dirName: propertyId
    property string label: Helpers.capitalizeFirstLetter(propertyId)
    readonly property int columnCount: Math.max(1, Math.floor((selection.width + selection.columnSpacing) / (itemWidth + selection.columnSpacing)))
    readonly property int rowCount: Math.ceil(selection.files.length / columnCount)
    readonly property int visibleRows: Math.min(2, rowCount)

    PopupEditorColors {
        id: popupColors
    }

    Column {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 8

        Text {
            id: propertyLabel

            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            text: imageSelection.label
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            width: parent.width
        }

        GridView {
            id: selection

            readonly property var files: Rg.fileQuery.getSelectableFilesForDirectory(root.rootUrl + "images/" + imageSelection.dirName + "/")
            readonly property int columnSpacing: 8

            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            cellHeight: imageSelection.itemHeight + columnSpacing
            cellWidth: Math.max(imageSelection.itemWidth, Math.floor((width + columnSpacing) / imageSelection.columnCount) - columnSpacing)
            clip: true
            height: visibleRows * cellHeight
            interactive: rowCount > visibleRows
            keyNavigationEnabled: true
            model: files
            width: parent.width

            delegate: Rectangle {
                id: imageChoice

                required property int index
                required property string modelData

                border.color: selection.currentIndex === index ? popupColors.accent : popupColors.divider
                border.width: selection.currentIndex === index ? 2 : 1
                color: selection.currentIndex === index ? popupColors.accentFill : popupColors.preview
                height: selection.cellHeight - selection.columnSpacing
                radius: 6
                width: selection.cellWidth - selection.columnSpacing

                Image {
                    anchors.fill: parent
                    anchors.margins: 6
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "../images/" + imageSelection.dirName + "/" + imageChoice.modelData
                    sourceSize.height: height
                    sourceSize.width: width
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        selection.currentIndex = imageChoice.index;
                    }
                }
            }

            currentIndex: files.indexOf(imageSelection.src[imageSelection.propertyId])
            onCurrentIndexChanged: {
                if (currentIndex >= 0) {
                    imageSelection.src[imageSelection.propertyId] = files[currentIndex];
                }
            }
        }
    }
}
