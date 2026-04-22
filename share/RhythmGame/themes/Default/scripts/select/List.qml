pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQml.Models
import "../common/helpers.js" as Helpers

PathView {
    id: pathView

    property var current: model[currentIndex]
    property var filter: null
    property var folderContents: []
    onOpenedFolder: refresh()
    function refresh() {
        refreshScores();
        refreshFolderClearStats();
    }
    Component.onDestruction: {
        Rg.profileList?.mainProfile?.scoreDb?.cancelPending();
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
        let dirs = [];
        for (let item of folderContents) {
            if (item instanceof ChartData) {
                dirs.push(item.chartDirectory);
            }
        }
        previewFiles = Rg.previewFilePathFetcher.getPreviewFilePaths(dirs);
    }
    property var scores: {
        return {};
    }
    property var previewFiles: {
        return {};
    }
    property var folderClearStats: []
    function refreshFolderClearStats() {
        folderClearStats = [];
        for (let folder of folderContents) {
            if (folder instanceof ChartData || folder instanceof entry || folder instanceof course || folder === null) {
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
    readonly property bool movingManually: visualMoveActive || pendingWheelSteps !== 0
    property bool scrollingText: false
    property var sort: null
    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 70
    readonly property int lr2WheelDuration: 200
    readonly property int lr2ScrollUp: 1
    readonly property int lr2ScrollDown: 2
    property int listTopbarFixed: 0
    property int listCalculatedBarFixed: 0
    property int oldBarFixed: 0
    property int nowBarFixed: 0
    property int pendingWheelSteps: 0
    property int scrollDirection: 0
    property double barMoveStartMs: 0
    property double barMoveEndMs: 0
    readonly property bool visualMoveActive: listTopbarFixed !== nowBarFixed

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

    function wrapBarFixed(value) {
        let span = count * 1000;
        if (span <= 0) {
            return 0;
        }
        return ((value % span) + span) % span;
    }

    function lr2ChangeValueByTime(from, to, start, end, now) {
        if (end <= start || now >= end) {
            return to;
        }
        if (now <= start) {
            return from;
        }
        return Math.trunc(from + (to - from) * ((now - start) / (end - start)));
    }

    function cursorIndexForFixed(fixed) {
        let normalized = wrapBarFixed(fixed);
        let base = Math.floor(normalized / 1000);
        if (normalized % 1000 !== 0 && scrollDirection === lr2ScrollDown) {
            base += 1;
        }
        return count > 0 ? ((base % count) + count) % count : 0;
    }

    function publishBarState() {
        listCalculatedBarFixed = wrapBarFixed(listTopbarFixed);
        if (count <= 0) {
            return;
        }
        let nextIndex = cursorIndexForFixed(listCalculatedBarFixed);
        if (currentIndex !== nextIndex) {
            currentIndex = nextIndex;
            scrollingText = false;
            scrollingTextTimer.restart();
        }
    }

    function setNavigationImmediate(index) {
        let normalized = count > 0 ? ((index % count) + count) % count : 0;
        currentIndex = normalized;
        let fixed = normalized * 1000;
        listTopbarFixed = fixed;
        listCalculatedBarFixed = fixed;
        oldBarFixed = fixed;
        nowBarFixed = fixed;
        pendingWheelSteps = 0;
        scrollDirection = 0;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        scrollingText = false;
        scrollingTextTimer.restart();
    }

    function updateVisualIndex(now) {
        if (pendingWheelSteps !== 0) {
            let steps = pendingWheelSteps;
            pendingWheelSteps = 0;
            applyLr2ScrollDelta(-steps, lr2WheelDuration, now);
        }
        listTopbarFixed = lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
        publishBarState();
    }

    function applyLr2ScrollDelta(entries, durationMs, now) {
        if (count === 0 || entries === 0) {
            return;
        }
        listTopbarFixed = lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
        oldBarFixed = listTopbarFixed;
        nowBarFixed += Math.round(entries * 1000);
        scrollDirection = entries < 0 ? lr2ScrollUp : lr2ScrollDown;
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs);
        highlightMoveDuration = Math.max(1, durationMs);
        publishBarState();
    }

    function queueWheelSteps(steps) {
        pendingWheelSteps += steps;
    }

    function scrollBy(entries, durationMs) {
        if (count === 0 || entries === 0) return;
        let now = Date.now();
        let duration = durationMs !== undefined ? durationMs : lr2SpeedFirst;
        updateVisualIndex(now);
        applyLr2ScrollDelta(entries, duration, now);
    }

    function scrollByKey(entries, repeated) {
        if (count === 0 || entries === 0) return;
        let now = Date.now();
        if (repeated) {
            if (now <= barMoveEndMs - 20) return;
            scrollBy(entries, lr2SpeedNext);
            return;
        }
        if (now <= barMoveEndMs) return;
        scrollBy(entries, lr2SpeedFirst);
    }

    function decrementViewIndex(repeated) {
        scrollByKey(-1, !!repeated);
    }

    function incrementViewIndex(repeated) {
        scrollByKey(1, !!repeated);
    }

    function resetNavigation() {
        setNavigationImmediate(currentIndex);
    }

    AudioPlayer {
        id: closeFolderSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close";
    }
    function goBack() {
        if (historyStack.length === 1) {
            return;
        }
        let last = historyStack.pop();
        let folder = open(historyStack[historyStack.length - 1]);
        let idx = folder.findIndex((folderItem) => {
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
        });
        pathView.positionViewAtIndex(idx, PathView.Center);
        resetNavigation();
        closeFolderSound.stop();
        closeFolderSound.play();
    }
    AudioPlayer {
        id: openFolderSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open";
    }
    function goForward(item, skipSound = false) {
        if (item instanceof ChartData) {
            console.info("Opening chart " + item.path);
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(item.path, Rg.profileList.battleProfiles.player1Profile, false, false, null, Rg.profileList.battleProfiles.player2Profile, false, false, null);
            } else {
                globalRoot.openChart(item.path, Rg.profileList.mainProfile, false, false, null, null, false, false, null);
            }
            return;
        }
        if (item instanceof course) {
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(item, Rg.profileList.battleProfiles.player1Profile, false, false, null, Rg.profileList.battleProfiles.player2Profile, false, false, null);
            } else {
                globalRoot.openCourse(item, Rg.profileList.mainProfile, false, false, null, null, false, false, null);
            }
            return;
        }
        if (item instanceof entry || item === null) {
            return;
        }
        historyStack.push(item);
        open(item);
        if (!skipSound) {
            openFolderSound.stop();
            openFolderSound.play();
        }
        pathView.positionViewAtIndex(0, PathView.Center);
        resetNavigation();
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
        pathView.positionViewAtIndex(0, PathView.Center);
        resetNavigation();
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
        let result = resultFolders.concat(resultCharts);
        if (result.length === 0) {
            result.push(null);
        }
        return result;
    }

    function sortOrFilterChanged() {
        if (folderContents.length) {
            let old = pathView.current;
            let sortedFiltered = sortFilter(folderContents);
            addToMinimumCount(sortedFiltered);
            let currentIdx = sortedFiltered.indexOf(old);
            pathView.model = sortedFiltered;
            if (currentIdx >= 0)
                pathView.positionViewAtIndex(currentIdx, PathView.Center);
            else
                pathView.positionViewAtIndex(0, PathView.Center);
            resetNavigation();
        }
    }

    dragMargin: 200
    highlightMoveDuration: lr2SpeedFirst
    highlightRangeMode: PathView.StrictlyEnforceRange
    pathItemCount: 16
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    snapMode: PathView.SnapToItem
    cacheItemCount: 16

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
        readonly property var scoreWithBestClear: "scoreWithBestClear" in item ? item.scoreWithBestClear : null
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
        goForward("", true);
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
    // All this code is to make sure that scrolling is consistent and acts as you'd expect for different types of input.
    // When you hold down both up and down, you want the last pressed key to take precedence
    // When you release one of them, the other one should take over
    // Additionally, when both P1 and P2 are holding up or down, the song wheel should not be accelerated to 2x speed.
    property var lastKey: []
    function navigate(number, type, up, key) {
        if (lastKey[lastKey.length - 1] !== key) {
            return;
        }
        let func = up ? pathView.decrementViewIndex : pathView.incrementViewIndex;
        if (type === InputTranslator.ButtonTick) {
            if (number === 0 || number >= 10) {
                func(number > 0);
            }
        } else if (type === InputTranslator.ClassicScratchTick) {
            func(false);
        } else {
            func(!!number);
        }
    }
    Input.onCol1sDownTicked: (number, type) => navigate(number, type, false, BmsKey.Col1sDown)
    Input.onCol1sUpTicked: (number, type) => navigate(number, type, true, BmsKey.Col1sUp)
    Input.onCol2sDownTicked: (number, type) => navigate(number, type, false, BmsKey.Col2sDown)
    Input.onCol2sUpTicked: (number, type) => navigate(number, type, true, BmsKey.Col2sUp)
    Input.onCol1sDownPressed: lastKey.push(BmsKey.Col1sDown);
    Input.onCol1sUpPressed: lastKey.push(BmsKey.Col1sUp);
    Input.onCol2sDownPressed: lastKey.push(BmsKey.Col2sDown);
    Input.onCol2sUpPressed: lastKey.push(BmsKey.Col2sUp);
    Input.onCol1sDownReleased: lastKey = lastKey.filter(k => k !== BmsKey.Col1sDown)
    Input.onCol1sUpReleased: lastKey = lastKey.filter(k => k !== BmsKey.Col1sUp)
    Input.onCol2sDownReleased: lastKey = lastKey.filter(k => k !== BmsKey.Col2sDown)
    Input.onCol2sUpReleased: lastKey = lastKey.filter(k => k !== BmsKey.Col2sUp)
    Keys.onUpPressed: (event) => {
        event.accepted = true;
        if (!event.isAutoRepeat) lastKey.push(Qt.Key_Up);
        navigate(event.isAutoRepeat, null, true, Qt.Key_Up);
    }
    Keys.onDownPressed: (event) => {
        event.accepted = true;
        if (!event.isAutoRepeat) lastKey.push(Qt.Key_Down);
        navigate(event.isAutoRepeat, null, false, Qt.Key_Down);
    }
    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Up) {
            if (!event.isAutoRepeat) lastKey = lastKey.filter(k => k !== Qt.Key_Up)
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            if (!event.isAutoRepeat) lastKey = lastKey.filter(k => k !== Qt.Key_Down);
            event.accepted = true;
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
            globalRoot.openChart(songList.current.path, Rg.profileList.mainProfile, true, false, null, null, false, false, null);
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
            root.openReplay(key, Qt.LeftButton);
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
            globalRoot.openChart(songList.current.path, Rg.profileList.mainProfile, true, false, null, null, false, false, null);
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
            root.openReplay(key, Qt.LeftButton);
        } else {
            goForward(current);
        }
    }
    Input.onButtonPressed: (key) => {
        if (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26) {
            goBack();
        }
    }
    AudioPlayer {
        id: scratchSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "scratch";
    }
    onCurrentItemChanged: {
        scrollingTextTimer.restart();
        scrollingText = false;
        scratchSound.stop();
        scratchSound.play();
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
        id: movementTick

        interval: 16
        repeat: true
        running: pathView.visualMoveActive || pathView.pendingWheelSteps !== 0
        onTriggered: pathView.updateVisualIndex(Date.now())
    }
    MouseArea {
        id: mouse

        anchors.fill: parent
        property real wheelRemainder: 0

        onWheel: wheel => {
            let rawSteps = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y / 120 : wheel.pixelDelta.y / 120;
            wheelRemainder += rawSteps;
            let steps = wheelRemainder > 0 ? Math.floor(wheelRemainder) : Math.ceil(wheelRemainder);
            if (steps !== 0) {
                wheelRemainder -= steps;
                pathView.queueWheelSteps(steps);
            }
            wheel.accepted = true;
        }
    }
}
