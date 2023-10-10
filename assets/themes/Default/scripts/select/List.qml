import QtQuick
import RhythmGameQml

PathView {
    id: pathView

    property var current: model.at(currentIndex)
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
            model.model = SongFolderFactory.open(item);
        }
    }

    dragMargin: 200
    focus: true
    highlightMoveDuration: 100
    pathItemCount: 16
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    snapMode: PathView.SnapToItem

    delegate: Loader {
        id: selectItemLoader

        property bool isCurrentItem: PathView.isCurrentItem
        property bool scrollingText: pathView.scrollingText

        source: display instanceof ChartData ? "Chart.qml" : "Folder.qml"
    }
    model: CycleModel {
        minimumAmount: pathItemCount
        model: SongFolderFactory.open("/")

        onModelChanged: {
            pathView.positionViewAtIndex(0, PathView.Center);
            pathView.current = Qt.binding(function () {
                    return model.at(currentIndex);
                });
        }
    }
    path: Path {
        id: path

        property double gap: 0.88

        startX: pathView.width - 300
        startY: pathView.y - 100

        PathLine {
            x: pathView.width - 400
            y: pathView.y + pathView.height / 2
        }
        PathPercent {
            value: 0.5
        }
        PathLine {
            x: pathView.width - 300 - 100 - (200 / (pathView.pathItemCount + path.gap)) * (1 + path.gap)
            y: pathView.y + pathView.height / 2 + ((pathView.height + 200) / (pathView.pathItemCount + path.gap)) * (1 + path.gap)
        }
        PathPercent {
            value: 0.5 + (1 / pathView.pathItemCount)
        }
        PathLine {
            x: pathView.width - 300 - 200 - (200 / (pathView.pathItemCount + path.gap)) * path.gap
            y: pathView.y + (pathView.height + 100) + ((pathView.height + 200) / (pathView.pathItemCount + path.gap)) * path.gap
        }
    }

    Component.onCompleted: {
        positionViewAtIndex(0, PathView.Center);
    }
    Keys.onDownPressed: {
        incrementViewIndex();
    }
    Keys.onLeftPressed: {
        if (model.model.parentFolder) {
            open(model.model.parentFolder);
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
    onCurrentItemChanged: {
        scrollingTextTimer.restart();
        scrollingText = false;
    }

    Timer {
        id: scrollingTextTimer

        interval: 500

        onTriggered: {
            scrollingText = true;
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
