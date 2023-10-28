import QtQuick
import RhythmGameQml
import QtQml.Models

PathView {
    id: pathView

    property var current: model[currentIndex]
    property string currentFolder: "/"
    // we need to keep a reference to ChartDatas, otherwise they will be garbage collected
    property var folder: []
    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    property bool movingManually: false
    property bool scrollingText: false

    function decrementViewIndex() {
        decrementCurrentIndex();
        movingTimer.restart();
        movingManually = true;
    }
    function incrementViewIndex() {
        incrementCurrentIndex();
        movingTimer.restart();
        movingManually = true;
    }
    function open(item) {
        if (item instanceof ChartData) {
            console.info("Opening chart " + item.path);
            globalRoot.openChart(item.path);
        } else {
            let folder = SongFolderFactory.open(item);
            let length = folder.length;
            let limit = Math.max(length, pathItemCount);
            for (let i = length; i < limit; i++) {
                folder.push(folder[i % length]);
            }
            pathView.folder = folder;
            pathView.currentFolder = item;
            pathView.positionViewAtIndex(0, PathView.Center);
        }
    }

    dragMargin: 200
    focus: true
    highlightMoveDuration: 100
    model: folder
    pathItemCount: 16
    preferredHighlightBegin: 0.499999999
    preferredHighlightEnd: 0.5
    snapMode: PathView.SnapToItem

    delegate: Loader {
        id: selectItemLoader

        property bool isCurrentItem: PathView.isCurrentItem
        property bool scrollingText: pathView.scrollingText

        source: modelData instanceof ChartData ? "Chart.qml" : "Folder.qml"
    }
    path: Path {
        id: path

        property int extra: 90
        property double gap: 0.90
        property int w: 190

        startX: pathView.width - 300
        startY: pathView.y - extra

        PathLine {
            x: pathView.width - 300 - path.w / 2
            y: pathView.y + pathView.height / 2
        }
        PathPercent {
            value: 0.5
        }
        PathLine {
            x: pathView.width - 300 - path.w / 2 - (path.w / (pathView.pathItemCount + path.gap)) * (1 + path.gap)
            y: pathView.y + pathView.height / 2 + ((pathView.height + path.extra * 2) / (pathView.pathItemCount + path.gap)) * (1 + path.gap)
        }
        PathPercent {
            value: 0.5 + (1 / pathView.pathItemCount)
        }
        PathLine {
            x: pathView.width - 300 - path.w - (path.w / (pathView.pathItemCount + path.gap)) * path.gap
            y: pathView.y + (pathView.height + path.extra) + ((pathView.height + path.extra * 2) / (pathView.pathItemCount + path.gap)) * path.gap
        }
    }

    Component.onCompleted: {
        open("/");
    }
    Keys.onDownPressed: {
        incrementViewIndex();
    }
    Keys.onLeftPressed: {
        let parentFolder = SongFolderFactory.parentFolder(currentFolder);
        if (parentFolder) {
            open(parentFolder);
        } else {
            sceneStack.pop();
        }
    }
    Keys.onReturnPressed: {
        open(current);
    }
    Keys.onRightPressed: {
        open(current);
    }
    Keys.onUpPressed: {
        decrementViewIndex();
    }
    onCurrentItemChanged: {
        scrollingTextTimer.restart();
        scrollingText = false;
    }

    Timer {
        id: scrollingTextTimer

        interval: 500

        onTriggered: {
            pathView.scrollingText = true;
        }
    }
    Timer {
        id: movingTimer

        interval: pathView.highlightMoveDuration

        onTriggered: {
            pathView.movingManually = false;
        }
    }
    MouseArea {
        id: mouse

        anchors.fill: parent

        onWheel: wheel => {
            if (wheel.angleDelta.y > 0)
                pathView.decrementViewIndex();
            else
                pathView.incrementViewIndex();
        }
    }
}
