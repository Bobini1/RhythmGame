import RhythmGameQml
import QtQuick
import "../common/helpers.js" as Helpers

Row {
    id: imageSelection
    height: Math.max(selection.height, propertyLabel.height)
    spacing: 10
    property real itemHeight: 140
    property real itemWidth: 120
    required property string propertyId
    required property var src
    property string label: Helpers.capitalizeFirstLetter(propertyId)

    Text {
        id: propertyLabel

        anchors.verticalCenter: parent.verticalCenter
        color: "white"
        width: 110
        font.bold: true
        text: imageSelection.label
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    GridView {
        id: selection

        readonly property var files: Rg.fileQuery.getSelectableFilesForDirectory(root.rootUrl + "images/" + imageSelection.propertyId + "/")

        activeFocusOnTab: true
        cellHeight: imageSelection.itemHeight + imageSelection.spacing
        cellWidth: imageSelection.itemWidth + imageSelection.spacing
        height: cellHeight * (Math.floor((files.length - 1) / 3) + 1)
        model: files
        width: cellWidth * 3
        interactive: false
        keyNavigationEnabled: true

        delegate: Image {
            fillMode: Image.PreserveAspectFit
            height: selection.cellHeight - imageSelection.spacing
            source: "images/" + imageSelection.propertyId + "/" + modelData
            width: selection.cellWidth - imageSelection.spacing

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    selection.currentIndex = index;
                }
            }
        }
        highlight: Rectangle {
            border.color: "red"
            border.width: selection.activeFocus ? 2 : 0
            color: "lightsteelblue"
            radius: 5
        }

        currentIndex: files.indexOf(imageSelection.src[imageSelection.propertyId])
        onCurrentIndexChanged: {
            imageSelection.src[imageSelection.propertyId] = files[currentIndex];
        }
    }
}
