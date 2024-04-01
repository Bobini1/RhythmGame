import QtQuick
import RhythmGameQml
import QtQml.Models

PathView {
    id: pathView

    property var current: model[currentIndex]
    property string currentFolder: ""
    property var filter: null
    // we need to keep references to ChartDatas, otherwise they will be garbage collected
    property var folderContents: []
    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    property bool movingManually: false
    property bool scrollingText: false
    property var sort: null

    function addToMinimumCount(input) {
        let length = input.length;
        let limit = Math.max(length, pathItemCount);
        for (let i = length; i < limit; i++) {
            input.push(input[i % length]);
        }
    }
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
            pathView.folderContents = folder;
            folder = sortFilter(folder);
            addToMinimumCount(folder);
            pathView.currentFolder = item;
            pathView.model = folder;
            pathView.positionViewAtIndex(1, PathView.Center);
        }
    }
    function search(query) {
        let results = SongFolderFactory.search(query);
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        pathView.folderContents = results;
        results = sortFilter(results);
        addToMinimumCount(results);
        pathView.currentFolder = pathView.currentFolder + "search/";
        pathView.model = results;
        pathView.positionViewAtIndex(1, PathView.Center);
    }
    function sortFilter(input) {
        let resultFolders = [];
        let resultCharts = [];
        for (let item of input) {
            if (item instanceof ChartData) {
                if (filter && !filter(item))
                    continue;
                resultCharts.push(item);
            } else {
                resultFolders.push(item);
            }
        }
        if (sort) {
            resultCharts.sort(sort);
        }
        return resultFolders.concat(resultCharts);
    }
    function sortOrFilterChanged() {
        if (folderContents.length) {
            let old = pathView.current;
            let sortedFiltered = sortFilter(folderContents);
            addToMinimumCount(sortedFiltered);
            let currentIdx = sortedFiltered.indexOf(old);
            pathView.model = sortedFiltered;
            if (currentIdx >= 0)
                pathView.positionViewAtIndex(currentIdx + 1, PathView.Center);
            else
                pathView.positionViewAtIndex(1, PathView.Center);
        }
    }

    dragMargin: 200
    highlightMoveDuration: 100
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
        open("");
    }
    Keys.onDownPressed: {
        incrementViewIndex();
    }
    Keys.onLeftPressed: {
        if (!currentFolder) {
            sceneStack.pop();
        }
        let parentFolder = SongFolderFactory.parentFolder(currentFolder);
        open(parentFolder);
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
    onFilterChanged: {
        sortOrFilterChanged();
    }
    onSortChanged: {
        sortOrFilterChanged();
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
