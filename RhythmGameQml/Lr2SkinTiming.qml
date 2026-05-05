pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    required property var skinModel
    required property var selectContext

    signal autoAdvanceRequested()

    property int resolutionMs: 10
    property int selectInfoStartSkinTime: 0
    property int selectScrollStartSkinTime: 0
    property int selectNoScrollStartSkinTime: 0
    property int selectDatabaseLoadedSkinTime: 0
    property int selectAnimationLimit: 3200
    property int barAnimationLimit: 2200
    property int currentFps: 0
    property real lastFpsSampleMs: 0
    property int fpsSampleIntervalMs: 250
    readonly property int selectInfoAnimationLimit: 1000
    readonly property var zeroTimers: ({ "0": 0 })
    readonly property bool shouldAutoAdvance: root.host
        && root.host.effectiveScreenKey === "decide"
        && !!root.host.chart
        && root.skinModel.sceneTime > 0

    property Lr2SkinClock clock: Lr2SkinClock {
        id: skinClock

        resolutionMs: root.resolutionMs
        screenKey: root.host ? root.host.effectiveScreenKey : ""
        selectAnimationLimit: root.selectAnimationLimit
        barAnimationLimit: root.barAnimationLimit
        selectInfoAnimationLimit: root.selectInfoAnimationLimit
        selectInfoStartSkinTime: root.selectInfoStartSkinTime
    }

    property alias sceneStartMs: skinClock.sceneStartMs
    property alias skinClockRef: skinClock
    property alias skinClockNowMs: skinClock.nowMs
    property alias globalSkinTime: skinClock.globalSkinTime
    property alias selectLiveSkinTime: skinClock.selectLiveSkinTime
    property alias selectSourceSkinTime: skinClock.selectSourceSkinTime
    property alias selectInfoElapsed: skinClock.selectInfoElapsed
    property alias renderSkinTime: skinClock.renderSkinTime
    property alias barSkinTime: skinClock.barSkinTime

    property FrameAnimation skinStopwatch: FrameAnimation {
        id: skinStopwatch
        running: root.host && root.host.screenUpdatesActive
        onTriggered: {
            if (!root.host) {
                return;
            }
            const now = Date.now();
            if (now - root.lastFpsSampleMs >= root.fpsSampleIntervalMs) {
                root.lastFpsSampleMs = now;
                root.currentFps = skinStopwatch.smoothFrameTime > 0
                    ? Math.round(1.0 / skinStopwatch.smoothFrameTime)
                    : 0;
            }
            root.clock.advanceFrame(now);
            if (root.host.isGameplayScreen()
                    && root.host.chart
                    && (root.host.gameplayReadySkinTime < 0 || root.host.gameplayStartSkinTime < 0)) {
                root.host.updateGameplayStatusTimers();
                root.host.startGameplayWhenReady();
            }
        }
    }

    property Timer dateTimeStopwatch: Timer {
        interval: 1000
        running: root.host && root.host.screenUpdatesActive
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            if (root.host) {
                root.host.updateLr2DateTimeNumbers();
            }
        }
    }

    property Timer sceneEndTimer: Timer {
        interval: Math.max(1, root.skinModel.sceneTime)
        repeat: false
        onTriggered: {
            if (root.shouldAutoAdvance) {
                root.autoAdvanceRequested();
            }
        }
    }

    property Timer gameplayStartTimer: Timer {
        interval: Math.max(1, root.skinModel.playStart || 2000)
        repeat: false
        onTriggered: {
            if (root.host
                    && root.host.gameplayStartArmed
                    && root.host.enabled
                    && root.host.isGameplayScreen()
                    && root.host.chart
                    && root.host.chartStatusIs(root.host.chart.status, ChartRunner.Ready)) {
                root.host.gameplayStartArmed = false;
                root.host.chart.start();
            }
        }
    }

    property Timer gameplayPoorBgaOptionTimer: Timer {
        interval: 1000
        repeat: false
        onTriggered: {
            if (!root.host) {
                return;
            }
            root.host.queueGameplayRevision();
            root.host.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
    }

    // Timer fire times (ms since scene start). LR2 select panels use timers
    // 21..26 for side-drawer opening and 31..36 for closing, so synthesize
    // those without unfreezing the whole select skin clock.
    readonly property var timers: {
        if (!root.host) {
            return root.zeroTimers;
        }
        if (root.host.isGameplayScreen()) {
            root.host.gameplayTimerRevision;
            let values = root.host.gameplayTimerValues || root.zeroTimers;
            let rhythmTimer = root.host.gameplayRhythmTimerSkinTime();
            if (rhythmTimer < 0) {
                return values;
            }
            let result = {};
            for (let key in values) {
                result[key] = values[key];
            }
            result[140] = rhythmTimer;
            return result;
        }
        if (root.host.isResultScreen()) {
            let resultTimers = { "0": 0 };
            if (root.host.acceptsInput) {
                resultTimers[1] = Math.min(root.renderSkinTime, root.skinModel.startInput || 0);
            }
            if (root.renderSkinTime >= root.host.resultGraphStartSkinTime) {
                resultTimers[150] = root.host.resultGraphStartSkinTime;
            }
            if (root.host.resultTimer151SkinTime >= 0) {
                resultTimers[151] = root.host.resultTimer151SkinTime;
            } else if (root.renderSkinTime >= root.host.resultGraphEndSkinTime) {
                resultTimers[151] = root.host.resultGraphEndSkinTime;
            }
            if (root.host.resultTimer152SkinTime >= 0) {
                resultTimers[152] = root.host.resultTimer152SkinTime;
            }
            return resultTimers;
        }
        return root.zeroTimers;
    }

    function updateSelectAnimationLimits() {
        let startInput = root.skinModel.startInput || 0;
        root.selectAnimationLimit = Math.max(3200, startInput);
        root.barAnimationLimit = Math.max(2200, startInput);
    }

    function restartSkinClock() {
        if (!root.host) {
            return;
        }
        root.updateSelectAnimationLimits();
        root.clock.restart(Date.now());
        root.selectScrollStartSkinTime = 0;
        root.selectNoScrollStartSkinTime = 0;
        root.selectDatabaseLoadedSkinTime = 0;
        root.host.selectHeldButtonTimerStarts = ({});
        root.host.selectPanelClosing = 0;
        root.host.selectScratchSoundReady = false;
        root.host.updateLr2DateTimeNumbers();
        root.restartSelectInfoTimer();
        if (root.host.screenUpdatesActive && root.shouldAutoAdvance) {
            root.sceneEndTimer.restart();
        } else {
            root.sceneEndTimer.stop();
        }
    }

    function restartSelectInfoTimer() {
        root.selectInfoStartSkinTime = root.selectLiveSkinTime;
    }

    function quantizedSkinClock(now) {
        return root.clock.quantize(now);
    }

    function gameplayTimerFireTime(timer) {
        if (!root.host) {
            return -1;
        }
        if (Number(timer || 0) === 140) {
            return root.host.gameplayRhythmTimerSkinTime();
        }
        root.host.gameplayTimerRevision;
        let value = root.host.gameplayTimerValues ? root.host.gameplayTimerValues[timer] : undefined;
        return value === undefined || value === null ? -1 : value;
    }

    function resultTimerFireTime(timer) {
        if (!root.host) {
            return -1;
        }
        if (timer === 0) {
            return 0;
        }
        if (timer === 1) {
            return root.host.acceptsInput ? Math.min(root.renderSkinTime, root.skinModel.startInput || 0) : -1;
        }
        if (timer === 150) {
            return root.renderSkinTime >= root.host.resultGraphStartSkinTime
                ? root.host.resultGraphStartSkinTime
                : -1;
        }
        if (timer === 151) {
            if (root.host.resultTimer151SkinTime >= 0) {
                return root.host.resultTimer151SkinTime;
            }
            return root.renderSkinTime >= root.host.resultGraphEndSkinTime
                ? root.host.resultGraphEndSkinTime
                : -1;
        }
        if (timer === 152) {
            return root.host.resultTimer152SkinTime >= 0 ? root.host.resultTimer152SkinTime : -1;
        }
        return -1;
    }

    function selectTimerBaseTime(liveClock) {
        return liveClock === true ? root.selectSourceSkinTime : root.renderSkinTime;
    }

    function selectElapsedSince(startSkinTime) {
        return Math.max(0, root.selectLiveSkinTime - startSkinTime);
    }

    function selectTimerFireTime(timer, liveClock) {
        if (!root.host) {
            return -1;
        }
        let baseTime = root.selectTimerBaseTime(liveClock);
        if (timer === 0) {
            return 0;
        }
        if (timer === 1) {
            return root.host.acceptsInput ? Math.min(root.renderSkinTime, root.skinModel.startInput || 0) : -1;
        }
        if (timer === 171) {
            return root.selectDatabaseLoadedSkinTime;
        }
        if (timer === 11) {
            return baseTime - root.selectInfoElapsed;
        }
        if (timer === 14 && root.host.acceptsInput) {
            return root.selectNoScrollStartSkinTime;
        }
        if (root.selectContext.visualMoveActive || root.selectContext.scrollFixedPointDragging) {
            if (timer === 10) {
                return root.selectScrollStartSkinTime;
            }
            if (timer === 12 && root.selectContext.scrollDirection === root.selectContext.lr2ScrollUp) {
                return root.selectScrollStartSkinTime;
            }
            if (timer === 13 && root.selectContext.scrollDirection === root.selectContext.lr2ScrollDown) {
                return root.selectScrollStartSkinTime;
            }
        }
        if (timer >= 21 && timer <= 26) {
            let panel = timer - 20;
            return root.host.selectPanel === panel
                ? baseTime - root.selectElapsedSince(root.host.selectPanelStartSkinTime)
                : -1;
        }
        if (timer >= 31 && timer <= 36) {
            let panel = timer - 30;
            let elapsed = root.selectElapsedSince(root.host.selectPanelCloseStartSkinTime);
            return root.host.selectPanelClosing === panel
                    && elapsed < root.host.selectPanelHoldTime
                ? baseTime - elapsed
                : -1;
        }
        if (timer === 15 && root.host.lr2ReadmeMode === 1) {
            return baseTime - root.host.lr2ReadmeElapsed;
        }
        if (timer === 16 && root.host.lr2ReadmeMode === 2) {
            return baseTime - root.host.lr2ReadmeElapsed;
        }
        if (root.host.lr2RankingTransitionPhase !== 0 && timer === root.host.lr2RankingTransitionPhase) {
            return baseTime - root.host.lr2RankingTransitionElapsed;
        }
        return root.host.selectHeldButtonTimerFireTime(timer, liveClock);
    }

    function selectTimerCanFire(timer) {
        if (timer === 0 || timer === 1 || timer === 10 || timer === 11
                || timer === 12 || timer === 13 || timer === 14
                || timer === 15 || timer === 16 || timer === 171
                || timer === 175 || timer === 176) {
            return true;
        }
        if (timer >= 21 && timer <= 26) {
            return true;
        }
        if (timer >= 31 && timer <= 36) {
            return true;
        }
        return root.host && root.host.isSelectHeldButtonTimer(timer);
    }

    function skinTimerCanFire(timer) {
        if (!root.host) {
            return false;
        }
        let idx = Number(timer || 0);
        if (idx === 0) {
            return true;
        }
        if (root.host.isGameplayScreen()) {
            return true;
        }
        if (root.host.isResultScreen()) {
            return idx === 1 || idx === 150 || idx === 151 || idx === 152;
        }
        if (root.host.effectiveScreenKey === "select") {
            return root.selectTimerCanFire(idx);
        }
        return false;
    }

    function skinTimerFireTime(timer, liveClock) {
        if (!root.host) {
            return -1;
        }
        let idx = Number(timer || 0);
        if (idx === 0) {
            return 0;
        }
        if (root.host.isGameplayScreen()) {
            return root.gameplayTimerFireTime(idx);
        }
        if (root.host.isResultScreen()) {
            return root.resultTimerFireTime(idx);
        }
        if (root.host.effectiveScreenKey === "select") {
            return root.selectTimerFireTime(idx, liveClock);
        }
        return -1;
    }

    function restartGameplayStartTimer() {
        root.gameplayStartTimer.restart();
    }

    function stopGameplayStartTimer() {
        root.gameplayStartTimer.stop();
    }

    function restartGameplayPoorBgaOptionTimer() {
        root.gameplayPoorBgaOptionTimer.restart();
    }

    function stopGameplayPoorBgaOptionTimer() {
        root.gameplayPoorBgaOptionTimer.stop();
    }

    function pauseActivity() {
        root.sceneEndTimer.stop();
        root.stopGameplayStartTimer();
        root.stopGameplayPoorBgaOptionTimer();
    }
}
