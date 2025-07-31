pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQml.Models
import "../common/helpers.js" as Helpers

PathView {
    id: pathView

    property var current: model[currentIndex]
    property var filter: null
    // we need to keep references to ChartDatas, otherwise they will be garbage collected
    property var folderContents: []
    onOpenedFolder: refresh()
    function refresh() {
        refreshScores();
        refreshFolderClearStats();
    }
    function refreshScores() {
        Rg.profileList.mainProfile.scoreDb.cancelPending();
        if (historyStack[historyStack.length - 1] === "SEARCH") {
            let md5s = [];
            for (let item of folderContents) {
                if (typeof item === "object" && "md5" in item) {
                    md5s.push(item.md5);
                }
            }
            Rg.profileList.mainProfile.scoreDb.getScoresForMd5(md5s).then((result) => {
                scores = result.scores;
            });
        } else {
            Rg.profileList.mainProfile.scoreDb.getScores(historyStack[historyStack.length - 1]).then((result) => {
                if (result instanceof tableQueryResult) {
                    let newScores = result.scores.scores;
                    for (let [key, value] of Object.entries(result.courseScores.scores)) {
                        newScores[key] = value;
                    }
                    scores = newScores;
                } else {
                    scores = result.scores;
                }
            });
        }
    }
    property var scores: {
        return {};
    }
    property var folderClearStats: []
    function refreshFolderClearStats() {
        folderClearStats = [];
        for (let folder of folderContents) {
            if (folder instanceof ChartData || folder instanceof entry || folder instanceof course) {
                continue;
            }
            Rg.profileList.mainProfile.scoreDb.getScores(folder).then((result) => {
                if (result instanceof tableQueryResult) {
                    result = result.scores;
                }
                let clearCounts = {"NOPLAY": result.unplayed};
                for (let scores of Object.values(result.scores)) {
                    let clear = Helpers.getClearType(scores);
                    if (clearCounts[clear] === undefined) {
                        clearCounts[clear] = 0;
                    }
                    clearCounts[clear] += 1;
                }
                folderClearStats.push([folder, clearCounts]);
                folderClearStats = folderClearStats.slice();
            });
        }
    }

    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    property bool movingManually: movingTimer.running
    property bool scrollingText: false
    property var sort: null

    property var historyStack: []

    function addToMinimumCount(input) {
        let length = input.length;
        if (length >= pathItemCount) {
            return;
        }
        let limit = Math.ceil(pathItemCount / length) * length
        for (let i = length; i < limit; i++) {
            input.push(input[i % length] || null);
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
        let last = historyStack.pop();
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
            } else if (folderItem instanceof course && last instanceof course) {
                return folderItem.name === last.name;
            }
            return false;
        })) || 1;
        pathView.positionViewAtIndex(idx, PathView.Center);
    }

    function goForward(item) {
        if (item instanceof ChartData) {
            console.info("Opening chart " + item.path);
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(item.path, Rg.profileList.battleProfiles.player1Profile, false, null, Rg.profileList.battleProfiles.player2Profile, false, null);
            } else {
                globalRoot.openChart(item.path, Rg.profileList.mainProfile, false, null, null, false, null);
            }
            return;
        }
        if (item instanceof course) {
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(item, Rg.profileList.battleProfiles.player1Profile, false, null, Rg.profileList.battleProfiles.player2Profile, false, null);
            } else {
                globalRoot.openCourse(item, Rg.profileList.mainProfile, false, null, null, false, null);
            }
            return;
        }
        if (item instanceof entry || item === null) {
            return;
        }
        historyStack.push(item);
        open(item);
        pathView.positionViewAtIndex(1, PathView.Center);
    }

    function open(item) {
        let folder;
        if (item instanceof table) {
            let courses = item.courses;
            folder = [...item.levels, ...courses];
        } else if (item instanceof level) {
            folder = item.loadCharts();
        } else if (typeof item === "string") {
            folder = [];
            if (item === "") {
                let tables = Rg.tables.getList();
                for (let t of tables) {
                    if (t.status === table.Loaded) {
                        folder.push(t);
                    }
                }
            }
            folder.push(...Rg.songFolderFactory.open(item));
        } else {
            return [];
        }
        let newFolderContents = [];
        for (let item of folder) {
            newFolderContents.push(item);
        }
        folder = sortFilter(folder);
        addToMinimumCount(folder);
        pathView.model = folder;
        pathView.folderContents = newFolderContents;
        openedFolder();
        return folder;
    }

    signal openedFolder()

    function search(query) {
        let results = Rg.songFolderFactory.search(query);
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        let newFolderContents = [];
        for (let item of results) {
            newFolderContents.push(item);
        }
        results = sortFilter(results);
        addToMinimumCount(results);
        // The special path for searches.
        if (historyStack[historyStack.length - 1] !== "SEARCH") {
            historyStack.push("SEARCH");
        }
        pathView.model = results;
        pathView.folderContents = newFolderContents;
        pathView.positionViewAtIndex(1, PathView.Center);
        openedFolder();
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
    cacheItemCount: 4

    delegate: Loader {
        id: selectItemLoader

        Component {
            id: chartComponent
            ChartEntry {
                property string identifier: modelData instanceof course ? modelData.identifier : modelData.md5
                scores: pathView.scores[identifier] || []
                isCurrentItem: selectItemLoader.isCurrentItem
                scrollingText: selectItemLoader.scrollingText
            }
        }

        Component {
            id: folderComponent
            FolderEntry {
                clearStats: {
                    let stats = pathView.folderClearStats.find((item) => {
                        if (item[0] instanceof table && modelData instanceof table) {
                            return item[0].url === modelData.url;
                        } else if (item[0] instanceof level && modelData instanceof level) {
                            return item[0].name === modelData.name;
                        } else if (typeof item[0] === "string" && typeof modelData === "string") {
                            return item[0] === modelData;
                        }
                        return false;
                    });
                    if (stats) {
                        return stats[1];
                    }
                    return null;
                }
                isCurrentItem: selectItemLoader.isCurrentItem
                scrollingText: selectItemLoader.scrollingText
            }
        }

        readonly property var scoreWithBestPoints: "scoreWithBestPoints" in item ? item.scoreWithBestPoints : null
        readonly property var scores: "scores" in item ? item.scores : []
        readonly property var bestStats: "bestStats" in item ? item.bestStats : null
        readonly property bool isCurrentItem: PathView.isCurrentItem
        readonly property bool scrollingText: pathView.scrollingText

        sourceComponent: modelData instanceof ChartData || modelData instanceof entry || modelData instanceof course ? chartComponent : folderComponent
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
    Timer {
        id: scrAutorepeat
        interval: 100
        running: Input.col1sUp || Input.col1sDown
        repeat: true
        Input.onCol1sUpPressed: up = true
        Input.onCol1sDownPressed: up = false
        Input.onCol1sUpReleased: up = false
        Input.onCol1sDownReleased: up = true
        property bool up
        triggeredOnStart: true
        onTriggered: {
            if (up) {
                pathView.decrementViewIndex();
            } else {
                pathView.incrementViewIndex();
            }
        }
    }
    Timer {
        id: p2scrAutorepeat
        interval: 100
        running: Input.col2sUp || Input.col2sDown
        repeat: true
        Input.onCol2sUpPressed: up = true
        Input.onCol2sDownPressed: up = false
        Input.onCol2sUpReleased: up = false
        Input.onCol2sDownReleased: up = true
        property bool up
        onTriggered: {
            if (scrAutorepeat.running) {
                return;
            }
            if (up) {
                pathView.decrementViewIndex();
            } else {
                pathView.incrementViewIndex();
            }
        }
    }
    Input.onCol11Pressed: {
        goForward(current);
    }
    Input.onCol17Pressed: {
        goForward(current);
    }
    Input.onCol13Pressed: {
        if (current instanceof ChartData) {
            globalRoot.openChart(songList.current.path, Rg.profileList.mainProfile, true, null, null, false, null);
        } else {
            goForward(current);
        }
    }
    Input.onCol15Pressed: {
        if (current instanceof ChartData && songList.currentItem.scores.length > 0) {
            let key = 0;
            if (Keys.digit2Pressed) {
                key = 1;
            } else if (Keys.digit3Pressed) {
                key = 2;
            } else if (Keys.digit4Pressed) {
                key = 3;
            }
            root.openReplay(key);
        } else {
            goForward(current);
        }
    }
    Input.onCol21Pressed: {
        goForward(current);
    }
    Input.onCol27Pressed: {
        goForward(current);
    }
    Input.onCol23Pressed: {
        if (current instanceof ChartData) {
            globalRoot.openChart(songList.current.path, Rg.profileList.mainProfile, true, null, null, false, null);
        } else {
            goForward(current);
        }
    }
    Input.onCol25Pressed: {
        if (current instanceof ChartData && songList.currentItem.scores.length > 0) {
            let key = 0;
            if (Keys.digit2Pressed) {
                key = 1;
            } else if (Keys.digit3Pressed) {
                key = 2;
            } else if (Keys.digit4Pressed) {
                key = 3;
            }
            root.openReplay(key);
        } else {
            goForward(current);
        }
    }
    Input.onButtonPressed: (key) => {
        if (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26) {
            goBack();
        }
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
