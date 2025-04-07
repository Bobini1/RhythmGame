pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQml.Models

PathView {
    id: pathView

    property var current: model[currentIndex]
    property var filter: null
    // we need to keep references to ChartDatas, otherwise they will be garbage collected
    property var folderContents: []
    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    property bool movingManually: movingTimer.running
    property bool scrollingText: false
    property var sort: null

    property var historyStack: []

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
    }

    function incrementViewIndex() {
        incrementCurrentIndex();
        movingTimer.restart();
    }

    function goBack() {
        if (historyStack.length === 1) {
            return;
        }
        let last = historyStack[historyStack.length - 1];
        historyStack.pop();
        let folder = open(historyStack[historyStack.length - 1]);
        let idx = (1 + folder.findIndex((folderItem) => {
            if (folderItem instanceof ChartData && last instanceof ChartData) {
                return folderItem.path === last.path;
            } else if (typeof folderItem === "string" && typeof last === "string") {
                return folderItem === last;
            } else if (folderItem instanceof level && last instanceof level) {
                return folderItem.name === last.name;
            } else if (folderItem instanceof table && last instanceof table) {
                return folderItem.name === last.name;
            }
            return false;
        })) || 1;
        pathView.positionViewAtIndex(idx, PathView.Center);
    }

    function goForward(item) {
        if (item instanceof ChartData) {
            console.info("Opening chart " + item.path);
            globalRoot.openChart(item.path);
            return;
        }
        if (item instanceof entry) {
            return;
        }
        historyStack.push(item);
        open(item);
        pathView.positionViewAtIndex(1, PathView.Center);
    }

    function open(item) {
        let folder;
        if (item instanceof table) {
            folder = item.levels;
        } else if (item instanceof level) {
            folder = item.loadCharts();
        } else if (typeof item === "string") {
            folder = [];
            if (item === "") {
                let tables = Tables.getList();
                for (let t of tables) {
                    if (t.status === table.Loaded) {
                        folder.push(t);
                    }
                }
            }
            folder.push(...SongFolderFactory.open(item));
        } else {
            return;
        }
        pathView.folderContents.length = 0;
        for (let item of folder) {
            pathView.folderContents.push(item);
        }
        folder = sortFilter(folder);
        addToMinimumCount(folder);
        pathView.model = folder;
        openedFolder();
        return folder;
    }

    signal openedFolder()

    function search(query) {
        let results = SongFolderFactory.search(query);
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        let curItem = current;
        pathView.folderContents.length = 0;
        for (let item of results) {
            pathView.folderContents.push(item);
        }
        results = sortFilter(results);
        addToMinimumCount(results);
        // The special path for searches.
        if (historyStack[historyStack.length - 1] !== "SEARCH") {
            historyStack.push("SEARCH");
        }
        pathView.model = results;
        pathView.positionViewAtIndex(1, PathView.Center);
    }

    function sortFilter(input) {
        let resultFolders = [];
        let resultCharts = [];
        for (let item of input) {
            if (item instanceof ChartData || item instanceof entry) {
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

        source: typeof modelData === "string" || modelData instanceof level || modelData instanceof table ? "Folder.qml" : "Chart.qml"
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
        goForward("");
    }
    Keys.onDownPressed: {
        incrementViewIndex();
    }
    Keys.onLeftPressed: {
        if (!historyStack[historyStack.length - 1]) {
            sceneStack.pop();
        }
        goBack();
    }
    Keys.onReturnPressed: {
        goForward(current);
    }
    Keys.onRightPressed: {
        goForward(current);
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
