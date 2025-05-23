pragma ValueTypeBehavior: Addressable
import QtQuick 2.0
import RhythmGameQml
import "../common/helpers.js" as Helpers

Image {
    id: folder
    required property var clearStats
    readonly property bool isTable: modelData instanceof table
    readonly property bool isLevel: modelData instanceof level
    property bool scrollingText: false
    property bool isCurrentItem: false


    Image {
        id: lamp

        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.top: parent.top
        anchors.topMargin: 9
        asynchronous: true
        readonly property string clearType: {
            if (folder.clearStats) {
                for (let type of Helpers.clearTypePriorities) {
                    if (type in folder.clearStats) {
                        return type;
                    }
                }
            }
            return "NOPLAY";
        }
        source: root.iniImagesUrl + "parts.png/C_" + clearType
    }

    Loader {
        id: clearBar
        active: clearStats !== null
        asynchronous: true
        height: 8
        width: 400
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 9
        anchors.right: parent.right
        anchors.rightMargin: 37
        sourceComponent: Row {
            property int totalLength: Object.values(clearStats).reduce((acc, stat) => acc + stat, 0)
            Repeater {
                model: {
                    let arr = Helpers.clearTypePriorities.slice()
                    arr.reverse();
                    return arr;
                }
                delegate: AnimatedSprite {
                    id: clearType
                    running: true
                    height: parent.height
                    // don't divide by zero!
                    width: ((clearStats[modelData] || 0) / (totalLength || 1)) * parent.width
                    frameDuration: 100
                    frameWidth: 1
                    frameHeight: 8
                    frameCount: 2
                    source: modelData !== "NOPLAY" ? root.iniImagesUrl + "parts.png/" + modelData + "_bar" : ""
                }
            }
        }
    }

    asynchronous: true
    source: {
        let suffix = clearStats !== null ? "_loaded" : "";
        let base = root.iniImagesUrl + "folders.png/";
        if (isTable || isLevel) {
            return base + "folder_red" + suffix;
        }
        return base + "folder_green" + suffix;
    }

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: "black"
        height: parent.height
        scrolling: folder.isCurrentItem && folder.scrollingText
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
