pragma ValueTypeBehavior: Addressable
import QtQuick 2.0
import RhythmGameQml

Image {
    id: folder
    property bool scrollingText: parent.scrollingText
    readonly property bool isTable: modelData instanceof table
    readonly property bool isLevel: modelData instanceof level

    asynchronous: true
    source: root.iniImagesUrl + "folders.png/" + (isTable || isLevel ? "folder_red" : "folder_green")

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: "black"
        height: parent.height
        scrolling: isCurrentItem && parent.scrollingText
        text: {
            if (folder.isLevel) {
                return pathView.historyStack[pathView.historyStack.length-1].symbol + modelData.name
            }
            if (folder.isTable) {
                return modelData.name
            }
            return modelData || "- EMPTY -";
        }
        width: parent.width * 0.7
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.positionViewAtIndex(index + 1, PathView.Center);
            Qt.callLater(() => pathView.goForward(pathView.current));
        }
    }
}
