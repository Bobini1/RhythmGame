import QtQuick
import RhythmGameQml

PathView {
    id: pathView

    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    property bool movingManually: false

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
            model = SongFolderFactory.open(item);
        }
    }

    dragMargin: 200
    focus: true
    highlightMoveDuration: 100
    model: SongFolderFactory.open("/")
    pathItemCount: 15

    // selected item should be in the middle of the arc
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    snapMode: PathView.SnapToItem

    delegate: Loader {
        id: selectItemLoader

        property bool isCurrentItem: PathView.isCurrentItem

        source: display instanceof ChartData ? "Chart.qml" : "Folder.qml"
    }
    path: Path {
        startX: pathView.width - 350
        startY: -100

        PathLine {
            x: pathView.width - 475
            y: pathView.height / 2
        }
        PathPercent {
            value: 0.5
        }
        PathLine {
            x: pathView.width - 350 - 140 - (280 / pathView.pathItemCount) * 1.75
            y: pathView.height / 2 + ((pathView.height + 200) / pathView.pathItemCount) * 1.75
        }
        PathPercent {
            value: 0.5 + (1 / pathView.pathItemCount)
        }
        PathLine {
            x: pathView.width - 350 - 280 - (280 / pathView.pathItemCount) * 0.75
            y: (pathView.height + 100) + ((pathView.height + 200) / pathView.pathItemCount) * 0.75
        }
    }

    Component.onCompleted: {
        positionViewAtIndex(0, PathView.Center);
    }
    Keys.onDownPressed: {
        incrementViewIndex();
    }
    Keys.onLeftPressed: {
        if (model.parentFolder) {
            model = SongFolderFactory.open(model.parentFolder);
        } else {
            sceneStack.pop();
        }
    }
    Keys.onReturnPressed: {
        open(model.at(currentIndex));
    }
    Keys.onRightPressed: {
        open(model.at(currentIndex));
    }
    Keys.onUpPressed: {
        decrementViewIndex();
    }
    onModelChanged: {
        model.minimumAmount = pathItemCount;
    }

    Timer {
        id: movingTimer

        interval: pathView.highlightMoveDuration

        onTriggered: {
            pathView.movingManually = false;
        }
    }
    Image {
        id: selector

        anchors.centerIn: parent
        anchors.verticalCenterOffset: 30
        source: iniImagesUrl + "folders.png/frame"
        z: pathView.count + 1
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
