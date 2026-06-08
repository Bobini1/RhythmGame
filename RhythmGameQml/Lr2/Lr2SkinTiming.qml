pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

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
        clock: skinClock
        frameAnimation: skinStopwatch
        gameplayFrameState: root.host ? root.host.gameplayFrameStateRef : null
        selectVisualState: root.selectContext ? root.selectContext.visualStateObject : null
        gameplayScreen: root.host ? root.host.gameplayScreenActive : false
        gameplayStartupPending: root.host
            && frameDriver.gameplayScreen
            && !!root.host.chart
            && (root.host.gameplayReadySkinTime < 0 || root.host.gameplayStartSkinTime < 0)
        onGameplayStartupTickRequested: {
            if (root.host) {
                root.host.updateGameplayStatusTimers();
                root.host.startGameplayWhenReady();
            }
        }
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
        clock: skinClock
        selectVisualMoveActive: root.selectContext ? root.selectContext.visualMoveActive : false
        selectScrollFixedPointDragging: root.selectContext ? root.selectContext.scrollFixedPointDragging : false
        selectScrollDirection: root.selectContext ? root.selectContext.scrollDirection : 0
        selectScrollUp: root.selectContext ? root.selectContext.lr2ScrollUp : 1
        selectScrollDown: root.selectContext ? root.selectContext.lr2ScrollDown : 2
        gameplayRhythmTimerSkinTime: root.host ? root.host.gameplayRhythmTimerSkinTime : -1
        selectHeldButtonTimerStarts: root.host ? root.host.selectHeldButtonTimerStarts : ({})
        screenKey: root.host ? root.host.effectiveScreenKey : ""
        gameplayScreen: root.host ? root.host.gameplayScreenActive : false
        resultScreen: root.host ? root.host.resultScreenActive : false
        acceptsInput: root.host ? root.host.acceptsInput : false
        startInput: root.skinModel ? (root.skinModel.startInput || 0) : 0
        globalSkinTime: root.host && root.host.effectiveScreenKey === "select"
            ? root.renderSkinTime
            : root.globalSkinTime
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
    }

    property Lr2SelectInfoTimerBridge selectInfoTimerBridge: Lr2SelectInfoTimerBridge {
        navigationController: root.selectContext ? root.selectContext.navigationController : null
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
                    && root.host.gameplayScreenActive
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
            root.host.refreshGameplayRuntimeActiveOptions();
        }
    }

    // Timer fire times (ms since scene start). LR2 select panels use timers
    // 21..26 for side-drawer opening and 31..36 for closing, so synthesize
    // those without unfreezing the whole select skin clock.
    readonly property var timers: {
        if (root.host && root.host.resultScreenActive) {
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

    function restartSkinClock() : var {
        if (!root.host) {
            return;
        }
        let startInput = root.skinModel.startInput || 0;
        root.selectAnimationLimit = Math.max(3200, startInput);
        root.barAnimationLimit = Math.max(2200, startInput);
        root.clock.restart(Date.now());
        root.selectScrollStartSkinTime = 0;
        root.selectNoScrollStartSkinTime = 0;
        root.selectDatabaseLoadedSkinTime = 0;
        root.timerState.clearSelectTimerFireCache();
        root.host.selectHeldButtonTimerStarts = ({});
        root.host.selectPanelClosing = 0;
        root.host.selectScratchSoundReady = false;
        root.host.updateLr2DateTimeNumbers();
        root.clock.restartSelectInfoTimer();
        if (root.host.screenUpdatesActive && root.shouldAutoAdvance) {
            root.sceneEndTimer.restart();
        } else {
            root.sceneEndTimer.stop();
        }
    }

    function selectHeldButtonTimer(timer: var) : var {
        return (timer >= 101 && timer <= 107) || (timer >= 111 && timer <= 117);
    }

    function dependOnSelectTimerClock(liveClock: var) : void {
        if (liveClock === true) {
            timerState.selectSourceSkinTime;
        } else {
            timerState.renderSkinTime;
        }
    }

    function dependOnSelectElapsedTimer(liveClock: var) : void {
        root.dependOnSelectTimerClock(liveClock);
        timerState.selectLiveSkinTime;
    }

    function dependOnSelectTimerFireInputs(timer: var, liveClock: var) : void {
        let timerId = Number(timer || 0);
        if (timerId === 0) {
            return;
        }
        if (timerId === 1) {
            timerState.acceptsInput;
            timerState.startInput;
            timerState.renderSkinTime;
            return;
        }
        if (timerId === 10) {
            timerState.selectVisualMoveActive;
            timerState.selectScrollFixedPointDragging;
            timerState.selectScrollStartSkinTime;
            return;
        }
        if (timerId === 11) {
            root.dependOnSelectTimerClock(liveClock);
            timerState.selectInfoElapsed;
            return;
        }
        if (timerId === 12) {
            timerState.selectVisualMoveActive;
            timerState.selectScrollFixedPointDragging;
            timerState.selectScrollDirection;
            timerState.selectScrollUp;
            timerState.selectScrollStartSkinTime;
            return;
        }
        if (timerId === 13) {
            timerState.selectVisualMoveActive;
            timerState.selectScrollFixedPointDragging;
            timerState.selectScrollDirection;
            timerState.selectScrollDown;
            timerState.selectScrollStartSkinTime;
            return;
        }
        if (timerId === 14) {
            timerState.acceptsInput;
            timerState.selectNoScrollStartSkinTime;
            return;
        }
        if (timerId === 15) {
            timerState.lr2ReadmeMode;
            if (timerState.lr2ReadmeMode === 1) {
                root.dependOnSelectElapsedTimer(liveClock);
                timerState.lr2ReadmeElapsed;
            }
            return;
        }
        if (timerId === 16) {
            timerState.lr2ReadmeMode;
            if (timerState.lr2ReadmeMode === 2) {
                root.dependOnSelectElapsedTimer(liveClock);
                timerState.lr2ReadmeElapsed;
            }
            return;
        }
        if (timerId === 171) {
            timerState.selectDatabaseLoadedSkinTime;
            return;
        }
        if (timerId >= 21 && timerId <= 26) {
            timerState.selectPanel;
            if (timerState.selectPanel === timerId - 20) {
                root.dependOnSelectElapsedTimer(liveClock);
                timerState.selectPanelStartSkinTime;
            }
            return;
        }
        if (timerId >= 31 && timerId <= 36) {
            timerState.selectPanelClosing;
            if (timerState.selectPanelClosing === timerId - 30) {
                root.dependOnSelectElapsedTimer(liveClock);
                timerState.selectPanelCloseStartSkinTime;
                timerState.selectPanelHoldTime;
            }
            return;
        }
        if (timerId === 175 || timerId === 176) {
            timerState.lr2RankingTransitionPhase;
            if (timerState.lr2RankingTransitionPhase === timerId) {
                root.dependOnSelectElapsedTimer(liveClock);
                timerState.lr2RankingTransitionElapsed;
            }
            return;
        }
        if (root.selectHeldButtonTimer(timerId)) {
            let starts = timerState.selectHeldButtonTimerStarts;
            if (starts && starts[timerId] !== undefined && liveClock !== true) {
                timerState.renderSkinTime;
                timerState.selectLiveSkinTime;
            }
        }
    }

    function selectTimerFireTime(timer: var, liveClock: var) : var {
        root.dependOnSelectTimerFireInputs(timer, liveClock);
        return timerState.selectTimerFireTime(timer, liveClock === true);
    }

    function skinTimerFireTime(timer: var, liveClock: var) : var {
        timerState.gameplayScreen;
        timerState.resultScreen;
        timerState.screenKey;
        if (timerState.gameplayScreen) {
            if (root.host) {
                root.host.gameplayTimerRevision;
            }
            timerState.gameplayRhythmTimerSkinTime;
        } else if (timerState.resultScreen) {
            timerState.acceptsInput;
            timerState.startInput;
            timerState.renderSkinTime;
            timerState.resultGraphStartSkinTime;
            timerState.resultGraphEndSkinTime;
            timerState.resultTimer151SkinTime;
            timerState.resultTimer152SkinTime;
        } else if (timerState.screenKey === "select") {
            root.dependOnSelectTimerFireInputs(timer, liveClock);
        }
        return timerState.skinTimerFireTime(timer, liveClock === true);
    }

    function pauseActivity() : void {
        root.sceneEndTimer.stop();
        root.gameplayStartTimer.stop();
        root.gameplayPoorBgaOptionTimer.stop();
    }
}
