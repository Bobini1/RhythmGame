pragma ValueTypeBehavior: Addressable
import QtQuick 2.0
import RhythmGameQml

Image {
    id: folder
    property bool scrollingText: parent.scrollingText
    readonly property bool isTable: modelData instanceof table || modelData instanceof level

    asynchronous: true
    source: root.iniImagesUrl + "folders.png/" + (isTable ? "folder_red" : "folder_green")

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: "black"
        height: parent.height
        scrolling: isCurrentItem && parent.scrollingText
        text: folder.isTable ? modelData.name : (modelData || "- EMPTY -")
        width: parent.width * 0.7
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.currentIndex = index;
            pathView.goForward(modelData);
        }
    }
}
