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
    property int selectScrollStartSkinTime: 0
    property int selectNoScrollStartSkinTime: 0
    property int selectDatabaseLoadedSkinTime: 0
    property int selectAnimationLimit: 3200
    property int barAnimationLimit: 2200
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
    }

    property Lr2SkinFrameDriver frameDriver: Lr2SkinFrameDriver {
        id: frameDriver
        host: root.host
        clock: skinClock
        gameplayFrameState: root.host ? root.host.gameplayFrameStateRef : null
        selectVisualState: root.selectContext ? root.selectContext.visualStateObject : null
        frameAnimation: skinStopwatch
        gameplayScreen: root.host ? root.host.isGameplayScreen() : false
        gameplayStartupPending: root.host
            && frameDriver.gameplayScreen
            && !!root.host.chart
            && (root.host.gameplayReadySkinTime < 0 || root.host.gameplayStartSkinTime < 0)
    }

    property alias sceneStartMs: skinClock.sceneStartMs
    property alias skinClockRef: skinClock
    property alias skinClockNowMs: skinClock.nowMs
    property alias globalSkinTime: skinClock.globalSkinTime
    property alias selectInfoStartSkinTime: skinClock.selectInfoStartSkinTime
    property alias selectLiveSkinTime: skinClock.selectLiveSkinTime
    property alias selectSourceSkinTime: skinClock.selectSourceSkinTime
    property alias selectInfoElapsed: skinClock.selectInfoElapsed
    property alias renderSkinTime: skinClock.renderSkinTime
    property alias barSkinTime: skinClock.barSkinTime
    property alias currentFps: frameDriver.currentFps
    property alias fpsSampleIntervalMs: frameDriver.fpsSampleIntervalMs
    property alias skinTimerStateRef: timerState

    property Lr2SkinTimerState timerState: Lr2SkinTimerState {
        id: timerState
        host: root.host
        clock: skinClock
        selectContext: root.selectContext
        screenKey: root.host ? root.host.effectiveScreenKey : ""
        gameplayScreen: root.host ? root.host.isGameplayScreen() : false
        resultScreen: root.host ? root.host.isResultScreen() : false
        acceptsInput: root.host ? root.host.acceptsInput : false
        startInput: root.skinModel ? (root.skinModel.startInput || 0) : 0
        globalSkinTime: root.globalSkinTime
        renderSkinTime: root.renderSkinTime
        selectSourceSkinTime: root.selectSourceSkinTime
        selectLiveSkinTime: root.selectLiveSkinTime
        selectScrollStartSkinTime: root.selectScrollStartSkinTime
        selectNoScrollStartSkinTime: root.selectNoScrollStartSkinTime
        selectDatabaseLoadedSkinTime: root.selectDatabaseLoadedSkinTime
        selectPanel: root.host ? root.host.selectPanel : 0
        selectPanelStartSkinTime: root.host ? root.host.selectPanelStartSkinTime : 0
        selectPanelClosing: root.host ? root.host.selectPanelClosing : 0
        selectPanelCloseStartSkinTime: root.host ? root.host.selectPanelCloseStartSkinTime : 0
        selectPanelHoldTime: root.host ? root.host.selectPanelHoldTime : 0
        lr2ReadmeMode: root.host ? root.host.lr2ReadmeMode : 0
        lr2ReadmeElapsed: root.host ? root.host.lr2ReadmeElapsed : 0
        lr2RankingTransitionPhase: root.host ? root.host.lr2RankingTransitionPhase : 0
        lr2RankingTransitionElapsed: root.host ? root.host.lr2RankingTransitionElapsed : 0
        resultTimer151SkinTime: root.host ? root.host.resultTimer151SkinTime : -1
        resultTimer152SkinTime: root.host ? root.host.resultTimer152SkinTime : -1
        resultGraphStartSkinTime: root.host ? root.host.resultGraphStartSkinTime : 0
        resultGraphEndSkinTime: root.host ? root.host.resultGraphEndSkinTime : 0
        gameplayTimerRevision: root.host ? root.host.gameplayTimerRevision : 0
        gameplayTimerValues: root.host ? root.host.gameplayTimerValues : ({ "0": 0 })
    }

    property Lr2SelectInfoTimerBridge selectInfoTimerBridge: Lr2SelectInfoTimerBridge {
        selectContext: root.selectContext
        clock: skinClock
        active: root.host
            && root.host.screenUpdatesActive
            && root.host.effectiveScreenKey === "select"
    }

    property FrameAnimation skinStopwatch: FrameAnimation {
        id: skinStopwatch
        running: root.host && root.host.screenUpdatesActive
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
            root.host.bumpGameplayRevision();
            root.host.refreshGameplayRuntimeActiveOptions();
        }
    }

    function clearSelectTimerFireCache() {
        root.timerState.clearSelectTimerFireCache();
    }

    // Timer fire times (ms since scene start). LR2 select panels use timers
    // 21..26 for side-drawer opening and 31..36 for closing, so synthesize
    // those without unfreezing the whole select skin clock.
    readonly property var timers: {
        if (root.host && root.host.isResultScreen()) {
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
        root.clearSelectTimerFireCache();
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
        root.clock.restartSelectInfoTimer();
    }

    function quantizedSkinClock(now) {
        return root.clock.quantize(now);
    }

    function gameplayTimerFireTime(timer) {
        timerState.revision;
        return timerState.gameplayTimerFireTime(timer);
    }

    function resultTimerFireTime(timer) {
        timerState.revision;
        return timerState.resultTimerFireTime(timer);
    }

    function selectTimerFireTime(timer, liveClock) {
        timerState.revision;
        timerState.selectInfoRevision;
        return timerState.selectTimerFireTime(timer, liveClock === true);
    }

    function selectTimerCanFire(timer) {
        timerState.revision;
        return timerState.selectTimerCanFire(timer);
    }

    function skinTimerCanFire(timer) {
        timerState.revision;
        return timerState.skinTimerCanFire(timer);
    }

    function skinTimerFireTime(timer, liveClock) {
        timerState.revision;
        timerState.selectInfoRevision;
        return timerState.skinTimerFireTime(timer, liveClock === true);
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
