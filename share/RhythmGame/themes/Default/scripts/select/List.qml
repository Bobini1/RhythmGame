pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

PathView {
    id: pathView

    property var current: model[currentIndex]
    property alias folderContents: selectController.folderContents
    property alias historyStack: selectController.historyStack
    property alias scores: selectController.scores
    property alias previewFiles: selectController.previewFiles
    property alias folderClearStats: selectController.folderClearStats
    property alias controller: selectController

    signal openedFolder()

    StandardSelectController {
        id: selectController

        enabled: pathView.enabled
        navigationTarget: pathView
        minimumEntryCount: pathView.pathItemCount
        autoplayAction: () => root.openSelectedAutoplay()
        replayAction: button => root.openSelectedReplay(button)
        cycleReplayTypeAction: () => root.cycleReplayType()
        cycleSortModeAction: delta => root.cycleSortMode(delta)
        openInternetRankingAction: () => root.openSelectedInternetRanking()

        onOpenedFolder: pathView.openedFolder()
    }

    model: selectController.entries

    readonly property bool movingInAnyWay: movingManually || flicking || moving || dragging
    readonly property bool movingManually: visualMoveActive || pendingWheelSteps !== 0
    property bool scrollingText: false
    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 50
    readonly property int lr2WheelBaseDuration: 120
    readonly property int lr2AnalogTicksPerScroll: 3
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
    property real wheelRemainder: 0
    property int analogScrollBuffer: 0
    property int selectScratchRepeatDirection: 0
    property double selectScratchRepeatNextMs: 0
    property int suppressedCurrentItemSoundChanges: 0
    readonly property bool visualMoveActive: listTopbarFixed !== nowBarFixed

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

    function targetIndexForFixed(fixed) {
        return count > 0 ? ((Math.round(fixed / 1000) % count) + count) % count : 0;
    }

    function animatedTopbarFixed(now) {
        return lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
    }

    function publishBarState(syncSelection) {
        listCalculatedBarFixed = wrapBarFixed(listTopbarFixed);
        if (!syncSelection || count <= 0) {
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
        stopEntryChangeSounds();
        let normalized = count > 0 ? ((index % count) + count) % count : 0;
        currentIndex = normalized;
        let fixed = normalized * 1000;
        listTopbarFixed = fixed;
        listCalculatedBarFixed = fixed;
        oldBarFixed = fixed;
        nowBarFixed = fixed;
        pendingWheelSteps = 0;
        analogScrollBuffer = 0;
        selectScratchRepeatDirection = 0;
        selectScratchRepeatNextMs = 0;
        scrollDirection = 0;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        suppressedCurrentItemSoundChanges = 0;
        scrollingText = false;
        scrollingTextTimer.restart();
    }

    function updateVisualIndex(now) {
        if (pendingWheelSteps !== 0) {
            let steps = pathViewSafeWheelSteps(pendingWheelSteps);
            pendingWheelSteps -= steps;
            let entries = -steps;
            applyLr2ScrollDelta(entries, beatorajaWheelDurationForEntries(entries, now), now);
            if (pendingWheelSteps !== 0) {
                pendingWheelStepTimer.restart();
            }
            return;
        }
        listTopbarFixed = animatedTopbarFixed(now);
        publishBarState(false);
    }

    function pathViewSafeWheelSteps(steps) {
        if (count <= 2) {
            return steps > 0 ? 1 : -1;
        }
        let maxSameDirectionSteps = Math.max(1, Math.floor((count - 1) / 2));
        return Math.max(-maxSameDirectionSteps, Math.min(maxSameDirectionSteps, steps));
    }

    function beatorajaWheelDurationForEntries(entries, now) {
        if (entries === 0) {
            return 0;
        }
        let currentFixed = animatedTopbarFixed(now);
        let remainingScroll = Math.trunc((nowBarFixed - currentFixed) / 1000);
        remainingScroll = Math.max(-2, Math.min(2, remainingScroll + entries));
        if (remainingScroll === 0) {
            return 0;
        }
        return Math.max(1, Math.trunc(lr2WheelBaseDuration / (remainingScroll * remainingScroll)));
    }

    function applyLr2ScrollDelta(entries, durationMs, now) {
        if (count === 0 || entries === 0) {
            return;
        }
        listTopbarFixed = animatedTopbarFixed(now);
        oldBarFixed = listTopbarFixed;
        nowBarFixed += Math.round(entries * 1000);
        scrollDirection = entries < 0 ? lr2ScrollUp : lr2ScrollDown;
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs);
        highlightMoveDuration = Math.max(1, durationMs);
        let nextIndex = targetIndexForFixed(nowBarFixed);
        playEntryChangeSounds(Math.abs(Math.round(entries)));
        if (currentIndex !== nextIndex) {
            suppressedCurrentItemSoundChanges += 1;
            currentIndex = nextIndex;
            scrollingText = false;
            scrollingTextTimer.restart();
        }
        publishBarState(false);
    }

    function stopEntryChangeSounds() {
        scratchSound.stop();
    }

    function playEntryChangeSounds(repeats) {
        let count = Math.max(0, Math.round(repeats));
        if (count <= 0) {
            return;
        }
        for (let i = 0; i < count; ++i) {
            scratchSound.playOverlapping();
        }
    }

    function queueWheelSteps(steps) {
        pendingWheelSteps += steps;
        pendingWheelStepTimer.restart();
    }

    function queueAnalogScratchTick(up) {
        analogScrollBuffer += up ? 1 : -1;
        let steps = Math.trunc(analogScrollBuffer / lr2AnalogTicksPerScroll);
        analogScrollBuffer = analogScrollBuffer % lr2AnalogTicksPerScroll;
        if (steps !== 0) {
            queueWheelSteps(steps);
        }
    }

    function handleWheel(wheel) {
        let delta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
        if (delta === 0) {
            return;
        }
        wheelRemainder += delta / 120.0;
        let steps = wheelRemainder > 0 ? Math.floor(wheelRemainder) : Math.ceil(wheelRemainder);
        if (steps !== 0) {
            wheelRemainder -= steps;
            queueWheelSteps(steps);
        }
        wheel.accepted = true;
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
        scrollBy(entries, repeated ? lr2SpeedNext : lr2SpeedFirst);
    }

    function resetScratchRepeat() {
        selectScratchRepeatDirection = 0;
        selectScratchRepeatNextMs = 0;
    }

    function releaseScratchRepeat(up) {
        let sameDirectionStillHeld = up
            ? (Input.col1sUp || Input.col2sUp)
            : (Input.col1sDown || Input.col2sDown);
        if (!sameDirectionStillHeld
                && selectScratchRepeatDirection === (up ? 1 : -1)) {
            resetScratchRepeat();
        }
    }

    function handleScratchRepeat(up, number) {
        let direction = up ? 1 : -1;
        let now = Date.now();
        let firstTick = number === 0 || selectScratchRepeatDirection !== direction;
        if (!firstTick && now < selectScratchRepeatNextMs) {
            return;
        }
        selectScratchRepeatDirection = direction;
        selectScratchRepeatNextMs = now + (firstTick ? lr2SpeedFirst : lr2SpeedNext);
        let func = up ? pathView.decrementViewIndex : pathView.incrementViewIndex;
        func(!firstTick);
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

        sourceComponent: selectController.isChartItem(modelData)
            || modelData instanceof course ? chartComponent : folderComponent
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

    Keys.onLeftPressed: selectController.goBack()
    Keys.onReturnPressed: selectController.goForward(current)
    Keys.onEnterPressed: selectController.goForward(current)
    Keys.onRightPressed: selectController.goForward(current)
    Keys.onUpPressed: event => selectController.handleUpPressed(event)
    Keys.onDownPressed: event => selectController.handleDownPressed(event)
    Keys.onReleased: event => selectController.handleReleased(event)
    AudioPlayer {
        id: scratchSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "scratch"
    }
    onCurrentItemChanged: {
        selectController.setFocused(currentIndex, current);
        scrollingTextTimer.restart();
        scrollingText = false;
        if (suppressedCurrentItemSoundChanges > 0) {
            suppressedCurrentItemSoundChanges -= 1;
        } else {
            playEntryChangeSounds(1);
        }
    }
    Timer {
        id: scrollingTextTimer

        interval: 500

        onTriggered: {
            pathView.scrollingText = true;
        }
    }
    Timer {
        id: pendingWheelStepTimer

        interval: 0
        repeat: false

        onTriggered: pathView.updateVisualIndex(Date.now())
    }
    Timer {
        id: movementTick

        interval: 16
        repeat: true
        running: pathView.visualMoveActive
        onTriggered: pathView.updateVisualIndex(Date.now())
    }
    MouseArea {
        id: mouse

        anchors.fill: parent
        acceptedButtons: Qt.NoButton

        onWheel: wheel => {
            pathView.handleWheel(wheel);
        }
    }
}
