pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: root
    focus: true
    property string csvPath
    property string screenKey: ""
    property var chart
    property var scores: []
    property var profiles: []
    property var chartData: null
    property var chartDatas: []
    property var course: null
    property var skinSettings
    property string skinSettingsData: ""
    property var selectContextRef: null
    property bool componentReady: false
    readonly property var chartAssetData: {
        if (root.effectiveScreenKey === "select") {
            return selectContext.visualChartWrapper;
        }
        if (root.chartData) {
            return root.chartData;
        }
        let runner = root.chart;
        if (!runner) {
            return null;
        }
        if (runner.chartData) {
            return runner.chartData;
        }
        if (runner.chartDatas && runner.currentChartIndex !== undefined) {
            return runner.chartDatas[runner.currentChartIndex] || null;
        }
        return runner.chartDirectory ? runner : null;
    }
    readonly property string chartAssetStageFileSource: root.effectiveScreenKey === "select"
        ? selectContext.visualStageFileSource
        : (skinModel.usesStageFileSource
            ? Lr2SkinUtils.chartAssetUrl(root.chartAssetData,
                                         root.chartAssetData ? root.chartAssetData.stageFile : "")
            : "")
    readonly property string chartAssetBackBmpSource: root.effectiveScreenKey === "select"
        ? selectContext.visualBackBmpSource
        : (skinModel.usesBackBmpSource
            ? Lr2SkinUtils.chartAssetUrl(root.chartAssetData,
                                         root.chartAssetData ? root.chartAssetData.backBmp : "")
            : "")
    readonly property string chartAssetBannerSource: root.effectiveScreenKey === "select"
        ? selectContext.visualBannerSource
        : (skinModel.usesBannerSource
            ? Lr2SkinUtils.chartAssetUrl(root.chartAssetData,
                                         root.chartAssetData ? root.chartAssetData.banner : "")
            : "")
    readonly property var lr2SkinMetadata: root.parseLr2SkinMetadata()
    readonly property string lr2SkinFamily: root.lr2SkinMetadata.format === "beatoraja" ? "beatoraja" : "lr2"
    readonly property bool lr2SkinUsesBeatorajaSemantics: root.lr2SkinFamily === "beatoraja"
    readonly property var emptyActiveOptions: []
    readonly property var zeroTimers: ({ "0": 0 })
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property bool usedOptionFilterActive: !!skinModel
        && ((!!skinModel.usedOptions && skinModel.usedOptions.length > 0)
            || (!!skinModel.usedElementOptions && skinModel.usedElementOptions.length > 0))
    readonly property var usedOptionLookup: {
        let result = {};
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        for (let option of used) {
            result[Math.abs(option)] = true;
        }
        let elementUsed = skinModel && skinModel.usedElementOptions ? skinModel.usedElementOptions : [];
        for (let option of elementUsed) {
            result[Math.abs(option)] = true;
        }
        return result;
    }
    property int tableInfoRevision: 0
    property int gameplayRevision: 0
    property int gameplayNumberRevision1: 0
    property int gameplayNumberRevision2: 0
    property int gameplayStaticNumberRevision: 0
    property int gameplayTimerRevision: 0
    property bool gameplayRevisionRefreshPending: false
    property bool gameplayNumberRevision1Pending: false
    property bool gameplayNumberRevision2Pending: false
    property bool gameplayTimerRevisionPending: false
    property bool gameplayRuntimeRefreshPending: false
    property alias gameplayRuntimeActiveOptions: selectUpdateController.gameplayRuntimeActiveOptions
    property alias selectRuntimeActiveOptions: selectUpdateController.selectRuntimeActiveOptions
    property int gameplayReadySkinTime: -1
    property int gameplayStartSkinTime: -1
    property int gameplayGaugeUpSkinTime1: -1
    property int gameplayGaugeUpSkinTime2: -1
    property int gameplayGaugeDownSkinTime1: -1
    property int gameplayGaugeDownSkinTime2: -1
    property int gameplayJudgeSkinTime1: -1
    property int gameplayJudgeSkinTime2: -1
    property int gameplayLastJudgement1: -1
    property int gameplayLastJudgement2: -1
    property int gameplayJudgeCombo1: 0
    property int gameplayJudgeCombo2: 0
    property int gameplayJudgeRevision1: 0
    property int gameplayJudgeRevision2: 0
    property var gameplayJudgeTimingCounts1: ({ early: [0, 0, 0, 0, 0, 0], late: [0, 0, 0, 0, 0, 0] })
    property var gameplayJudgeTimingCounts2: ({ early: [0, 0, 0, 0, 0, 0], late: [0, 0, 0, 0, 0, 0] })
    property int gameplayLastJudgeTiming1: 0
    property int gameplayLastJudgeTiming2: 0
    property var gameplayJudgeLaneValues1: []
    property var gameplayJudgeLaneValues2: []
    property int gameplayJudgeNowValue1: 0
    property int gameplayJudgeNowValue2: 0
    property int gameplayLastMissSkinTime1: -1
    property int gameplayLastMissSkinTime2: -1
    readonly property bool gameplayPoorBgaVisible1: root.gameplayLastMissSkinTime1 >= 0
        && root.renderSkinTime - root.gameplayLastMissSkinTime1 < 1000
    readonly property bool gameplayPoorBgaVisible2: root.gameplayLastMissSkinTime2 >= 0
        && root.renderSkinTime - root.gameplayLastMissSkinTime2 < 1000
    readonly property int gameplayPoorBgaOption1: root.gameplayPoorBgaVisible1 ? 248 : 247
    readonly property int gameplayPoorBgaOption2: root.gameplayPoorBgaVisible2 ? 268 : 267
    property int gameplayFullComboSkinTime1: -1
    property int gameplayFullComboSkinTime2: -1
    property int gameplayPreviousGauge1: -1
    property int gameplayPreviousGauge2: -1
    property int gameplayPreviousCombo1: 0
    property int gameplayPreviousCombo2: 0
    property int gameplayScorePrintStart1: 0
    property int gameplayScorePrintStart2: 0
    property int gameplayScorePrintTarget1: 0
    property int gameplayScorePrintTarget2: 0
    property int gameplayScorePrintStartSkinTime1: 0
    property int gameplayScorePrintStartSkinTime2: 0
    property int gameplayScorePrintEndSkinTime1: 0
    property int gameplayScorePrintEndSkinTime2: 0
    property var gameplayScores1: []
    property int gameplayScoresRevision: 0
    property int gameplayScoreRequest: 0
    property var gameplayHeldButtonTimerStarts: ({})
    property var gameplayOffButtonTimerStarts: ({})
    property var gameplayPreviousPressedTimers: ({})
    property var gameplayHitTimerStarts: ({})
    property var gameplayLongNoteTimerStarts: ({})
    property bool gameplayResultOpened: false
    property bool gameplayCourseResultPending: false
    property bool gameplayShowedCourseResult: false
    property bool gameplayPlayStopped: false
    property bool gameplayNothingWasHit: true
    property bool gameplayStartArmed: false
    property alias gameplayFrameStateRef: gameplayFrameState
    readonly property var gameplayPlayer1: root.chart ? root.chart.player1 : null
    readonly property var gameplayPlayer2: root.chart ? root.chart.player2 : null
    readonly property var gameplayRhythmTimerPlayer: root.gameplayPlayer1 || root.gameplayPlayer2
    readonly property int gameplayRhythmTimerSkinTime: {
        if (root.gameplayScreenActive) {
            return gameplayFrameState.rhythmTimerSkinTime;
        }
        let player = root.gameplayRhythmTimerPlayer;
        if (!player) {
            return -1;
        }
        let beatPosition = Number(player.beatPosition || 0);
        if (beatPosition > 0) {
            let rhythm = (beatPosition - Math.floor(beatPosition)) * 1000;
            return Math.max(0, Math.round(root.renderSkinTime - rhythm));
        }
        let bpm = Math.max(1, Number(player.bpm || 0));
        let elapsedMs = Math.max(0, Number(player.elapsed || 0)) / 1000000;
        let beatMs = 60000 / bpm;
        let rhythm = beatMs > 0 ? (elapsedMs % beatMs) * 1000 / beatMs : 0;
        return Math.max(0, Math.round(root.renderSkinTime - rhythm));
    }
    readonly property bool stackScreenActive: screenState.stackActive
    readonly property bool screenUpdatesActive: screenState.updatesActive
    readonly property int lr2CurrentFps: skinTiming.currentFps
    readonly property var lr2InitialClockNow: wallClockState.initialNow
    property alias skinTimingRef: skinTiming
    property alias skinTimerStateRef: skinTiming.skinTimerStateRef
    property alias skinRuntimeRef: skinRuntime
    property alias skinValueResolverRef: valueResolver
    property alias skinSliderStateRef: skinSliderState
    property alias selectPanelControllerRef: selectPanelController
    property alias selectHoverStateRef: selectHoverState
    property alias selectSearchStateRef: selectSearchState
    property alias lr2ClockNowMs: wallClockState.nowMs
    property alias lr2ClockYear: wallClockState.year
    property alias lr2ClockMonth: wallClockState.month
    property alias lr2ClockDay: wallClockState.day
    property alias lr2ClockHour: wallClockState.hour
    property alias lr2ClockMinute: wallClockState.minute
    property alias lr2ClockSecond: wallClockState.second
    property alias lr2SceneUptimeSeconds: wallClockState.uptimeSeconds
    property alias resultOldScores1: resultState.resultOldScores1
    property alias resultOldScores2: resultState.resultOldScores2
    property alias resultOldScoresRequest: resultState.resultOldScoresRequest
    property alias resultTimer151SkinTime: resultState.resultTimer151SkinTime
    property alias resultTimer152SkinTime: resultState.resultTimer152SkinTime
    property alias resultGraphStartSkinTime: resultState.resultGraphStartSkinTime
    property alias resultGraphEndSkinTime: resultState.resultGraphEndSkinTime
    property alias resultTimingStatsCache: resultState.resultTimingStatsCache
    property alias resultJudgeTimingCountsCache: resultState.resultJudgeTimingCountsCache
    readonly property var gameplayKeyTimers: root.gameplayUsesFiveKeyTimers()
        ? [100, 101, 102, 103, 104, 105, 110, 111, 112, 113, 114, 115]
        : [100, 101, 102, 103, 104, 105, 106, 107, 110, 111, 112, 113, 114, 115, 116, 117]
    readonly property var gameplayLongNoteTimers: root.gameplayUsesFiveKeyTimers()
        ? [70, 71, 72, 73, 74, 75, 80, 81, 82, 83, 84, 85]
        : [70, 71, 72, 73, 74, 75, 76, 77, 80, 81, 82, 83, 84, 85, 86, 87]
    readonly property var gameplayHiddenFiveKeyTimers: [
        56, 57, 66, 67,
        76, 77, 86, 87,
        106, 107, 116, 117,
        126, 127, 136, 137
    ]

    function parseLr2SkinMetadata() : var {
        if (!root.skinSettingsData || root.skinSettingsData.length === 0) {
            return {};
        }
        try {
            return JSON.parse(root.skinSettingsData) || {};
        } catch (error) {
            console.warn("Failed to parse LR2 skin metadata for " + root.screenKey + ": " + error);
            return {};
        }
    }

    function normalizedClearType(clearType: var) : var {
        let value = String(clearType || "NOPLAY").toUpperCase();
        switch (value) {
        case "":
            return "NOPLAY";
        case "ASSIST":
        case "ASSISTEASY":
        case "ASSIST_EASY":
            return "AEASY";
        case "LIGHT_ASSIST":
        case "LIGHTASSISTEASY":
        case "LIGHT_ASSIST_EASY":
            return "LIGHTASSIST";
        case "CLEAR":
            return "NORMAL";
        case "EX_HARD":
            return "EXHARD";
        case "EXHARD_DAN":
        case "EX_HARD_DAN":
            return "EXHARDDAN";
        case "FULLCOMBO":
        case "FULL_COMBO":
        case "FULL COMBO":
            return "FC";
        case "NO_PLAY":
        case "NO PLAY":
            return "NOPLAY";
        case "FAILED":
        case "AEASY":
        case "LIGHTASSIST":
        case "EASY":
        case "NORMAL":
        case "HARD":
        case "EXHARD":
        case "EXHARDDAN":
        case "FC":
        case "PERFECT":
        case "MAX":
        case "NOPLAY":
            return value;
        default:
            return value;
        }
    }

    function skinClearTypeForStatus(clearType: var) : var {
        let value = root.normalizedClearType(clearType);
        if (root.lr2SkinUsesBeatorajaSemantics) {
            return value;
        }
        switch (value) {
        case "AEASY":
        case "LIGHTASSIST":
            return "FAILED";
        case "EXHARD":
        case "EXHARDDAN":
            return "HARD";
        default:
            return value;
        }
    }

    onEnabledChanged: {
        if (root.effectiveScreenKey === "decide" && enabled) {
            Qt.callLater(() => sceneStack.pop());
        }
        if (enabled) {
            root.openSelectIfNeeded();
            root.activateGameplayIfNeeded();
        } else {
            root.stopSelectAudio();
            root.stopGameplayLifecycle();
        }
    }

    onScreenUpdatesActiveChanged: {
        if (!root.componentReady) {
            return;
        }
        if (screenUpdatesActive) {
            root.updateLr2DateTimeNumbers();
            root.openSelectIfNeeded();
            root.activateGameplayIfNeeded();
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        } else {
            root.pauseScreenActivity();
        }
    }

    Repeater {
        model: root.gameplayScreenActive && root.chart ? root.gameplayKeyTimers : []

        delegate: Item {
            required property int modelData

            readonly property int timerId: modelData
            readonly property var columnState: root.gameplayColumnStateForKeyTimer(timerId)

            width: 0
            height: 0
            visible: false

            onColumnStateChanged: root.syncGameplayKeyTimerFromColumnState(timerId, columnState)
            Component.onCompleted: root.syncGameplayKeyTimerFromColumnState(timerId, columnState)

            Connections {
                target: columnState
                function onPressedChanged() : void {
                    root.syncGameplayKeyTimerFromColumnState(timerId, columnState);
                }
            }
        }
    }

    Repeater {
        model: root.gameplayScreenActive && root.chart ? root.gameplayLongNoteTimers : []

        delegate: Item {
            required property int modelData

            readonly property int timerId: modelData
            readonly property var columnState: root.gameplayColumnStateForLongNoteTimer(timerId)

            width: 0
            height: 0
            visible: false

            onColumnStateChanged: root.syncGameplayLongNoteTimerFromColumnState(timerId, columnState)
            Component.onCompleted: root.syncGameplayLongNoteTimerFromColumnState(timerId, columnState)

            Connections {
                target: columnState
                function onHoldingLongNoteChanged() : void {
                    root.syncGameplayLongNoteTimerFromColumnState(timerId, columnState);
                }
            }
        }
    }

    function openSelectIfNeeded() : var {
        if (root.effectiveScreenKey === "select" && !root.screenUpdatesActive) {
            return;
        }
        if (root.effectiveScreenKey === "select" && selectContext.historyStack.length === 0) {
            root.selectScratchSoundReady = false;
            selectContext.openRoot();
            root.selectScratchSoundReady = true;
        } else if (root.effectiveScreenKey === "select" && !root.selectScratchSoundReady) {
            root.selectScratchSoundReady = true;
        }
        if (root.effectiveScreenKey === "select"
                && root.acceptsInput
                && root.heldOptionPanel > 0
                && !root.startHoldSuppressed) {
            root.holdSelectPanel(root.heldOptionPanel);
        }
        root.forceActiveFocus();
    }

    Shortcut {
        enabled: root.screenUpdatesActive && !selectSearchState.focused
        sequence: "Esc"

        onActivated: {
            if (root.effectiveScreenKey === "select" && root.lr2ReadmeMode > 0) {
                root.closeReadme();
                return;
            }
            if (root.effectiveScreenKey === "select" && root.selectPanel > 0) {
                root.closeSelectPanel();
                return;
            }
            if (root.gameplayScreenActive && root.handleGameplayEscape()) {
                return;
            }
            sceneStack.pop();
        }
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "F1"
        onActivated: root.toggleMainHelp()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "F2"
        onActivated: root.toggleSelectedReadme()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select"
        sequence: "F4"
        onActivated: globalRoot.toggleFullScreen()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "F5"
        onActivated: root.openLr2InternetRanking()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "F8"
        onActivated: root.reloadCurrentSelectFolder()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "3"
        onActivated: root.triggerSelectPanelButton(308, 1)
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !selectSearchState.focused
        sequence: "5"
        onActivated: root.toggleSelectPanel(3)
    }

    StackView.onActivated: {
        root.activateGameplayIfNeeded();
    }
    
    readonly property string effectiveScreenKey: screenState.effectiveKey
    readonly property bool gameplayScreenActive: screenState.gameplayScreen
    readonly property bool resultScreenActive: screenState.resultScreen
    function playerIsAutoPlayer(player: var) : var {
        return !!player && player instanceof AutoPlayer;
    }

    function gameplayAutoplayActive() : var {
        return root.gameplayScreenActive
            && !!root.chart
            && root.playerIsAutoPlayer(root.chart.player1)
            && (!root.chart.player2 || root.playerIsAutoPlayer(root.chart.player2));
    }

    function gameplayReplayActive() : var {
        if (!root.gameplayScreenActive || !root.chart) {
            return false;
        }
        let p1 = root.gameplayPlayer(1);
        let p2 = root.gameplayPlayer(2);
        return !!((p1 && p1.replayedScore) || (p2 && p2.replayedScore));
    }

    function optionLookupFor(options: var) : var {
        if (!options) {
            return {};
        }
        if (options.__lookup) {
            return options.__lookup;
        }
        let lookup = {};
        for (let i = 0; i < options.length; ++i) {
            lookup[options[i]] = true;
        }
        options.__lookup = lookup;
        return lookup;
    }

    function finalizeOptionList(options: var) : var {
        root.optionLookupFor(options);
        return options;
    }

    function appendParseOption(options: var, option: var) : void {
        if (option === undefined || option === null) {
            return;
        }
        let optionNumber = Number(option);
        if (!isFinite(optionNumber)) {
            return;
        }
        let lookup = root.optionLookupFor(options);
        if (lookup[optionNumber] === true) {
            return;
        }
        options.push(optionNumber);
        lookup[optionNumber] = true;
    }

    function resultTargetRankParseOption(side: var, baseOption: var) : var {
        let targetScore = root.resultTargetSavedScore(side);
        let targetResult = targetScore && targetScore.result ? targetScore.result : null;
        if (targetResult) {
            return root.resultRankOptionForResult(targetResult, baseOption);
        }
        let totalNotes = root.resultTotalNotes(root.resultData(side));
        let maxPoints = Math.max(0, totalNotes || 0) * 2;
        let points = Math.max(0, root.resultTargetPoints(side) || 0);
        if (points <= 0 || maxPoints <= 0) {
            return baseOption + 8;
        }
        let rank = Math.floor(points * 9 / maxPoints);
        if (rank >= 8) {
            return baseOption;
        }
        if (rank <= 1) {
            return baseOption + 7;
        }
        return baseOption + (8 - rank);
    }

    function appendResultParseOptions(options: var) : void {
        let current1 = root.resultData(1);
        let current2 = root.resultData(2);

        root.appendParseOption(options, root.resultClearOption());
        root.appendParseOption(options, 350);
        root.appendParseOption(options, root.resultRankOptionForResult(current1, 300));
        root.appendParseOption(options, current2
            ? root.resultRankOptionForResult(current2, 310)
            : root.resultTargetRankParseOption(1, 310));
        root.appendParseOption(options, root.resultRankOptionForResult(root.resultOldBestResult(1), 320));
        root.appendParseOption(options, root.resultRankOptionForResult(current1, 340));

        if (root.resultScoreImproved(1)) {
            root.appendParseOption(options, 330);
            if (root.resultRawRank(current1) > root.resultRawRank(root.resultOldBestResult(1))) {
                root.appendParseOption(options, 335);
            }
        } else {
            root.appendParseOption(options, 1330);
            if (root.resultRawRank(current1) === root.resultRawRank(root.resultOldBestResult(1))) {
                root.appendParseOption(options, 1335);
            }
        }
        root.appendParseOption(options, root.resultComboImproved(1) ? 331 : 1331);
        root.appendParseOption(options, root.resultBadPoorImproved(1) ? 332 : 1332);

        let targetDiff = root.resultExScore(current1) - root.resultTargetPoints(1);
        if (targetDiff > 0) {
            root.appendParseOption(options, 336);
        } else if (targetDiff === 0) {
            root.appendParseOption(options, 1336);
        }

        if (current1 && current2) {
            let diff = root.resultExScore(current1) - root.resultExScore(current2);
            root.appendParseOption(options, diff > 0 ? 352 : (diff < 0 ? 353 : 354));
        }
    }

    readonly property var parseActiveOptions: {
        let autoplayOn = root.gameplayAutoplayActive();
        let options = [
            0,  // DEFAULT is always true for #IF.
            20, // no select side panel active.
            30, // stock LR2 parse-time default: BGA normal.
            autoplayOn ? 33 : 32,
            34, // stock LR2 parse-time default: ghost off.
            39, // stock LR2 parse-time default: score graph on.
            41, // stock LR2 parse-time default: BGA on.
            46, // difficulty filter enabled.
            52, // non-extra mode.
            54, // 1P autoscratch/assist off.
            56, // 2P autoscratch/assist off.
            61, // score save enabled.
            80, // ready.
            82, // no replay playback.
            572, // not editing a custom course.
            610, // no IR ranking rows loaded by default.
            620, // internet ranking panel closed.
            622, // ghost battle off.
            624  // no rival score.
        ];
        if (root.effectiveScreenKey === "select") {
            options.push(900, 905, 910, 915); // stock LR2 select defaults
        } else if (root.effectiveScreenKey === "decide") {
            options.push(900); // stock LR2 decide default: show stagefile
            options.push(33); // LR2 decide reports both autoplay states true.
        } else if (root.resultScreenActive) {
            root.appendResultParseOptions(options);
        }
        return root.finalizeOptionList(options);
    }
    property alias selectPanel: selectPanelController.selectPanel
    property alias selectPanelHeldByStart: selectPanelController.selectPanelHeldByStart
    property alias selectPanelStartSkinTime: selectPanelController.selectPanelStartSkinTime
    property alias selectPanelClosing: selectPanelController.selectPanelClosing
    property alias selectPanelCloseStartSkinTime: selectPanelController.selectPanelCloseStartSkinTime
    readonly property int selectPanelHoldTime: selectPanelController.selectPanelHoldTime
    readonly property int selectPanelElapsed: selectPanelController.selectPanelElapsed
    readonly property int selectPanelCloseElapsed: selectPanelController.selectPanelCloseElapsed
    readonly property bool selectPanelCloseFinished: selectPanelController.selectPanelCloseFinished
    readonly property int selectHeldButtonSkinTime: selectPanelController.selectHeldButtonSkinTime
    property alias selectHeldButtonTimerStarts: selectPanelController.selectHeldButtonTimerStarts
    readonly property bool hasSelectHeldButtonTimers: selectPanelController.hasSelectHeldButtonTimers
    readonly property var selectHeldButtonTimerFireTimes: selectPanelController.selectHeldButtonTimerFireTimes
    readonly property int selectTargetScratchInitialRepeatMillis: selectPanelController.selectTargetScratchInitialRepeatMillis
    readonly property int selectTargetScratchRepeatMillis: selectPanelController.selectTargetScratchRepeatMillis
    property alias selectTargetScratchDirection: selectPanelController.selectTargetScratchDirection
    property alias selectTargetScratchNextMs: selectPanelController.selectTargetScratchNextMs
    readonly property bool anyStartHeld: selectPanelController.anyStartHeld
    readonly property bool anySelectHeld: selectPanelController.anySelectHeld
    readonly property int heldOptionPanel: selectPanelController.heldOptionPanel
    property alias startHoldSuppressed: selectPanelController.startHoldSuppressed

    Lr2OptionState {
        id: optionState
        host: root
    }

    readonly property var mainGeneralVarsRef: optionState.mainGeneralVarsValue
    readonly property var lr2GaugeLabels: optionState.lr2GaugeLabels
    readonly property var lr2GaugeValues: optionState.lr2GaugeValues
    readonly property var lr2ClassicGaugeLabels: optionState.lr2ClassicGaugeLabels
    readonly property var lr2RandomLabels: optionState.lr2RandomLabels
    readonly property var lr2RandomValues: optionState.lr2RandomValues
    readonly property var lr2RandomSupportedIndexes: optionState.lr2RandomSupportedIndexes
    readonly property var lr2HiSpeedFixLabels: optionState.lr2HiSpeedFixLabels
    readonly property var lr2HiSpeedFixValues: optionState.lr2HiSpeedFixValues
    readonly property var lr2DpOptionLabels: optionState.lr2DpOptionLabels
    readonly property var lr2DpOptionSupportedIndexes: optionState.lr2DpOptionSupportedIndexes
    readonly property var lr2GaugeAutoShiftLabels: optionState.lr2GaugeAutoShiftLabels
    readonly property var lr2GaugeAutoShiftSupportedIndexes: optionState.lr2GaugeAutoShiftSupportedIndexes
    readonly property var lr2BottomShiftableGaugeLabels: optionState.lr2BottomShiftableGaugeLabels
    readonly property var lr2BottomShiftableGaugeValues: optionState.lr2BottomShiftableGaugeValues
    readonly property var lr2LnModeLabels: optionState.lr2LnModeLabels
    readonly property var lr2JudgeAlgorithmLabels: optionState.lr2JudgeAlgorithmLabels
    readonly property var lr2BattleLabels: optionState.lr2BattleLabels
    readonly property var lr2TargetLabels: optionState.lr2TargetLabels
    readonly property var lr2TargetValues: optionState.lr2TargetValues
    readonly property var lr2ClassicTargetLabels: optionState.lr2ClassicTargetLabels
    readonly property var lr2BeatorajaTargetLabels: optionState.lr2BeatorajaTargetLabels
    readonly property var lr2BeatorajaTargetFractions: optionState.lr2BeatorajaTargetFractions
    readonly property var lr2BgaLabels: optionState.lr2BgaLabels
    readonly property var lr2BgaSizeLabels: optionState.lr2BgaSizeLabels
    readonly property var lr2GhostLabels: optionState.lr2GhostLabels
    readonly property var lr2HidSudLabels: optionState.lr2HidSudLabels
    readonly property var lr2ReplayLabels: optionState.lr2ReplayLabels
    readonly property string lr2SearchPlaceholderText: optionState.lr2SearchPlaceholderText
    property alias lr2ReplayType: optionState.lr2ReplayType
    property alias lr2SliderValues: optionState.lr2SliderValues
    property alias lr2EqOn: optionState.lr2EqOn
    property alias lr2PitchOn: optionState.lr2PitchOn
    property alias lr2PitchType: optionState.lr2PitchType
    property alias lr2FxType: optionState.lr2FxType
    property alias lr2FxOn: optionState.lr2FxOn
    property alias lr2FxTarget: optionState.lr2FxTarget
    property alias lr2ReadmeText: readmeState.text
    readonly property var lr2ReadmeLines: readmeState.lines
    property alias lr2ReadmeMode: readmeState.mode
    property alias lr2ReadmeStartSkinTime: readmeState.startSkinTime
    readonly property int lr2ReadmeElapsed: readmeState.elapsed
    property alias lr2ReadmeOffsetX: readmeState.offsetX
    property alias lr2ReadmeOffsetY: readmeState.offsetY
    property alias lr2ReadmeLineSpacing: readmeState.lineSpacing
    property alias lr2ReadmeMouseHeld: readmeState.mouseHeld
    property alias lr2ReadmeMouseX: readmeState.mouseX
    property alias lr2ReadmeMouseY: readmeState.mouseY
    Lr2ReadmeState {
        id: readmeState
        host: root
        updatesActive: root.screenUpdatesActive && root.effectiveScreenKey === "select"
            && root.lr2ReadmeMode > 0
        skinHeight: root.skinH
        skinTime: root.lr2ReadmeMode > 0 ? root.selectLiveSkinTime : 0
    }
    property alias lr2SkinPreviewScreenKey: skinSettingsState.previewScreenKey
    property alias lr2SkinCustomOffset: skinSettingsState.customOffset
    property alias lr2SkinSettingMetadata: skinSettingsState.metadata
    property alias lr2SkinSettingItems: skinSettingsState.items
    property alias suppressNextSkinClockRestart: skinSettingsState.suppressNextClockRestart

    Lr2SkinSettingsState {
        id: skinSettingsState
        host: root
    }

    Lr2ResultState {
        id: resultState
        host: root
        selectContext: selectContext
    }

    readonly property bool selectHoverTracking: root.screenUpdatesActive
        && root.effectiveScreenKey === "select"
        && !!skinModel
        && skinModel.hasMouseHover
    Lr2SelectHoverState {
        id: selectHoverState
        host: root
        skinRuntime: root.skinRuntimeRef
        runtimeElementDescriptors: root.skinRuntimeRef ? root.skinRuntimeRef.elementDescriptors : []
        tracking: root.selectHoverTracking
        skinScale: root.skinScale
    }
    property alias selectScratchSoundReady: selectSideEffects.scratchSoundReady
    readonly property bool selectAudioActive: root.screenUpdatesActive
        && root.effectiveScreenKey === "select"

    onSelectAudioActiveChanged: {
        if (!root.selectAudioActive) {
            root.stopSelectAudio();
        }
    }

    function copyObject(object: var) : var {
        let result = {};
        if (!object) {
            return result;
        }
        let keys = object.keys ? object.keys() : Object.keys(object);
        for (let key of keys) {
            result[key] = object[key];
        }
        return result;
    }

    function localizedName(value: var) : var { return skinSettingsState.localizedName(value); }
    function lr2SkinTypeScreenKey(type: var) : var { return skinSettingsState.skinTypeScreenKey(type); }
    function profileRoot() : var { return skinSettingsState.profileRoot(); }
    function lr2ConfiguredThemeName(screen: var) : var { return skinSettingsState.configuredThemeName(screen); }
    function lr2ThemeFamilyForScreen(screen: var) : var { return skinSettingsState.themeFamilyForScreen(screen); }
    function lr2ScreenObject(screen: var) : var { return skinSettingsState.screenObject(screen); }
    function lr2AvailableThemeNamesForScreen(screen: var) : var { return skinSettingsState.availableThemeNamesForScreen(screen); }
    function lr2SettingDestinationForScreen(screen: var) : var { return skinSettingsState.settingDestinationForScreen(screen); }
    function currentLr2SkinPreviewScreen() : var { return skinSettingsState.currentPreviewScreen(); }
    function defaultLr2SkinPreviewScreen() : var { return skinSettingsState.defaultPreviewScreen(); }
    function setLr2SkinPreviewScreen(screen: var) : var { return skinSettingsState.setPreviewScreen(screen); }
    function lr2SkinPreviewTitle() : var { return skinSettingsState.previewTitle(); }
    function lr2SkinPreviewMaker() : var { return skinSettingsState.previewMaker(); }
    function normalizeLr2SkinSetting(item: var, family: var) : var { return skinSettingsState.normalizeSetting(item, family); }
    function buildLr2SkinSettingItems() : var { return skinSettingsState.buildItems(); }
    function refreshLr2SkinSettingItems() : void { skinSettingsState.refreshItems(); }
    function lr2SkinCustomMaxOffset() : var { return skinSettingsState.maxOffset(); }
    function lr2SkinCustomPosition() : var { return skinSettingsState.position(); }
    function setLr2SkinCustomPosition(position: var) : var { return skinSettingsState.setPosition(position); }
    function lr2SkinSettingAtVisibleRow(row: var) : var { return skinSettingsState.settingAtVisibleRow(row); }
    function lr2SkinSettingCurrentValue(item: var) : var { return skinSettingsState.currentValue(item); }
    function lr2SkinSettingName(row: var) : var { return skinSettingsState.settingName(row); }
    function lr2SkinSettingValueText(row: var) : var { return skinSettingsState.settingValueText(row); }
    function changeLr2SkinSetting(row: var, delta: var) : var { return skinSettingsState.changeSetting(row, delta); }
    function changeLr2Soundset(delta: var) : var { return skinSettingsState.changeSoundset(delta); }
    function changeLr2SelectedTheme(delta: var) : var { return skinSettingsState.changeSelectedTheme(delta); }
    function queueSkinClockRestartAfterLoad() : void { skinSettingsState.queueSkinClockRestartAfterLoad(); }

    function setArrayValue(array: var, index: var, value: var) : var { return optionState.setArrayValue(array, index, value); }
    function sliderInitialValue(type: var) : var { return optionState.sliderInitialValue(type); }
    function sliderRawValue(type: var) : var { return optionState.sliderRawValue(type); }
    function setSliderRawValue(type: var, value: var) : void { optionState.setSliderRawValue(type, value); }
    function lr2SliderNumber(num: var) : var { return optionState.lr2SliderNumber(num); }

    function openReadmeText(text: var) : void {
        readmeState.openText(text);
    }

    function openReadmePath(path: var) : var {
        return readmeState.openPath(path);
    }

    function toggleSelectedReadme() : var {
        if (root.lr2ReadmeMode > 0) {
            root.closeReadme();
            return true;
        }
        if (root.effectiveScreenKey !== "select") {
            return false;
        }
        selectContext.navigationController.refreshFocusedState();
        return root.openReadmePath(
            selectContext.attachedTextFile(selectContext.selectedStateChartData));
    }

    function openHelpFile(index: var) : var {
        let helpFiles = skinModel.helpFiles || [];
        if (index < 0 || index >= helpFiles.length) {
            return false;
        }
        return root.openReadmePath(helpFiles[index]);
    }

    function toggleMainHelp() : var {
        if (root.lr2ReadmeMode > 0) {
            root.closeReadme();
            return true;
        }
        if (root.effectiveScreenKey !== "select") {
            return false;
        }
        return root.openHelpFile(0);
    }

    function closeReadme() : void {
        readmeState.close();
    }

    function readmeLinesForSource(src: var) : var {
        return readmeState.linesForSource(src);
    }

    function readmeContentHeight() : var {
        return readmeState.contentHeight();
    }

    function clampReadmeOffsets() : void {
        readmeState.clampOffsets();
    }

    function scrollReadmeBy(dx: var, dy: var) : var {
        return readmeState.scrollBy(dx, dy);
    }

    function hideReadmeImmediately() : void {
        readmeState.hideImmediately();
    }

    function profileForSide(side) { return optionState.profileForSide(side); }
    function generalVarsForSide(side) { return optionState.generalVarsForSide(side); }
    function indexOfValue(values: var, value: var) : var { return optionState.indexOfValue(values, value); }
    function cycleArrayIndex(index: var, count: var, delta: var) : var { return optionState.cycleArrayIndex(index, count, delta); }
    function wrappedListValue(values: var, index: var) : var { return optionState.wrappedListValue(values, index); }
    function arrayContains(values: var, value: var) : var { return optionState.arrayContains(values, value); }
    function cycleSupportedIndex(current: var, delta: var, supportedIndexes: var, count: var) : var { return optionState.cycleSupportedIndex(current, delta, supportedIndexes, count); }

    readonly property int lr2GaugeIndexP1: optionState.lr2GaugeIndexP1
    readonly property int lr2GaugeIndexP2: optionState.lr2GaugeIndexP2
    function setGaugeIndex(side: var, index: var) : void { optionState.setGaugeIndex(side, index); }
    function selectOptionSourceCount(buttonId: var, sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        return count > 1 ? count : selectPanelController.observedSelectButtonSourceCount(buttonId);
    }
    function lr2GaugeButtonId(side: var) : var {
        return side === 2 ? 41 : 40;
    }
    function lr2RandomButtonId(side: var) : var {
        return side === 2 ? 43 : 42;
    }
    function lr2GaugeButtonFrame(side: var, sourceCount: var) : var {
        let count = root.selectOptionSourceCount(root.lr2GaugeButtonId(side), sourceCount);
        return optionState.lr2GaugeButtonFrame(side, count);
    }
    function setGaugeButtonIndex(side: var, index: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(root.lr2GaugeButtonId(side), sourceCount);
        optionState.setGaugeButtonIndex(side, index, count);
    }
    function adjustGaugeButtonIndex(side: var, delta: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(root.lr2GaugeButtonId(side), sourceCount);
        optionState.adjustGaugeButtonIndex(side, delta, count);
    }
    function lr2GaugeText(side: var, sourceCount: var) : var {
        let count = root.selectOptionSourceCount(root.lr2GaugeButtonId(side), sourceCount);
        return optionState.lr2GaugeText(side, count);
    }

    readonly property int lr2RandomIndexP1: optionState.lr2RandomIndexP1
    readonly property int lr2RandomIndexP2: optionState.lr2RandomIndexP2
    function setRandomIndex(side: var, index: var) : void { optionState.setRandomIndex(side, index); }
    function lr2RandomButtonFrame(side: var, sourceCount: var) : var {
        let count = root.selectOptionSourceCount(root.lr2RandomButtonId(side), sourceCount);
        return optionState.lr2RandomButtonFrame(side, count);
    }
    function setRandomButtonIndex(side: var, index: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(root.lr2RandomButtonId(side), sourceCount);
        optionState.setRandomButtonIndex(side, index, count);
    }
    function adjustRandomButtonIndex(side: var, delta: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(root.lr2RandomButtonId(side), sourceCount);
        optionState.adjustRandomButtonIndex(side, delta, count);
    }
    function lr2RandomText(side: var, sourceCount: var) : var {
        let count = root.selectOptionSourceCount(root.lr2RandomButtonId(side), sourceCount);
        return optionState.lr2RandomText(side, count);
    }

    readonly property int lr2HidSudIndexP1: optionState.lr2HidSudIndexP1
    readonly property int lr2HidSudIndexP2: optionState.lr2HidSudIndexP2
    function lr2HidSudIndex(side: var) : var { return optionState.lr2HidSudIndex(side); }
    function setHidSudIndex(side: var, index: var) : void {
        optionState.setHidSudIndex(side, index);
        root.noteGameplayOptionChanged();
    }

    readonly property int lr2HiSpeedFixIndex: optionState.lr2HiSpeedFixIndex
    function setHiSpeedFixIndex(index: var) : void { optionState.setHiSpeedFixIndex(index); }

    readonly property int lr2BattleIndex: optionState.lr2BattleIndex
    function ensureBattleProfiles() : var { return optionState.ensureBattleProfiles(); }
    function setBattleIndex(index: var) : void { optionState.setBattleIndex(index); }

    readonly property int lr2DpOptionIndex: optionState.lr2DpOptionIndex
    function setDpOptionIndex(index: var) : void { optionState.setDpOptionIndex(index); }

    readonly property int lr2FlipIndex: optionState.lr2FlipIndex
    function setFlipIndex(index: var) : void { optionState.setFlipIndex(index); }

    readonly property int lr2LaneCoverIndex: optionState.lr2LaneCoverIndex
    function setLaneCoverIndex(index: var) : void {
        optionState.setLaneCoverIndex(index);
        root.noteGameplayOptionChanged();
    }
    readonly property int lr2LiftIndex: optionState.lr2LiftIndex
    function setLiftIndex(index: var) : void {
        optionState.setLiftIndex(index);
        root.noteGameplayOptionChanged();
    }
    readonly property int lr2HiddenIndex: optionState.lr2HiddenIndex
    function setHiddenIndex(index: var) : void {
        optionState.setHiddenIndex(index);
        root.noteGameplayOptionChanged();
    }
    function toggleLaneCover(side: var) : void {
        optionState.toggleLaneCover(side);
        root.noteGameplayOptionChanged();
    }

    readonly property int lr2BgaIndex: optionState.lr2BgaIndex
    readonly property int lr2BeatorajaBgaIndex: optionState.lr2BeatorajaBgaIndex
    function setBgaIndex(index: var) : void { optionState.setBgaIndex(index); }

    readonly property int lr2BgaSizeIndex: optionState.lr2BgaSizeIndex
    function setBgaSizeIndex(index: var) : void { optionState.setBgaSizeIndex(index); }

    readonly property int lr2GaugeAutoShiftIndex: optionState.lr2GaugeAutoShiftIndex
    function setGaugeAutoShiftIndex(index: var) : void { optionState.setGaugeAutoShiftIndex(index); }

    readonly property int lr2BottomShiftableGaugeIndex: optionState.lr2BottomShiftableGaugeIndex
    function setBottomShiftableGaugeIndex(index: var) : var { return optionState.setBottomShiftableGaugeIndex(index); }

    readonly property int lr2LnModeIndex: optionState.lr2LnModeIndex
    function setLnModeIndex(index: var) : var { return optionState.setLnModeIndex(index); }
    function beatorajaAssistOptionFrame(buttonId: var) : var {
        return optionState.beatorajaAssistOptionFrame(buttonId);
    }
    function toggleBeatorajaAssistOption(buttonId: var) : var {
        return optionState.toggleBeatorajaAssistOption(buttonId);
    }

    readonly property int lr2JudgeAlgorithmIndex: optionState.lr2JudgeAlgorithmIndex
    function setJudgeAlgorithmIndex(index: var) : var { return optionState.setJudgeAlgorithmIndex(index); }

    readonly property int lr2ScoreGraphIndex: optionState.lr2ScoreGraphIndex
    function setScoreGraphIndex(index: var) : void { optionState.setScoreGraphIndex(index); }

    readonly property int lr2GhostIndex: optionState.lr2GhostIndex
    function setGhostIndex(index: var) : void { optionState.setGhostIndex(index); }

    readonly property bool lr2BgaEnabled: optionState.lr2BgaEnabled

    readonly property int lr2ScoreTargetIndex: optionState.lr2ScoreTargetIndex
    function setScoreTargetIndex(index: var) : void { optionState.setScoreTargetIndex(index); }

    readonly property int lr2ClassicTargetIndex: optionState.lr2ClassicTargetIndex
    function setClassicTargetIndex(index: var) : void { optionState.setClassicTargetIndex(index); }

    readonly property int lr2BeatorajaTargetIndex: optionState.lr2BeatorajaTargetIndex
    function setBeatorajaTargetIndex(index: var) : void { optionState.setBeatorajaTargetIndex(index); }

    function lr2TargetButtonFrame(sourceCount: var) : var {
        let count = root.selectOptionSourceCount(77, sourceCount);
        return optionState.lr2TargetButtonFrame(count);
    }

    function setTargetButtonIndex(index: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(77, sourceCount);
        optionState.setTargetButtonIndex(index, count);
    }

    function adjustTargetButtonIndex(delta: var, sourceCount: var) : void {
        let count = root.selectOptionSourceCount(77, sourceCount);
        optionState.adjustTargetButtonIndex(delta, count);
    }

    function lr2TargetText(sourceCount: var) : var {
        let count = root.selectOptionSourceCount(77, sourceCount);
        return optionState.lr2TargetText(count);
    }

    function lr2TargetNameText(textId: var) : var {
        return optionState.lr2TargetNameText(textId);
    }

    readonly property int lr2TargetPercent: optionState.lr2TargetPercent
    function setTargetPercent(percent: var) : void { optionState.setTargetPercent(percent); }

    readonly property int lr2HiSpeedP1: optionState.lr2HiSpeedP1
    readonly property int lr2HiSpeedP2: optionState.lr2HiSpeedP2
    function noteGameplayOptionChanged() : var {
        if (!root.gameplayScreenActive) {
            return;
        }
        root.requestGameplayRevisionRefresh();
    }

    function setHiSpeedNumber(side: var, value: var) : void {
        optionState.setHiSpeedNumber(side, value);
        root.noteGameplayOptionChanged();
    }
    function nextLr2HiSpeedNumber(current: var, steps: var) : var { return optionState.nextLr2HiSpeedNumber(current, steps); }
    function adjustHiSpeedNumber(side: var, steps: var) : void {
        optionState.adjustHiSpeedNumber(side, steps);
        root.noteGameplayOptionChanged();
    }
    function adjustDurationNumber(side: var, steps: var) : void {
        optionState.adjustDurationNumber(side, steps);
        root.noteGameplayOptionChanged();
    }
    function adjustScratchDurationNumber(side: var, amount: var) : void {
        optionState.adjustScratchDurationNumber(side, amount);
        root.noteGameplayOptionChanged();
    }
    function adjustScratchCoverNumber(side: var, amount: var, changeLift: var) : void {
        optionState.adjustScratchCoverNumber(side, amount, changeLift);
        root.noteGameplayOptionChanged();
    }
    function adjustLaneCoverRatio(side: var, steps: var) : void {
        optionState.adjustLaneCoverRatio(side, steps);
        root.noteGameplayOptionChanged();
    }
    function adjustGameplayCoverValue(side: var, steps: var, changeLift: var) : void {
        optionState.adjustGameplayCoverValue(side, steps, changeLift);
        root.noteGameplayOptionChanged();
    }
    function adjustOffset(delta: var) : void { optionState.adjustOffset(delta); }

    function isLoggedIn() : var { return optionState.isLoggedIn(); }
    readonly property var lr2RankingChart: lr2Ranking.chart
    readonly property string lr2RankingMd5: lr2Ranking.md5
    property alias lr2RankingRequestMd5: lr2Ranking.requestMd5
    property alias lr2RankingOpenWhenReady: lr2Ranking.openWhenReady
    property alias lr2InternetRankingOpenWhenReady: lr2Ranking.internetOpenWhenReady
    property alias lr2RankingTransitionPhase: lr2Ranking.transitionPhase
    property alias lr2RankingTransitionAction: lr2Ranking.transitionAction
    property alias lr2RankingTransitionStartSkinTime: lr2Ranking.transitionStartSkinTime
    readonly property int lr2RankingTransitionDuration: lr2Ranking.transitionDuration
    readonly property int lr2RankingTransitionElapsed: lr2Ranking.transitionElapsed

    function commitLr2RankingRequest(chart: var) : var { return lr2Ranking.commitRequest(chart); }
    function startLr2RankingTransition(action: var) : var { return lr2Ranking.startTransition(action); }
    function clearLr2RankingTransition() : void { lr2Ranking.clearTransition(); }
    function enterLr2RankingPostSwapTimer() : void { lr2Ranking.enterPostSwapTimer(); }
    function performLr2RankingOpen() : var { return lr2Ranking.performOpen(); }
    function performLr2RankingClose() : var { return lr2Ranking.performClose(); }
    function advanceLr2RankingTransition() : void { lr2Ranking.advanceTransition(); }
    function requestLr2RankingFetch(chart: var) : var { return lr2Ranking.requestFetch(chart); }
    function finishOpenLr2InternetRanking() : var { return lr2Ranking.finishOpenInternetRanking(); }
    function openLr2InternetRanking() : var {
        selectContext.navigationController.refreshFocusedState();
        return lr2Ranking.openInternetRanking();
    }
    function finishOpenLr2Ranking() : var { return lr2Ranking.finishOpenRanking(); }
    function openLr2Ranking() : var {
        selectContext.navigationController.refreshFocusedState();
        return lr2Ranking.openRanking();
    }
    function closeLr2Ranking() : var { return lr2Ranking.closeRanking(); }
    function isLr2RankingKey(key: var) : var { return key === BmsKey.Col14 || key === BmsKey.Col24; }

    function clearStatusOption() : var { return optionState.clearStatusOption(); }

    Rectangle {
        anchors.fill: parent
        color: "black"
        z: -1000000
    }

    function inferScreenKey(path: var) : var {
        return screenState.inferKey(path);
    }

    function panelMatches(panel: var) : var {
        const panelId = Number(panel || 0);
        if (panelId > 0) {
            return root.selectPanel === panelId;
        }
        if (panelId < 0) {
            return root.selectPanel === 0;
        }
        return true;
    }

    function selectInputReady() : var {
        return screenState.selectInputReady;
    }

    function selectPointerInputReady() : var {
        return screenState.selectPointerInputReady;
    }

    function selectScrollReady() : var {
        return screenState.selectScrollReady;
    }

    function selectPointerScrollReady() : var {
        return screenState.selectPointerScrollReady;
    }

    function selectNavigationReady() : var {
        return screenState.selectNavigationReady;
    }

    function clearSelectSearchFocus() : void { selectSearchState.clearFocus(); }
    function resetSelectSearch() : void { selectSearchState.reset(); }

    function fallbackSortId(dsts: var) : var {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return 0;
        }
        return dsts[0].sortId || 0;
    }

    readonly property real selectBarElementSortBase: root.computeSelectBarElementSortBase()

    function updateMinSortId(current: var, dsts: var) : var {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return current;
        }
        const sortId = dsts[0].sortId || 0;
        return current < 0 || sortId < current ? sortId : current;
    }

    function computeSelectBarElementSortBase() : var {
        let result = -1;
        const rows = skinModel.barRows || [];
        for (let rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
            const row = rows[rowIndex];
            if (!row) {
                continue;
            }
            result = root.updateMinSortId(result, row.offDsts);
            result = root.updateMinSortId(result, row.onDsts);
        }
        return result >= 0 ? result : 0;
    }

    function isSelectBarElement(type: var, src: var) : var {
        return type === 4
            || type === 5
            || type === 13
            || (type === 3 && !!src && src.kind !== undefined);
    }

    function selectBarElementLayer(type: var, src: var) : var {
        if (type === 13) {
            return 0.20; // folder progress graph, after the row body
        }
        if (type === 4) {
            return 0.30; // bar title
        }
        if (type === 5) {
            return 0.60; // level number
        }
        if (type !== 3 || !src) {
            return 0.0;
        }

        switch (src.kind || 0) {
        case 0: // BAR_BODY_OFF
        case 1: // BAR_BODY_ON, consumed by the row body cache
            return 0.0;
        case 2: // BAR_FLASH
            return 0.10;
        case 3: // BAR_LAMP
        case 4: // BAR_MY_LAMP
        case 5: // BAR_RIVAL_LAMP
            return 0.50;
        case 6: // BAR_RANK
        case 7: // BAR_RIVAL
            return 0.65;
        case 8: // BAR_LABEL: long note / random / mine labels
            return 0.70;
        default:
            return 0.40;
        }
    }

    function timelineSortId(dsts: var, skinTime: var, activeOptions: var, timers: var) : var {
        let state = timelineResolver.stateFor(
            dsts,
            skinTime,
            timers || root.timers,
            activeOptions || root.runtimeActiveOptions);
        if (state && state.valid) {
            return state.sortId;
        }
        return root.fallbackSortId(dsts);
    }

    function noteElementSortId(skinTime: var, activeOptions: var, timers: var) : var {
        let noteDsts = skinModel.noteDsts || [];
        let result = -1;
        for (let lane = 0; lane < noteDsts.length; ++lane) {
            let laneDsts = noteDsts[lane] || [];
            if (laneDsts.length === 0) {
                continue;
            }
            let sortId = root.timelineSortId(laneDsts, skinTime, activeOptions, timers);
            if (result < 0 || sortId < result) {
                result = sortId;
            }
        }
        return result >= 0 ? result : 0;
    }

    function staticNoteElementSortId() : var {
        let noteDsts = skinModel.noteDsts || [];
        let result = -1;
        for (let lane = 0; lane < noteDsts.length; ++lane) {
            let laneDsts = noteDsts[lane] || [];
            if (laneDsts.length === 0) {
                continue;
            }
            let sortId = root.fallbackSortId(laneDsts);
            if (result < 0 || sortId < result) {
                result = sortId;
            }
        }
        return result >= 0 ? result : 0;
    }

    function elementZ(type: var, index: var, src: var, dsts: var) : var {
        // LR2/beatoraja draw BAR_* parts through a single bar renderer. Their
        // #DST_BAR_* command positions configure that renderer, but the
        // resulting draw calls still participate in the global LR2 sort order.
        if (root.isSelectBarElement(type, src)) {
            return root.fallbackSortId(dsts)
                + root.selectBarElementLayer(type, src) * 0.001
                + index * 0.000000001;
        }

        // OpenLR2 stamps every non-bar DST keyframe with the skin command
        // counter and globally sorts DrawingBuf by that value before drawing.
        if (type === 8) {
            return root.staticNoteElementSortId() + index * 0.000001;
        }
        return root.fallbackSortId(dsts) + index * 0.000001;
    }

    function addOption(options: var, option: var) : var {
        if (option === undefined || option === null) {
            return;
        }
        let absOption = Math.abs(option);
        if (root.usedOptionFilterActive
                && !root.usedOptionLookup[absOption]) {
            return;
        }
        let lookup = root.optionLookupFor(options);
        if (lookup[option] === true) {
            return;
        }
        options.push(option);
        lookup[option] = true;
    }

    readonly property string activeGaugeName1: root.resolveActiveGaugeNameForSide(1)
    readonly property string activeGaugeName2: root.resolveActiveGaugeNameForSide(2)
    readonly property bool activeGaugeSurvival1: root.gaugeNameIsSurvival(root.activeGaugeName1)
    readonly property bool activeGaugeSurvival2: root.gaugeNameIsSurvival(root.activeGaugeName2)
    readonly property bool activeGaugeUsesExSprites1: root.gaugeNameUsesExGaugeSprites(root.activeGaugeName1)
    readonly property bool activeGaugeUsesExSprites2: root.gaugeNameUsesExGaugeSprites(root.activeGaugeName2)

    function configuredGaugeName(side: var) : var {
        let vars = root.generalVarsForSide(side);
        return String(vars ? vars.gaugeType : "").toUpperCase();
    }

    function gaugeAboveThreshold(value: var, threshold: var) : var {
        return Number(value || 0) > Number(threshold || 0);
    }

    function gameplayActiveGauge(score: var) : var {
        if (!score || !score.gauges || score.gauges.length === 0) {
            return null;
        }
        let gauges = score.gauges;
        for (let gauge of gauges) {
            if (root.gaugeAboveThreshold(gauge.gauge, gauge.threshold)) {
                return gauge;
            }
        }
        return gauges[gauges.length - 1] || null;
    }

    function gameplayActiveGaugeName(score: var) : var {
        let gauge = root.gameplayActiveGauge(score);
        return String(gauge && gauge.name ? gauge.name : "").toUpperCase();
    }

    function resolveActiveGaugeNameForSide(side: var) : var {
        if (root.gameplayScreenActive) {
            let activeName = root.gameplayActiveGaugeName(root.gameplayScore(side));
            if (activeName.length > 0) {
                return activeName;
            }
        }
        if (root.resultScreenActive) {
            let resultName = root.resultGaugeName(side);
            if (resultName.length > 0) {
                return resultName;
            }
        }
        return root.configuredGaugeName(side);
    }

    function activeGaugeNameForSide(side: var) : var {
        return side === 2 ? root.activeGaugeName2 : root.activeGaugeName1;
    }

    function gaugeNameIsSurvival(name: var) : var {
        switch (String(name || "").toUpperCase()) {
        case "HARD":
        case "EXHARD":
        case "FC":
        case "PERFECT":
        case "MAX":
        case "DAN":
        case "EXDAN":
        case "EXHARDDAN":
            return true;
        default:
            return false;
        }
    }

    function gaugeNameUsesBeatorajaExOption(name: var) : var {
        switch (String(name || "").toUpperCase()) {
        case "AEASY":
        case "EASY":
        case "EXHARD":
        case "FC":
        case "PERFECT":
        case "MAX":
        case "EXDAN":
        case "EXHARDDAN":
            return true;
        default:
            return false;
        }
    }

    function gaugeNameUsesExGaugeSprites(name: var) : var {
        switch (String(name || "").toUpperCase()) {
        case "EXHARD":
        case "FC":
        case "PERFECT":
        case "MAX":
        case "EXDAN":
        case "EXHARDDAN":
            return true;
        default:
            return false;
        }
    }

    function addGaugeExOption(options: var, side: var) : void {
        if (!root.lr2SkinUsesBeatorajaSemantics) {
            return;
        }
        let gauge = root.activeGaugeNameForSide(side);
        if (root.gaugeNameUsesBeatorajaExOption(gauge)) {
            root.addOption(options, side === 2 ? 1047 : 1046);
        }
    }

    function gaugeColorOption(side: var) : var {
        let red = root.gaugeNameIsSurvival(root.activeGaugeNameForSide(side));
        return side === 2 ? (red ? 45 : 44) : (red ? 43 : 42);
    }

    function gameplayGaugeTrophyOption(side: var) : var {
        let gauge = root.activeGaugeNameForSide(side);
        if (!root.lr2SkinUsesBeatorajaSemantics) {
            switch (root.skinClearTypeForStatus(gauge)) {
            case "EASY":
                return 121;
            case "HARD":
                return 119;
            case "NORMAL":
                return 118;
            default:
                return undefined;
            }
        }
        if (gauge === "AEASY") {
            return 124;
        }
        if (gauge === "EASY") {
            return 121;
        }
        if (gauge === "EXHARD" || gauge === "EXDAN" || gauge === "EXHARDDAN") {
            return 125;
        }
        if (gauge === "HARD" || gauge === "DAN" || gauge === "HARD_DAN") {
            return 119;
        }
        return 118;
    }

    function battleModeActive() : var {
        return !!(root.chart && root.chart.player2)
            || !!(Rg.profileList && Rg.profileList.battleActive);
    }

    function spToDpActive() : var {
        if (root.chart && !root.chart.player2) {
            let chartData = root.gameplayChartData();
            let sourceKeymode = chartData ? Number(chartData.keymode || 0) : 0;
            let effectiveKeymode = Number(root.chart.keymode || 0);
            if ((sourceKeymode === 5 && effectiveKeymode === 10)
                    || (sourceKeymode === 7 && effectiveKeymode === 14)) {
                return true;
            }
        }
        let vars = root.mainGeneralVarsRef;
        return !!vars && vars.dpOptions === DpOptions.Battle;
    }

    function laneCoverNumber(side: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return 0;
        }
        if (root.gameplayScreenActive) {
            let cover = vars.laneCoverOn ? (vars.laneCoverRatio || 0) : 0;
            let lift = vars.liftOn ? (vars.liftRatio || 0) : 0;
            return Math.round(Math.max(0, Math.min(1, 1 - cover - lift)) * 1000);
        }
        return Math.round((vars.laneCoverRatio || 0) * 1000);
    }

    function gameplayLaneCoverSliderPosition(side: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars || !vars.laneCoverOn) {
            return 0;
        }
        let lane = vars.laneCoverRatio || 0;
        if (vars.liftOn) {
            lane *= 1 - (vars.liftRatio || 0);
        }
        return lane;
    }

    function liftNumber(side: var) : var {
        let vars = root.generalVarsForSide(side);
        return vars && vars.liftOn ? Math.round((vars.liftRatio || 0) * 1000) : 0;
    }

    function hiddenNumber(side: var) : var {
        let vars = root.generalVarsForSide(side);
        return vars && vars.hiddenOn ? Math.round((vars.hiddenRatio || 0) * 1000) : 0;
    }

    function hiSpeedInteger(side: var) : var {
        let value = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        return Math.floor(value / 100);
    }

    function hiSpeedAfterDot(side: var) : var {
        let value = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        return Math.floor(value) % 100;
    }

    function durationNumber(side: var, green: var) : var {
        let vars = root.generalVarsForSide(side);
        let duration = vars && vars.noteScreenTimeMillis > 0 ? vars.noteScreenTimeMillis : 1000;
        return Math.round(duration * (green ? 0.6 : 1.0));
    }

    function durationNumberForBpm(side: var, bpm: var, green: var, cover: var) : var {
        let safeBpm = Math.max(1, bpm || 0);
        let hiSpeed = Math.max(0.01, (side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1) / 100);
        let vars = root.generalVarsForSide(side);
        let visible = cover && vars ? 1 - (vars.laneCoverRatio || 0) : 1;
        return Math.round((240000 / safeBpm / hiSpeed) * visible * (green ? 1.0 : 0.6));
    }

    function firstDstStateY(dsts: var) : var {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return 0;
        }
        let dst = dsts[0];
        return Math.abs(dst.y || 0);
    }

    function gameplayLaneOffsetHeight(side: var) : var {
        let model = root.skinModelRef || skinModel;
        let start = side === 2 ? 10 : 0;
        let end = side === 2 ? 20 : 10;
        let lineDsts = model && model.lineDsts ? model.lineDsts : [];
        for (let i = start; i < end && i < lineDsts.length; ++i) {
            let lineY = root.firstDstStateY(lineDsts[i]);
            if (lineY > 0) {
                return lineY;
            }
        }
        let noteDsts = model && model.noteDsts ? model.noteDsts : [];
        for (let lane = start; lane < end && lane < noteDsts.length; ++lane) {
            let noteY = root.firstDstStateY(noteDsts[lane]);
            if (noteY > 0) {
                return noteY;
            }
        }
        return Math.max(1, root.skinH || 480);
    }

    readonly property real gameplayLaneOffsetHeightP1: root.gameplayLaneOffsetHeight(1)
    readonly property real gameplayLaneOffsetHeightP2: root.gameplayLaneOffsetHeight(2)

    function cachedGameplayLaneOffsetHeight(side: var) : var {
        return side === 2 ? root.gameplayLaneOffsetHeightP2 : root.gameplayLaneOffsetHeightP1;
    }

    function lr2Clamp01(value: var) : var {
        return Math.max(0, Math.min(value || 0, 1));
    }

    function computeGameplayDstOffsetLiftY(side: var) : var {
        if (!root.gameplayScreenActive) {
            return 0;
        }
        let vars = root.generalVarsForSide(side);
        return vars && vars.liftOn
            ? -root.cachedGameplayLaneOffsetHeight(side) * root.lr2Clamp01(vars.liftRatio)
            : 0;
    }

    function computeGameplayDstOffsetLaneCoverY(side: var) : var {
        if (!root.gameplayScreenActive) {
            return 0;
        }
        let vars = root.generalVarsForSide(side);
        if (!vars || !vars.laneCoverOn) {
            return 0;
        }
        let height = root.cachedGameplayLaneOffsetHeight(side);
        let liftRatio = vars.liftOn ? root.lr2Clamp01(vars.liftRatio) : 0;
        let visibleHeight = height * Math.max(0, 1 - liftRatio);
        return visibleHeight * root.lr2Clamp01(vars.laneCoverRatio);
    }

    function computeGameplayDstOffsetHiddenY(side: var) : var {
        if (!root.gameplayScreenActive) {
            return 0;
        }
        let vars = root.generalVarsForSide(side);
        if (!vars || !vars.hiddenOn) {
            return 0;
        }
        let height = root.cachedGameplayLaneOffsetHeight(side);
        let liftRatio = vars.liftOn ? root.lr2Clamp01(vars.liftRatio) : 0;
        let visibleHeight = height * Math.max(0, 1 - liftRatio);
        return visibleHeight * root.lr2Clamp01(vars.hiddenRatio);
    }

    function computeGameplayDstOffsetHiddenA(side: var) : var {
        if (!root.gameplayScreenActive) {
            return 0;
        }
        let vars = root.generalVarsForSide(side);
        return vars && vars.hiddenOn ? 0 : -255;
    }

    readonly property real gameplayDstOffsetLiftY1: root.computeGameplayDstOffsetLiftY(1)
    readonly property real gameplayDstOffsetLiftY2: root.computeGameplayDstOffsetLiftY(2)
    readonly property real gameplayDstOffsetLaneCoverY1: root.computeGameplayDstOffsetLaneCoverY(1)
    readonly property real gameplayDstOffsetLaneCoverY2: root.computeGameplayDstOffsetLaneCoverY(2)
    readonly property real gameplayDstOffsetHiddenY1: root.computeGameplayDstOffsetHiddenY(1)
    readonly property real gameplayDstOffsetHiddenY2: root.computeGameplayDstOffsetHiddenY(2)
    readonly property real gameplayDstOffsetHiddenA1: root.computeGameplayDstOffsetHiddenA(1)
    readonly property real gameplayDstOffsetHiddenA2: root.computeGameplayDstOffsetHiddenA(2)

    function bpmDurationNumber(num: var, chartData: var) : var {
        let green = (num - 1312) % 2 === 1;
        let cover = (num - 1312) % 4 < 2;
        let mode = Math.floor((num - 1312) / 4);
        let bpm = 0;
        if (mode === 1) {
            bpm = chartData ? (chartData.mainBpm || chartData.initialBpm || chartData.maxBpm || 0) : 0;
        } else if (mode === 2) {
            bpm = chartData ? (chartData.minBpm || chartData.mainBpm || 0) : 0;
        } else if (mode === 3) {
            bpm = chartData ? (chartData.maxBpm || chartData.mainBpm || 0) : 0;
        } else {
            let player = root.gameplayPlayer(1);
            bpm = player && (player.bpm || 0) > 0
                ? player.bpm
                : (chartData ? (chartData.mainBpm || chartData.initialBpm || 0) : 0);
        }
        return root.durationNumberForBpm(1, bpm, green, cover);
    }

    function chartLengthSeconds(chartData: var) : var {
        return chartData ? Math.floor(Math.max(0, chartData.length || 0) / 1000000000) : -1;
    }

    function chartPlayableNoteCount(chartData: var) : var {
        if (!chartData) {
            return 0;
        }
        return (chartData.normalNoteCount || 0)
            + (chartData.scratchCount || 0)
            + (chartData.lnCount || 0)
            + (chartData.bssCount || 0);
    }

    function chartDensityNumber(chartData: var, propertyName: var, afterDot: var) : var {
        if (!chartData) {
            return -1;
        }
        let value = Math.max(0, chartData[propertyName] || 0);
        return afterDot ? Math.floor(value * 100) % 100 : Math.floor(value);
    }

    function chartHasBpmStop(chartData: var) : var {
        let changes = chartData && chartData.bpmChanges ? chartData.bpmChanges : [];
        for (let i = 0; i < changes.length; ++i) {
            if ((changes[i].bpm || 0) === 0) {
                return true;
            }
        }
        return false;
    }

    function clearTypeValue(clearType: var) : var {
        switch (root.skinClearTypeForStatus(clearType)) {
        case "FAILED":
            return 1;
        case "AEASY":
            return 2;
        case "LIGHTASSIST":
            return 3;
        case "EASY":
            return 4;
        case "NORMAL":
            return 5;
        case "HARD":
            return 6;
        case "EXHARD":
            return 7;
        case "FC":
            return 8;
        case "PERFECT":
            return 9;
        case "MAX":
            return 10;
        default:
            return 0;
        }
    }

    function dateTimeNumber(num: var) : var {
        switch (num) {
        case 20:
            return root.lr2CurrentFps;
        case 21:
            return root.lr2ClockYear;
        case 22:
            return root.lr2ClockMonth;
        case 23:
            return root.lr2ClockDay;
        case 24:
            return root.lr2ClockHour;
        case 25:
            return root.lr2ClockMinute;
        case 26:
            return root.lr2ClockSecond;
        case 27:
            return wallClockState.uptimeHours;
        case 28:
            return wallClockState.uptimeMinutes;
        case 29:
            return wallClockState.uptimeSecondPart;
        default:
            return 0;
        }
    }

    function updateLr2DateTimeNumbers() : void {
        wallClockState.update();
    }

    function chartStatusValue(status: var) : var {
        if (status === ChartRunner.Loading || status === "Loading") {
            return ChartRunner.Loading;
        }
        if (status === ChartRunner.Ready || status === "Ready") {
            return ChartRunner.Ready;
        }
        if (status === ChartRunner.Running || status === "Running") {
            return ChartRunner.Running;
        }
        if (status === ChartRunner.Finished || status === "Finished") {
            return ChartRunner.Finished;
        }

        let text = String(status);
        if (text.indexOf("Loading") >= 0) {
            return ChartRunner.Loading;
        }
        if (text.indexOf("Ready") >= 0) {
            return ChartRunner.Ready;
        }
        if (text.indexOf("Running") >= 0) {
            return ChartRunner.Running;
        }
        if (text.indexOf("Finished") >= 0) {
            return ChartRunner.Finished;
        }

        let numeric = Number(status);
        return isNaN(numeric) ? ChartRunner.Loading : numeric;
    }

    function chartStatusIs(status: var, expected: var) : var {
        return root.chartStatusValue(status) === expected;
    }

    function chartStatusAtLeast(status: var, minimum: var) : var {
        return root.chartStatusValue(status) >= minimum;
    }

    function gameplayStatusAtLeast(minimum: var) : var {
        if (!root.chart || root.chart.status === undefined) {
            return false;
        }
        if (root.chartStatusAtLeast(root.chart.status, minimum)) {
            return true;
        }

        let player1 = root.gameplayPlayer(1);
        let player2 = root.gameplayPlayer(2);
        if (player1 && player1.status !== undefined && root.chartStatusAtLeast(player1.status, minimum)) {
            return true;
        }
        if (player2 && player2.status !== undefined && root.chartStatusAtLeast(player2.status, minimum)) {
            return true;
        }

        // If the backend is already advancing the chart, LR2's loading/start timers
        // must be considered fired even if an aggregate runner status lags behind.
        if (minimum <= ChartRunner.Running) {
            return (player1 && (player1.elapsed || 0) > 0)
                || (player2 && (player2.elapsed || 0) > 0);
        }
        return false;
    }

    function isCourseGameplay() : var {
        return !!(root.chart && root.chart.chartDatas && root.chart.currentChartIndex !== undefined);
    }

    function isFinalCourseStage() : var {
        if (!root.isCourseGameplay() || !root.chart.chartDatas) {
            return false;
        }
        let chartCount = root.chart.chartDatas.length || 0;
        return chartCount > 0 && (root.chart.currentChartIndex || 0) >= chartCount - 1;
    }

    function gameplayProfiles() : var {
        return [
            root.chart && root.chart.player1 ? root.chart.player1.profile : null,
            root.chart && root.chart.player2 ? root.chart.player2.profile : null
        ];
    }

    function stopGameplayLifecycle() : void {
        root.gameplayStartArmed = false;
        skinTiming.gameplayStartTimer.stop();
        gameplayReadySound.stop();
        gameplayStopSound.stop();
    }

    function startGameplayWhenReady() : var {
        if (!root.enabled
                || !root.gameplayScreenActive
                || !root.chart
                || !root.chartStatusIs(root.chart.status, ChartRunner.Ready)
                || root.gameplayReadySkinTime < 0
                || StackView.status !== StackView.Active
                || root.gameplayStartArmed) {
            return;
        }

        if (gameplayReadySound.length > 0) {
            gameplayReadySound.stop();
            root.gameplayStartArmed = true;
            gameplayReadySound.play();
        } else {
            root.gameplayStartArmed = true;
            skinTiming.gameplayStartTimer.restart();
        }
    }

    function activateGameplayIfNeeded() : var {
        if (!root.enabled
                || !root.gameplayScreenActive
                || !root.chart
                || StackView.status !== StackView.Active) {
            return;
        }

        root.forceActiveFocus();
        if (root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
            if (root.isCourseGameplay() && root.gameplayCourseResultPending && !root.gameplayShowedCourseResult) {
                root.gameplayCourseResultPending = false;
                root.gameplayShowedCourseResult = true;
                let profiles = root.gameplayProfiles();
                let chartDatas = root.chart.chartDatas;
                let course = root.chart.course;
                Qt.callLater(() => {
                    if (root.enabled && root.gameplayScreenActive && root.chart && root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
                        globalRoot.openCourseResult(root.chart.finish(), profiles, chartDatas, course);
                    }
                });
            } else if (root.isCourseGameplay() && !root.gameplayResultOpened) {
                Qt.callLater(() => {
                    if (root.enabled && root.gameplayScreenActive && root.chart && root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
                        root.openGameplayStageResult();
                    }
                });
            } else {
                Qt.callLater(() => {
                    if (root.enabled && root.gameplayScreenActive && root.chart && root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
                        sceneStack.pop();
                    }
                });
            }
            return;
        }

        root.gameplayResultOpened = false;
        root.gameplayPlayStopped = false;
        if (!root.chartStatusIs(root.chart.status, ChartRunner.Running)) {
            root.gameplayNothingWasHit = true;
        }
        root.resetGameplayScoreReplayers();
        root.updateGameplaySavedScores();
        root.updateGameplayStatusTimers();
        root.startGameplayWhenReady();
    }

    function openGameplayStageResult() : var {
        if (!root.enabled
                || !root.gameplayScreenActive
                || !root.chart
                || root.gameplayResultOpened) {
            return;
        }

        root.gameplayResultOpened = true;
        root.gameplayStartArmed = false;
        skinTiming.gameplayStartTimer.stop();

        let chartData = root.gameplayChartData();
        let profiles = root.gameplayProfiles();
        let finalCourseStage = root.isFinalCourseStage();
        let scores = root.chart instanceof ChartRunner ? root.chart.finish() : root.chart.proceed();
        if (finalCourseStage) {
            root.gameplayCourseResultPending = true;
        }
        globalRoot.openResult(scores, profiles, chartData);
    }

    function handleGameplayStatusChanged() : var {
        root.updateGameplayStatusTimers();
        if (!root.enabled || !root.gameplayScreenActive || !root.chart) {
            return;
        }
        if (root.chartStatusIs(root.chart.status, ChartRunner.Ready)) {
            root.startGameplayWhenReady();
        } else if (root.chartStatusIs(root.chart.status, ChartRunner.Running)) {
            root.gameplayStartArmed = false;
            skinTiming.gameplayStartTimer.stop();
        } else if (root.chartStatusIs(root.chart.status, ChartRunner.Finished) && !root.gameplayPlayStopped) {
            root.openGameplayStageResult();
        }
    }

    function gameplayHitCountsAsPlayed(hit: var) : var {
        if (!hit || !hit.points) {
            return false;
        }
        let judgement = hit.points.judgement;
        return judgement !== Judgement.Poor
            && judgement !== Judgement.EmptyPoor
            && judgement !== Judgement.MineHit
            && judgement !== Judgement.MineAvoided;
    }

    function handleGameplayEscape() : var {
        if (!root.chart || root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
            return false;
        }
        if (root.gameplayNothingWasHit) {
            sceneStack.pop();
            return true;
        }

        root.gameplayPlayStopped = true;
        gameplayStopSound.play();
        root.openGameplayStageResult();
        return true;
    }

    function gameplayChartData() : var {
        if (!root.chart) {
            return null;
        }
        if (root.chart.chartData) {
            return root.chart.chartData;
        }
        if (root.chart.chartDatas && root.chart.currentChartIndex !== undefined) {
            return root.chart.chartDatas[root.chart.currentChartIndex] || null;
        }
        return null;
    }

    function resultScore(side: var) : var { return resultState.resultScore(side); }
    function resultData(side: var) : var { return resultState.resultData(side); }
    function resultProfile(side: var) : var { return resultState.resultProfile(side); }
    function resultChartData() : var { return resultState.resultChartData(); }
    function displayChartData() : var {
        if (root.effectiveScreenKey === "select") {
            return selectContext.selectedStateChartData;
        }
        return resultState.displayChartData();
    }
    function chartTableHash(chartData: var) : var {
        root.tableInfoRevision;
        if (typeof chartData === "string") {
            return String(chartData || "");
        }
        return chartData && chartData.md5 ? String(chartData.md5) : "";
    }
    function tableInfoForHash(hash: var) : var {
        root.tableInfoRevision;
        let normalized = String(hash || "");
        if (normalized.length <= 0 || !Rg.tables || !Rg.tables.search) {
            return null;
        }
        let matches = Rg.tables.search(normalized);
        return matches && matches.length > 0 ? matches[0] : null;
    }
    function tableInfoForChart(chartData: var) : var {
        return root.tableInfoForHash(root.chartTableHash(chartData));
    }
    function tableLevelNameForInfo(info: var) : var {
        if (!info) {
            return "";
        }
        let levelName = String(info.levelName || "");
        let symbol = String(info.symbol || "");
        return symbol.length > 0 && levelName.indexOf(symbol) !== 0
            ? symbol + levelName
            : levelName;
    }
    function tableFullNameForInfo(info: var) : var {
        if (!info) {
            return "";
        }
        return root.tableLevelNameForInfo(info) + String(info.tableName || "");
    }
    function chartTableName(chartData: var) : var {
        let info = root.tableInfoForChart(chartData);
        return info ? String(info.tableName || "") : "";
    }
    function chartTableLevelName(chartData: var) : var {
        return root.tableLevelNameForInfo(root.tableInfoForChart(chartData));
    }
    function chartTableFullName(chartData: var) : var {
        return root.tableFullNameForInfo(root.tableInfoForChart(chartData));
    }
    function chartInTable(chartData: var) : var {
        return !!root.tableInfoForChart(chartData);
    }
    function resultClearOption() : var { return resultState.resultClearOption(); }
    function resultTotalNotes(result: var) : var { return resultState.resultTotalNotes(result); }
    function resultJudgementCount(result: var, judgement: var) : var { return resultState.resultJudgementCount(result, judgement); }
    function resultPoorCount(result: var) : var { return resultState.resultPoorCount(result); }
    function resultBadPoor(result: var) : var { return resultState.resultBadPoor(result); }
    function emptyJudgeTimingCounts() : var { return resultState.emptyJudgeTimingCounts(); }
    function judgementTimingBucket(judgement: var) : var { return resultState.judgementTimingBucket(judgement); }
    function hitDeviationNanos(hit: var) : var { return resultState.hitDeviationNanos(hit); }
    function hitDeviationMillis(hit: var) : var { return resultState.hitDeviationMillis(hit); }
    function judgementUpdatesJudgeTimingValue(judgement: var) : var { return resultState.judgementUpdatesJudgeTimingValue(judgement); }
    function cloneJudgeTimingCounts(counts: var) : var { return resultState.cloneJudgeTimingCounts(counts); }
    function emptyJudgeLaneValues() : var { return resultState.emptyJudgeLaneValues(); }
    function nowJudgeValue(judgement: var) : var { return resultState.nowJudgeValue(judgement); }
    function laneJudgeValue(judgement: var, timing: var) : var { return resultState.laneJudgeValue(judgement, timing); }
    function setGameplayJudgeLaneValue(scoreSide: var, hit: var, value: var) : void { resultState.setGameplayJudgeLaneValue(scoreSide, hit, value); }
    function gameplayJudgeValueForId(num: var) : var { return resultState.gameplayJudgeValueForId(num); }
    function recordGameplayJudgeTiming(scoreSide: var, hit: var) : void { resultState.recordGameplayJudgeTiming(scoreSide, hit); }
    function gameplayJudgeTimingNumber(num: var, side: var) : var { return resultState.gameplayJudgeTimingNumber(num, side); }
    function resultJudgeTimingNumber(num: var, side: var) : var { return resultState.resultJudgeTimingNumber(num, side); }
    function judgeTimingCount(counts: var, bucket: var, early: var) : var { return resultState.judgeTimingCount(counts, bucket, early); }
    function judgeTimingNumberFromCounts(num: var, counts: var) : var { return resultState.judgeTimingNumberFromCounts(num, counts); }
    function resultCacheKey(score: var) : var { return resultState.resultCacheKey(score); }
    function resultJudgeTimingCounts(side: var) : var { return resultState.resultJudgeTimingCounts(side); }
    function resultTimingStats(side: var) : var { return resultState.resultTimingStats(side); }
    function signedAfterDot(value: var) : var { return resultState.signedAfterDot(value); }
    function resultGaugeInfo(side: var) : var { return resultState.resultGaugeInfo(side); }
    function resultGaugeName(side: var) : var { return resultState.resultGaugeName(side); }
    function resultGaugeValue(side: var) : var { return resultState.resultGaugeValue(side); }
    function cycleResultGauge(side: var, delta: var) : var {
        if (!resultState.cycleResultGauge(side, delta)) {
            return false;
        }
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        return true;
    }
    function handleResultGaugeSelectKey(key: var) : var {
        if (!root.resultInputReady()) {
            return false;
        }
        if (key === BmsKey.Select1) {
            root.cycleResultGauge(1, 1);
            return true;
        }
        if (key === BmsKey.Select2) {
            if (root.resultScore(2)) {
                root.cycleResultGauge(2, 1);
            }
            return true;
        }
        return false;
    }
    function gaugeAfterDot(value: var) : var { return resultState.gaugeAfterDot(value); }
    function resultExScore(result: var) : var { return resultState.resultExScore(result); }
    function resultLr2Score(result: var) : var { return resultState.resultLr2Score(result); }
    function resultScorePrint(result: var) : var { return resultState.resultScorePrint(result); }
    function resultRateInteger(result: var) : var { return resultState.resultRateInteger(result); }
    function resultRateDecimal(result: var) : var { return resultState.resultRateDecimal(result); }
    function resultScoreRateInteger(points: var, result: var) : var { return resultState.resultScoreRateInteger(points, result); }
    function resultScoreRateDecimal(points: var, result: var) : var { return resultState.resultScoreRateDecimal(points, result); }
    function resultRawRank(result: var) : var { return resultState.resultRawRank(result); }
    function resultRankDelta(result: var) : var { return resultState.resultRankDelta(result); }
    function resultRankOptionForResult(result: var, baseOption: var) : var { return resultState.resultRankOptionForResult(result, baseOption); }
    function resultOldScores(side: var) : var { return resultState.resultOldScores(side); }
    function resultBestScoreByPoints(scores: var) : var { return resultState.resultBestScoreByPoints(scores); }
    function resultOldBestScore(side: var) : var { return resultState.resultOldBestScore(side); }
    function resultLastOldScore(side: var) : var { return resultState.resultLastOldScore(side); }
    function resultTargetSavedScore(side: var) : var { return resultState.resultTargetSavedScore(side); }
    function resultTargetFraction(side: var) : var { return resultState.resultTargetFraction(side); }
    function resultTargetPoints(side: var) : var { return resultState.resultTargetPoints(side); }
    function resultHighScorePoints(side: var) : var { return resultState.resultHighScorePoints(side); }
    function resultTargetMaxPoints(side: var) : var { return resultState.resultTargetMaxPoints(side); }
    function resultOldBestResult(side: var) : var { return resultState.resultOldBestResult(side); }
    function resultUpdatedBestResult(side: var) : var { return resultState.resultUpdatedBestResult(side); }
    function resultScoreImproved(side: var) : var { return resultState.resultScoreImproved(side); }
    function resultComboImproved(side: var) : var { return resultState.resultComboImproved(side); }
    function resultBadPoorImproved(side: var) : var { return resultState.resultBadPoorImproved(side); }
    function updateResultOldScores() : void { resultState.updateResultOldScores(); }
    function gameplayPlayer(side: var) : var {
        return side === 2 ? root.gameplayPlayer2 : root.gameplayPlayer1;
    }

    function gameplayKeymode() : var {
        if (root.chart && root.chart.keymode !== undefined) {
            return root.chart.keymode || 0;
        }
        let chartData = root.gameplayChartData();
        return chartData ? chartData.keymode || 0 : 0;
    }

    function gameplayUsesDoublePlayLanes() : var {
        let keymode = root.gameplayKeymode();
        return keymode === 10 || keymode === 14;
    }

    function gameplayUsesFiveKeyTimers() : var {
        let keymode = root.gameplayKeymode();
        return keymode === 5 || keymode === 10;
    }

    function gameplayLr2LaneAllowedForTimers(lane: var) : var {
        if (lane < 0) {
            return false;
        }
        return !root.gameplayUsesFiveKeyTimers()
            || !((lane >= 6 && lane <= 7) || (lane >= 16 && lane <= 17));
    }

    function gameplayLanePlayer(side: var) : var {
        if (!root.chart) {
            return null;
        }
        if (side === 2) {
            return root.chart.player2 || (root.gameplayUsesDoublePlayLanes() ? root.chart.player1 : null);
        }
        return root.chart.player1;
    }

    function gameplayEngineColumnForLr2Lane(lane: var) : var {
        if (lane >= 10) {
            if (root.chart && root.chart.player2) {
                return lane === 10 ? 7 : lane - 11;
            }
            return lane === 10 ? 15 : 8 + lane - 11;
        }
        return lane === 0 ? 7 : lane - 1;
    }

    function gameplayLr2LaneForKeyTimer(timer: var) : var {
        if (root.gameplayUsesFiveKeyTimers()
                && ((timer >= 106 && timer <= 107) || (timer >= 116 && timer <= 117))) {
            return -1;
        }
        if (timer === 100) {
            return 0;
        }
        if (timer >= 101 && timer <= 107) {
            return timer - 100;
        }
        if (timer === 110) {
            return 10;
        }
        if (timer >= 111 && timer <= 117) {
            return timer - 100;
        }
        return -1;
    }

    function gameplayLr2LaneForLongNoteTimer(timer: var) : var {
        if (root.gameplayUsesFiveKeyTimers()
                && ((timer >= 76 && timer <= 77) || (timer >= 86 && timer <= 87))) {
            return -1;
        }
        if (timer === 70) {
            return 0;
        }
        if (timer >= 71 && timer <= 77) {
            return timer - 70;
        }
        if (timer === 80) {
            return 10;
        }
        if (timer >= 81 && timer <= 87) {
            return timer - 70;
        }
        return -1;
    }

    function gameplayKeyOnTimerAllowed(timer: var) : var {
        return root.gameplayLr2LaneForKeyTimer(timer) >= 0;
    }

    function gameplayLongNoteTimerAllowed(timer: var) : var {
        return root.gameplayLr2LaneForLongNoteTimer(timer) >= 0;
    }

    function gameplayHitTimerAllowed(timer: var) : var {
        if (root.gameplayUsesFiveKeyTimers()
                && ((timer >= 56 && timer <= 57) || (timer >= 66 && timer <= 67))) {
            return false;
        }
        return (timer >= 50 && timer <= 57) || (timer >= 60 && timer <= 67);
    }

    function gameplayCoursePlayer(side: var) : var {
        if (!root.chart) {
            return null;
        }
        return side === 2 ? root.chart.coursePlayer2 : root.chart.coursePlayer1;
    }

    function gameplayScore(side: var) : var {
        let player = root.gameplayPlayer(side);
        return player ? player.score : null;
    }

    function gameplayTotalNotes(score: var) : var {
        if (!score) {
            return 0;
        }
        let maxPoints = Math.floor(score.maxPoints || 0);
        if (maxPoints > 0) {
            return Math.floor(maxPoints / 2);
        }
        return Math.max(0, score.maxHits || 0);
    }

    function gameplayCurrentNotes(score: var) : var {
        if (!score) {
            return 0;
        }
        return Math.floor(Math.max(0, score.maxPointsNow || 0) / 2);
    }

    function gameplayExScore(score: var) : var {
        return score ? Math.floor(score.points || 0) : 0;
    }

    function gameplaySavedScorePoints(score: var) : var {
        return score && score.result ? Math.floor(score.result.points || 0) : 0;
    }

    function gameplayScoreRateInteger(points: var, score: var) : var {
        let denominator = root.gameplayTotalNotes(score) * 2;
        return denominator > 0 ? Math.floor(points * 100 / denominator) : 0;
    }

    function gameplayScoreRateDecimal(points: var, score: var) : var {
        let denominator = root.gameplayTotalNotes(score) * 2;
        return denominator > 0 ? Math.floor(points * 10000 / denominator) % 100 : 0;
    }

    function gameplayBestSavedScore() : var {
        root.gameplayScoresRevision;
        let scores = root.gameplayScores1 || [];
        let best = null;
        let bestPoints = -1;
        for (let score of scores) {
            let points = root.gameplaySavedScorePoints(score);
            if (points > bestPoints) {
                best = score;
                bestPoints = points;
            }
        }
        return best;
    }

    function gameplayLastSavedScore() : var {
        root.gameplayScoresRevision;
        let scores = root.gameplayScores1 || [];
        return scores.length > 0 ? scores[0] : null;
    }

    function nextRankTargetPoints(savedPoints: var, maxPoints: var) : var {
        let max = Math.max(0, Math.floor(maxPoints || 0));
        if (max <= 0) {
            return 0;
        }
        let current = Math.max(0, Math.floor(savedPoints || 0));
        for (let rank = 15; rank < 27; ++rank) {
            let target = Math.ceil(max * rank / 27);
            if (current < target) {
                return target;
            }
        }
        return max;
    }

    function gameplayScoreTargetMode() : var {
        let player = root.gameplayPlayer(1);
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        return vars ? vars.scoreTarget : ScoreTarget.BestScore;
    }

    function gameplayTargetSavedScore() : var {
        switch (root.gameplayScoreTargetMode()) {
        case ScoreTarget.LastScore:
            return root.gameplayLastSavedScore();
        case ScoreTarget.BestScore:
            return root.gameplayBestSavedScore();
        default:
            return null;
        }
    }

    function gameplayTargetFraction() : var {
        let player = root.gameplayPlayer(1);
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        return vars ? (vars.targetScoreFraction || 0) : 0;
    }

    function gameplayNextRankTargetFraction(maxPoints: var) : var {
        let max = Math.max(0, Math.floor(maxPoints || 0));
        if (max <= 0) {
            return 0;
        }
        let saved = root.gameplaySavedScorePoints(root.gameplayBestSavedScore());
        return root.nextRankTargetPoints(saved, max) / max;
    }

    function gameplayHighScorePoints() : var {
        root.gameplayScoresRevision;
        return root.gameplayBestSavedScore() ? Math.floor(gameplayBestScoreReplayer.points || 0) : 0;
    }

    function gameplayTargetScorePoints(side: var) : var {
        root.gameplayScoresRevision;
        side = side === 2 ? 2 : 1;
        if (root.battleModeActive()) {
            let opponent = root.gameplayScore(side === 2 ? 1 : 2);
            return opponent ? root.gameplayExScore(opponent) : 0;
        }
        if (root.gameplayTargetSavedScore()) {
            return Math.floor(gameplayTargetScoreReplayer.points || 0);
        }
        let score = root.gameplayScore(1);
        if (!score) {
            return 0;
        }
        let fraction = root.gameplayScoreTargetMode() === ScoreTarget.NextRank
            ? root.gameplayNextRankTargetFraction(score.maxPoints || 0)
            : root.gameplayTargetFraction();
        return Math.floor((score.maxPointsNow || 0) * fraction);
    }

    function gameplayTargetFinalPoints(side: var) : var {
        if (root.battleModeActive()) {
            side = side === 2 ? 2 : 1;
            let opponent = root.gameplayScore(side === 2 ? 1 : 2);
            return opponent ? root.gameplayExScore(opponent) : 0;
        }
        let targetScore = root.gameplayTargetSavedScore();
        if (targetScore) {
            return root.gameplaySavedScorePoints(targetScore);
        }
        let score = root.gameplayScore(1);
        if (!score) {
            return 0;
        }
        if (root.gameplayScoreTargetMode() === ScoreTarget.NextRank) {
            return root.nextRankTargetPoints(
                root.gameplaySavedScorePoints(root.gameplayBestSavedScore()),
                score.maxPoints || 0);
        }
        return Math.floor((score.maxPoints || 0) * root.gameplayTargetFraction());
    }

    function resetGameplayScoreReplayers() : void {
        gameplayTargetScoreReplayer.resetPoints();
        gameplayBestScoreReplayer.resetPoints();
    }

    function updateGameplaySavedScores() : var {
        root.gameplayScoreRequest += 1;
        let request = root.gameplayScoreRequest;
        root.gameplayScores1 = [];
        root.gameplayScoresRevision += 1;
        root.resetGameplayScoreReplayers();

        if (!root.gameplayScreenActive) {
            return;
        }

        let chartData = root.gameplayChartData();
        let player = root.gameplayPlayer(1);
        let scoreDb = player && player.profile ? player.profile.scoreDb : null;
        let md5 = chartData && chartData.md5 ? String(chartData.md5).toUpperCase() : "";
        if (!scoreDb || md5.length === 0) {
            return;
        }

        scoreDb.getScoresForMd5([md5]).then(result => {
            if (request !== root.gameplayScoreRequest) {
                return;
            }
            let scores = result && result.scores
                ? (result.scores[md5] || result.scores[md5.toLowerCase()] || [])
                : [];
            root.gameplayScores1 = scores;
            root.gameplayScoresRevision += 1;
            root.resetGameplayScoreReplayers();
        });
    }

    function notifyGameplayReplayHit(hit: var) : void {
        if (root.gameplayTargetSavedScore()) {
            gameplayTargetScoreReplayer.notifyHit(hit);
        }
        if (root.gameplayBestSavedScore()) {
            gameplayBestScoreReplayer.notifyHit(hit);
        }
    }

    function gameplayJudgementCount(score: var, judgement: var) : var {
        return score && score.judgementCount ? score.judgementCount(judgement) : 0;
    }

    function gameplayPoorCount(score: var) : var {
        return root.gameplayJudgementCount(score, 0) + root.gameplayJudgementCount(score, 1);
    }

    function gameplayLr2Score(score: var) : var {
        let totalNotes = root.gameplayTotalNotes(score);
        if (totalNotes <= 0) {
            return 0;
        }
        let pgreat = root.gameplayJudgementCount(score, 5);
        let great = root.gameplayJudgementCount(score, 4);
        let good = root.gameplayJudgementCount(score, 3);
        return Math.floor((good + (great + pgreat * 2) * 2) * 50000 / totalNotes);
    }

    function gameplayScorePrint(score: var, chartData: var) : var {
        let value = root.gameplayLr2Score(score);
        let keymode = chartData ? chartData.keymode : 0;
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function lr2LinearValueByTime(from: var, to: var, start: var, end: var, now: var) : var {
        if (from === to) {
            return from;
        }
        if (now <= end && start <= now && start < end) {
            let ratio = (now - start) / (end - start);
            return (1.0 - ratio) * from + ratio * to;
        }
        return start < now ? to : from;
    }

    function gameplayScorePrintTargetValue(side: var) : var {
        return root.gameplayScorePrint(root.gameplayScore(side), root.gameplayChartData());
    }

    function gameplayDisplayedScorePrint(side: var) : var {
        let startName = side === 2 ? "gameplayScorePrintStart2" : "gameplayScorePrintStart1";
        let targetName = side === 2 ? "gameplayScorePrintTarget2" : "gameplayScorePrintTarget1";
        let startTimeName = side === 2 ? "gameplayScorePrintStartSkinTime2" : "gameplayScorePrintStartSkinTime1";
        let endTimeName = side === 2 ? "gameplayScorePrintEndSkinTime2" : "gameplayScorePrintEndSkinTime1";
        let start = root[startName];
        let target = root[targetName];
        if (start === target) {
            return target;
        }
        let now = root.renderSkinTime;
        return Math.floor(root.lr2LinearValueByTime(
            start, target, root[startTimeName], root[endTimeName], now));
    }

    function resetGameplayScorePrint() : void {
        let value1 = root.gameplayScorePrintTargetValue(1);
        let value2 = root.gameplayScorePrintTargetValue(2);
        root.gameplayScorePrintStart1 = value1;
        root.gameplayScorePrintTarget1 = value1;
        root.gameplayScorePrintStartSkinTime1 = root.renderSkinTime;
        root.gameplayScorePrintEndSkinTime1 = root.renderSkinTime;
        root.gameplayScorePrintStart2 = value2;
        root.gameplayScorePrintTarget2 = value2;
        root.gameplayScorePrintStartSkinTime2 = root.renderSkinTime;
        root.gameplayScorePrintEndSkinTime2 = root.renderSkinTime;
    }

    function updateGameplayScorePrintTarget(side: var) : var {
        let targetName = side === 2 ? "gameplayScorePrintTarget2" : "gameplayScorePrintTarget1";
        let newTarget = root.gameplayScorePrintTargetValue(side);
        if (newTarget === root[targetName]) {
            return;
        }

        let now = root.renderSkinTime;
        if (side === 2) {
            root.gameplayScorePrintStart2 = root.gameplayDisplayedScorePrint(2);
            root.gameplayScorePrintTarget2 = newTarget;
            root.gameplayScorePrintStartSkinTime2 = now;
            root.gameplayScorePrintEndSkinTime2 = now + 500;
        } else {
            root.gameplayScorePrintStart1 = root.gameplayDisplayedScorePrint(1);
            root.gameplayScorePrintTarget1 = newTarget;
            root.gameplayScorePrintStartSkinTime1 = now;
            root.gameplayScorePrintEndSkinTime1 = now + 500;
        }
    }

    function gameplayCombo(side: var, maxCombo: var) : var {
        let coursePlayer = root.gameplayCoursePlayer(side);
        if (coursePlayer) {
            return maxCombo ? (coursePlayer.maxCombo || 0) : (coursePlayer.combo || 0);
        }
        let score = root.gameplayScore(side);
        return score ? (maxCombo ? (score.maxCombo || 0) : (score.combo || 0)) : 0;
    }

    function gameplayGaugeValue(score: var) : var {
        let gauge = root.gameplayActiveGauge(score);
        return gauge ? (gauge.gauge || 0) : 0;
    }

    function gameplayRateInteger(score: var, currentOnly: var) : var {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        return denominator > 0 ? Math.floor(root.gameplayExScore(score) * 100 / denominator) : 0;
    }

    function gameplayRateDecimal(score: var, currentOnly: var) : var {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        return denominator > 0 ? Math.floor(root.gameplayExScore(score) * 10000 / denominator) % 100 : 0;
    }

    function gameplayRankDelta(score: var) : var {
        let totalNotes = root.gameplayTotalNotes(score);
        let perfectScore = totalNotes * 2;
        let exScore = root.gameplayExScore(score);
        if (totalNotes <= 0 || exScore === perfectScore) {
            return 0;
        }
        let rank = Math.floor(exScore * 9 / perfectScore);
        rank = Math.max(1, Math.min(8, rank));
        return exScore - Math.floor(perfectScore * (rank + 1) / 9);
    }

    function gameplayTimeSeconds(side: var, remaining: var) : var {
        let player = root.gameplayPlayer(side);
        if (!player) {
            return 0;
        }
        let elapsed = Math.floor(Math.max(0, player.elapsed || 0) / 1000000000);
        if (!remaining) {
            return elapsed;
        }
        let length = Math.floor(Math.max(0, player.chartLength || 0) / 1000000000);
        return Math.max(0, length - elapsed);
    }

    function gameplayRawRank(score: var, currentOnly: var) : var {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        if (denominator <= 0) {
            return -1;
        }
        return Math.floor(root.gameplayExScore(score) * 9 / denominator);
    }

    function gameplayRankOption(score: var, baseOption: var, currentOnly: var) : var {
        let rank = root.gameplayRawRank(score, currentOnly);
        if (rank < 0) {
            return -1;
        }
        if (rank >= 8) {
            return baseOption;
        }
        if (rank >= 7) {
            return baseOption + 1;
        }
        if (rank >= 6) {
            return baseOption + 2;
        }
        if (rank >= 5) {
            return baseOption + 3;
        }
        if (rank >= 4) {
            return baseOption + 4;
        }
        if (rank >= 3) {
            return baseOption + 5;
        }
        if (rank >= 2) {
            return baseOption + 6;
        }
        return baseOption + 7;
    }

    function gameplayExactRankOption(score: var, baseOption: var) : var {
        if (!score || root.gameplayExScore(score) <= 0) {
            return baseOption + 8;
        }
        let rank = root.gameplayRawRank(score, false);
        if (rank < 0) {
            return baseOption + 8;
        }
        if (rank >= 8) {
            return baseOption;
        }
        if (rank <= 1) {
            return baseOption + 7;
        }
        return baseOption + (8 - rank);
    }

    function addGameplayGaugeRangeOption(options: var, score: var, baseOption: var) : void {
        let gauge = Math.floor(root.gameplayGaugeValue(score));
        let bucket = gauge >= 100 ? 10 : Math.max(0, Math.floor(gauge / 10));
        root.addOption(options, baseOption + bucket);
    }

    function gameplayGaugeQualified(score: var) : var {
        let gauge = root.gameplayActiveGauge(score);
        return gauge && root.gaugeAboveThreshold(gauge.gauge, gauge.threshold);
    }

    function gameplayGaugeOption(side: var) : var {
        let gauge = root.activeGaugeNameForSide(side);
        if (gauge === "HARD" || gauge === "EXHARD" || gauge === "DAN"
            || gauge === "EXDAN" || gauge === "EXHARDDAN") {
            return 119;
        }
        if (gauge === "EASY" || gauge === "AEASY") {
            return 121;
        }
        if (gauge === "FC" || gauge === "PERFECT" || gauge === "MAX") {
            return 122;
        }
        return 118;
    }

    function gameplayLaneOption(score: var) : var {
        if (!score) {
            return 126;
        }
        switch (score.noteOrderAlgorithm) {
        case NoteOrderAlgorithm.Mirror:
            return 127;
        case NoteOrderAlgorithm.Random:
        case NoteOrderAlgorithm.RandomPlus:
            return 128;
        case NoteOrderAlgorithm.SRandom:
        case NoteOrderAlgorithm.SRandomPlus:
            return 129;
        case NoteOrderAlgorithm.RRandom:
            return 131;
        default:
            return 126;
        }
    }

    function gameplayLaneCoverOption(side: var) : var {
        let hidSud = root.lr2HidSudIndex(side);
        if (hidSud === 3) {
            return 137;
        }
        if (hidSud === 1) {
            return 135;
        }
        if (hidSud === 2) {
            return 136;
        }
        return 134;
    }

    function gameplayJudgementOption(side: var, baseOption: var) : var {
        let judgement = side === 2 ? root.gameplayLastJudgement2 : root.gameplayLastJudgement1;
        return judgement >= 0 && judgement <= 5 ? baseOption + (5 - judgement) : -1;
    }

    function judgementCountForExist(resultOrScore: var, judgement: var) : var {
        if (!resultOrScore) {
            return 0;
        }
        if (resultOrScore.judgementCount) {
            return resultOrScore.judgementCount(judgement);
        }
        return root.resultJudgementCount(resultOrScore, judgement);
    }

    function gameplaySudChanging(side: var) : var {
        let vars = root.generalVarsForSide(side);
        let laneCoverOn = !!vars && !!vars.laneCoverOn;
        if (side === 1) {
            return laneCoverOn
                && (Input.col1sUp || Input.col1sDown
                    || ((!root.battleModeActive() && !root.spToDpActive())
                        && (Input.col2sUp || Input.col2sDown)));
        }
        return Input.col2sUp || Input.col2sDown;
    }

    function gameplayLaneCoverChangingOptionActive() : var {
        return root.gameplayScreenActive
            && (Input.start1 || Input.select1 || Input.start2 || Input.select2);
    }

    function bumpGameplayRevision() : void {
        root.gameplayRevision++;
    }

    function bumpGameplayNumberRevision(side: var) : void {
        if (side === 2) {
            root.gameplayNumberRevision2++;
        } else if (side === 1) {
            root.gameplayNumberRevision1++;
        } else {
            root.gameplayNumberRevision1++;
            root.gameplayNumberRevision2++;
        }
    }

    function requestGameplayRevisionRefresh(side: var) : var {
        if (!root.gameplayScreenActive) {
            return;
        }
        root.gameplayRevisionRefreshPending = true;
        if (side === 1) {
            root.gameplayNumberRevision1Pending = true;
        } else if (side === 2) {
            root.gameplayNumberRevision2Pending = true;
        } else {
            root.gameplayNumberRevision1Pending = true;
            root.gameplayNumberRevision2Pending = true;
        }
        root.requestGameplayRuntimeRefresh();
    }

    function flushGameplayRevisionRefresh() : void {
        root.flushGameplayRuntimeRefresh();
    }

    function bumpGameplayTimerRevision() : void {
        root.gameplayTimerRevision++;
    }

    function requestGameplayTimerRevision() : void {
        root.gameplayTimerRevisionPending = true;
        root.requestGameplayRuntimeRefresh();
    }

    function flushGameplayTimerRevision() : void {
        root.flushGameplayRuntimeRefresh();
    }

    function requestGameplayRuntimeRefresh() : var {
        if (root.gameplayRuntimeRefreshPending) {
            return;
        }
        root.gameplayRuntimeRefreshPending = true;
        Qt.callLater(() => root.flushGameplayRuntimeRefresh());
    }

    function flushGameplayRuntimeRefresh() : var {
        if (!root.gameplayRuntimeRefreshPending
                && !root.gameplayTimerRevisionPending
                && !root.gameplayRevisionRefreshPending
                && !root.gameplayNumberRevision1Pending
                && !root.gameplayNumberRevision2Pending) {
            return;
        }

        let refreshTimers = root.gameplayTimerRevisionPending;
        let refreshGameplay = root.gameplayRevisionRefreshPending;
        let refreshNumbers1 = root.gameplayNumberRevision1Pending;
        let refreshNumbers2 = root.gameplayNumberRevision2Pending;
        root.gameplayRuntimeRefreshPending = false;
        root.gameplayTimerRevisionPending = false;
        root.gameplayRevisionRefreshPending = false;
        root.gameplayNumberRevision1Pending = false;
        root.gameplayNumberRevision2Pending = false;

        if (refreshTimers && skinTiming.skinTimerStateRef.commitGameplayTimerChanges()) {
            root.bumpGameplayTimerRevision();
        }
        if (refreshNumbers1) {
            root.bumpGameplayNumberRevision(1);
        }
        if (refreshNumbers2) {
            root.bumpGameplayNumberRevision(2);
        }
        if (refreshGameplay && root.gameplayScreenActive) {
            root.bumpGameplayRevision();
            root.refreshGameplayRuntimeActiveOptions();
        }
    }

    function updateGameplayStaticNumberRevision() : void {
        root.gameplayStaticNumberRevision++;
    }

    function setGameplayTimerValue(timer: var, skinTime: var) : var {
        if (!timer || skinTime < 0) {
            return;
        }
        if (skinTiming.skinTimerStateRef.setGameplayTimerValue(timer, skinTime)) {
            root.requestGameplayTimerRevision();
        }
    }

    function clearGameplayTimerValue(timer: var) : var {
        if (!timer) {
            return;
        }
        if (skinTiming.skinTimerStateRef.clearGameplayTimerValue(timer)) {
            root.requestGameplayTimerRevision();
        }
    }

    function resetGameplayTimerValues() : void {
        if (skinTiming.skinTimerStateRef.resetGameplayTimerValues()) {
            root.gameplayTimerRevision++;
        }
    }

    function clearHiddenFiveKeyGameplayTimers() : void {
        if (!root.gameplayUsesFiveKeyTimers()) {
            return;
        }
        for (let timer of root.gameplayHiddenFiveKeyTimers) {
            delete root.gameplayHeldButtonTimerStarts[timer];
            delete root.gameplayOffButtonTimerStarts[timer];
            delete root.gameplayPreviousPressedTimers[timer];
            delete root.gameplayHitTimerStarts[timer];
            delete root.gameplayLongNoteTimerStarts[timer];
            root.clearGameplayTimerValue(timer);
        }
    }

    function resetGameplayTimers() : void {
        root.stopLr2GameplayOptionRepeat();
        root.gameplayReadySkinTime = -1;
        root.gameplayStartSkinTime = -1;
        root.gameplayGaugeUpSkinTime1 = -1;
        root.gameplayGaugeUpSkinTime2 = -1;
        root.gameplayGaugeDownSkinTime1 = -1;
        root.gameplayGaugeDownSkinTime2 = -1;
        root.gameplayJudgeSkinTime1 = -1;
        root.gameplayJudgeSkinTime2 = -1;
        root.gameplayLastJudgement1 = -1;
        root.gameplayLastJudgement2 = -1;
        root.gameplayJudgeCombo1 = 0;
        root.gameplayJudgeCombo2 = 0;
        root.gameplayJudgeRevision1 = 0;
        root.gameplayJudgeRevision2 = 0;
        root.gameplayJudgeTimingCounts1 = root.emptyJudgeTimingCounts();
        root.gameplayJudgeTimingCounts2 = root.emptyJudgeTimingCounts();
        root.gameplayLastJudgeTiming1 = 0;
        root.gameplayLastJudgeTiming2 = 0;
        root.gameplayJudgeLaneValues1 = root.emptyJudgeLaneValues();
        root.gameplayJudgeLaneValues2 = root.emptyJudgeLaneValues();
        root.gameplayJudgeNowValue1 = 0;
        root.gameplayJudgeNowValue2 = 0;
        root.gameplayLastMissSkinTime1 = -1;
        root.gameplayLastMissSkinTime2 = -1;
        root.gameplayFullComboSkinTime1 = -1;
        root.gameplayFullComboSkinTime2 = -1;
        root.gameplayPreviousGauge1 = Math.floor(root.gameplayGaugeValue(root.gameplayScore(1)));
        root.gameplayPreviousGauge2 = Math.floor(root.gameplayGaugeValue(root.gameplayScore(2)));
        root.gameplayPreviousCombo1 = root.gameplayScore(1) ? (root.gameplayScore(1).combo || 0) : 0;
        root.gameplayPreviousCombo2 = root.gameplayScore(2) ? (root.gameplayScore(2).combo || 0) : 0;
        root.resetGameplayScorePrint();
        root.gameplayHeldButtonTimerStarts = ({});
        root.gameplayOffButtonTimerStarts = root.initialGameplayOffButtonTimers();
        root.gameplayPreviousPressedTimers = ({});
        root.gameplayHitTimerStarts = ({});
        root.gameplayLongNoteTimerStarts = ({});
        root.resetGameplayTimerValues();
        root.clearHiddenFiveKeyGameplayTimers();
    }

    function updateGameplayStatusTimers() : var {
        if (!root.gameplayScreenActive || !root.chart || root.chart.status === undefined) {
            return;
        }
        let optionsChanged = false;
        let isReady = root.gameplayStatusAtLeast(ChartRunner.Ready);
        let isRunning = root.gameplayStatusAtLeast(ChartRunner.Running);
        if (isReady && root.gameplayReadySkinTime < 0) {
            let loadEndTime = Math.max(0, skinModel.loadEnd || 0);
            if (isRunning || root.renderSkinTime >= loadEndTime) {
                root.gameplayReadySkinTime = root.renderSkinTime;
                root.setGameplayTimerValue(40, root.gameplayReadySkinTime);
                optionsChanged = true;
            }
        }
        if (isRunning && root.gameplayStartSkinTime < 0) {
            if (root.gameplayReadySkinTime < 0) {
                root.gameplayReadySkinTime = root.renderSkinTime;
                root.setGameplayTimerValue(40, root.gameplayReadySkinTime);
                optionsChanged = true;
            }
            root.gameplayStartSkinTime = root.renderSkinTime;
            root.setGameplayTimerValue(41, root.gameplayStartSkinTime);
        }
        if (optionsChanged) {
            root.refreshGameplayRuntimeActiveOptions();
        }
    }

    function gameplayLr2LaneForHit(side: var, hit: var) : var {
        if (!hit || hit.column === undefined) {
            return -1;
        }
        let column = hit.column;
        if (side === 2 && root.gameplayPlayer(2)) {
            if (column === 7) {
                return 10;
            }
            let lane = column >= 0 && column <= 6 ? column + 11 : -1;
            return root.gameplayLr2LaneAllowedForTimers(lane) ? lane : -1;
        }
        let rightSide = side === 2 || (!root.gameplayPlayer(2) && column >= 8);
        if (rightSide) {
            if (column === 15) {
                return 10;
            }
            let lane = column >= 8 && column <= 14 ? column + 3 : -1;
            return root.gameplayLr2LaneAllowedForTimers(lane) ? lane : -1;
        }
        if (column === 7) {
            return 0;
        }
        let lane = column >= 0 && column <= 6 ? column + 1 : -1;
        return root.gameplayLr2LaneAllowedForTimers(lane) ? lane : -1;
    }

    function gameplayHitDisplaySide(scoreSide: var, hit: var) : var {
        if (scoreSide === 2) {
            return 2;
        }
        return root.gameplayLr2LaneForHit(scoreSide, hit) >= 10 ? 2 : 1;
    }

    function gameplayNoteForHit(side: var, hit: var) : var {
        if (!hit || hit.noteIndex === undefined || hit.noteIndex < 0) {
            return null;
        }
        let player = root.gameplayLanePlayer(side === 2 ? 2 : 1);
        let notes = player && player.notes && player.notes.notes
            && hit.column >= 0 && hit.column < player.notes.notes.length
                ? player.notes.notes[hit.column]
                : null;
        return notes && hit.noteIndex < notes.length ? notes[hit.noteIndex] : null;
    }

    function updateGameplayHitEffectTimers(side: var, hit: var) : var {
        if (!hit) {
            return;
        }
        let hitNote = root.gameplayNoteForHit(side, hit);
        let judgement = root.gameplayJudgementFromHit(hit);
        let startsLongNote = hit.action === hitEvent.Press
            && hitNote
            && hitNote.type === note.Type.LongNoteBegin;
        let endsLongNote = hit.action === hitEvent.Release
            && hitNote
            && hitNote.type === note.Type.LongNoteEnd;
        let judgedTap = hit.action === hitEvent.Press
            && !!hitNote
            && hit.noteRemoved
            && judgement >= Judgement.Bad
            && judgement <= Judgement.Perfect;
        if (!startsLongNote && !endsLongNote && !judgedTap) {
            return;
        }
        let lane = root.gameplayLr2LaneForHit(side, hit);
        if (lane < 0) {
            return;
        }

        let hitTimer = 50 + lane;
        let longNoteTimer = 70 + lane;

        if (hit.action === hitEvent.Press) {
            if (hitNote && hitNote.type === note.Type.LongNoteBegin) {
                root.gameplayLongNoteTimerStarts[longNoteTimer] = root.renderSkinTime;
                root.setGameplayTimerValue(longNoteTimer, root.renderSkinTime);
            } else {
                root.gameplayHitTimerStarts[hitTimer] = root.renderSkinTime;
                root.setGameplayTimerValue(hitTimer, root.renderSkinTime);
            }
        } else if (hitNote && hitNote.type === note.Type.LongNoteEnd) {
            delete root.gameplayLongNoteTimerStarts[longNoteTimer];
            root.clearGameplayTimerValue(longNoteTimer);
            root.gameplayHitTimerStarts[hitTimer] = root.renderSkinTime;
            root.setGameplayTimerValue(hitTimer, root.renderSkinTime);
        }
    }

    function gameplayJudgementFromHit(hit: var) : var {
        return hit && hit.points && hit.points.judgement !== undefined
            ? hit.points.judgement
            : -1;
    }

    function gameplayJudgeComboForHit(scoreSide: var, judgement: var) : var {
        return judgement >= Judgement.Good && judgement <= Judgement.Perfect
            ? root.gameplayCombo(scoreSide, false)
            : 0;
    }

    function updateGameplayHitTimers(displaySide: var, hit: var, scoreSide: var) : var {
        scoreSide = scoreSide || displaySide;
        let score = root.gameplayScore(scoreSide);
        if (!score) {
            return;
        }
        root.updateGameplayHitEffectTimers(displaySide, hit);
        let judgement = root.gameplayJudgementFromHit(hit);
        root.recordGameplayJudgeTiming(scoreSide, hit);

        let currentGauge = Math.floor(root.gameplayGaugeValue(score));
        let previousGaugeName = scoreSide === 2 ? "gameplayPreviousGauge2" : "gameplayPreviousGauge1";
        let previousGauge = root[previousGaugeName];
        if (previousGauge >= 0 && currentGauge > previousGauge) {
            if (scoreSide === 2) {
                root.gameplayGaugeUpSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(43, root.gameplayGaugeUpSkinTime2);
            } else {
                root.gameplayGaugeUpSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(42, root.gameplayGaugeUpSkinTime1);
            }
        } else if (previousGauge >= 0 && currentGauge < previousGauge) {
            if (scoreSide === 2) {
                root.gameplayGaugeDownSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(45, root.gameplayGaugeDownSkinTime2);
            } else {
                root.gameplayGaugeDownSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(44, root.gameplayGaugeDownSkinTime1);
            }
        }
        root[previousGaugeName] = currentGauge;

        if (judgement >= 0 && judgement <= Judgement.Perfect) {
            let displayCombo = root.gameplayJudgeComboForHit(scoreSide, judgement);
            if (displaySide === 2) {
                root.gameplayJudgeSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(47, root.gameplayJudgeSkinTime2);
                if (displayCombo > 0) {
                    root.setGameplayTimerValue(447, root.gameplayJudgeSkinTime2);
                }
                root.gameplayLastJudgement2 = judgement;
                root.gameplayJudgeCombo2 = displayCombo;
                root.gameplayJudgeRevision2++;
            } else {
                root.gameplayJudgeSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(46, root.gameplayJudgeSkinTime1);
                if (displayCombo > 0) {
                    root.setGameplayTimerValue(446, root.gameplayJudgeSkinTime1);
                }
                root.gameplayLastJudgement1 = judgement;
                root.gameplayJudgeCombo1 = displayCombo;
                root.gameplayJudgeRevision1++;
            }
        }
        if (judgement >= 0 && judgement <= Judgement.Bad) {
            if (displaySide === 2) {
                root.gameplayLastMissSkinTime2 = root.renderSkinTime;
            } else {
                root.gameplayLastMissSkinTime1 = root.renderSkinTime;
            }
            skinTiming.gameplayPoorBgaOptionTimer.restart();
        }

        let totalNotes = root.gameplayTotalNotes(score);
        let currentChartCombo = score ? (score.combo || 0) : 0;
        let previousComboName = scoreSide === 2 ? "gameplayPreviousCombo2" : "gameplayPreviousCombo1";
        let previousChartCombo = root[previousComboName] || 0;
        if (hit && hit.noteRemoved
                && totalNotes > 0
                && currentChartCombo >= totalNotes
                && previousChartCombo < totalNotes) {
            if (scoreSide === 2) {
                root.gameplayFullComboSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(49, root.gameplayFullComboSkinTime2);
            } else {
                root.gameplayFullComboSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(48, root.gameplayFullComboSkinTime1);
            }
        }
        root[previousComboName] = currentChartCombo;
    }

    function addGameplayTimer(result: var, timer: var, skinTime: var) : void {
        if (skinTime >= 0) {
            result[timer] = skinTime;
        }
    }

    function resetGameplayFrameSamples() : void {
        gameplayFrameState.reset();
    }

    function refreshGameplayFrameSamples(frameSkinTime: var) : void {
        let sampleSkinTime = frameSkinTime === undefined ? root.renderSkinTime : frameSkinTime;
        gameplayFrameState.refresh(sampleSkinTime);
    }

    function addGameplayTimers(result: var) : var {
        if (!root.gameplayScreenActive) {
            return;
        }
        root.gameplayTimerRevision;
        root.addGameplayTimer(result, 40, root.gameplayReadySkinTime);
        root.addGameplayTimer(result, 41, root.gameplayStartSkinTime);
        root.addGameplayTimer(result, 42, root.gameplayGaugeUpSkinTime1);
        root.addGameplayTimer(result, 43, root.gameplayGaugeUpSkinTime2);
        root.addGameplayTimer(result, 44, root.gameplayGaugeDownSkinTime1);
        root.addGameplayTimer(result, 45, root.gameplayGaugeDownSkinTime2);
        root.addGameplayTimer(result, 46, root.gameplayJudgeSkinTime1);
        root.addGameplayTimer(result, 47, root.gameplayJudgeSkinTime2);
        if (root.gameplayJudgeCombo1 > 0) {
            root.addGameplayTimer(result, 446, root.gameplayJudgeSkinTime1);
        }
        if (root.gameplayJudgeCombo2 > 0) {
            root.addGameplayTimer(result, 447, root.gameplayJudgeSkinTime2);
        }
        root.addGameplayTimer(result, 48, root.gameplayFullComboSkinTime1);
        root.addGameplayTimer(result, 49, root.gameplayFullComboSkinTime2);
        root.addGameplayTimer(result, 140, root.gameplayRhythmTimerSkinTime);
        root.addGameplayKeyTimers(result);
        root.addGameplayEffectTimers(result);
    }

    function addGameplayEffectTimers(result: var) : void {
        for (let keyName in root.gameplayHitTimerStarts) {
            if (root.gameplayHitTimerAllowed(Number(keyName))) {
                result[keyName] = root.gameplayHitTimerStarts[keyName];
            }
        }
        for (let keyName in root.gameplayLongNoteTimerStarts) {
            if (root.gameplayLongNoteTimerAllowed(Number(keyName))) {
                result[keyName] = root.gameplayLongNoteTimerStarts[keyName];
            }
        }
    }

    function initialGameplayOffButtonTimers() : var {
        let result = {};
        for (let timer of root.gameplayKeyTimers) {
            result[timer + 20] = 0;
        }
        return result;
    }

    function gameplayKeyOnTimerForKey(key: var) : var {
        switch (key) {
        case BmsKey.Col1sUp:
        case BmsKey.Col1sDown:
            return 100;
        case BmsKey.Col11:
            return 101;
        case BmsKey.Col12:
            return 102;
        case BmsKey.Col13:
            return 103;
        case BmsKey.Col14:
            return 104;
        case BmsKey.Col15:
            return 105;
        case BmsKey.Col16:
            return root.gameplayUsesFiveKeyTimers() ? 0 : 106;
        case BmsKey.Col17:
            return root.gameplayUsesFiveKeyTimers() ? 0 : 107;
        case BmsKey.Col2sUp:
        case BmsKey.Col2sDown:
            return 110;
        case BmsKey.Col21:
            return 111;
        case BmsKey.Col22:
            return 112;
        case BmsKey.Col23:
            return 113;
        case BmsKey.Col24:
            return 114;
        case BmsKey.Col25:
            return 115;
        case BmsKey.Col26:
            return root.gameplayUsesFiveKeyTimers() ? 0 : 116;
        case BmsKey.Col27:
            return root.gameplayUsesFiveKeyTimers() ? 0 : 117;
        default:
            return 0;
        }
    }

    function gameplayKeyOffTimerForOnTimer(timer: var) : var {
        return root.gameplayKeyOnTimerAllowed(timer) ? timer + 20 : 0;
    }

    function gameplayKeyTimerHeld(timer: var) : var {
        let columnState = root.gameplayColumnStateForKeyTimer(timer);
        return !!columnState && !!columnState.pressed;
    }

    function setGameplayKeyTimerPressed(timer: var, pressed: var) : var {
        if (!root.gameplayScreenActive || !root.chart) {
            return;
        }
        if (!root.gameplayKeyOnTimerAllowed(timer)) {
            let blockedOffTimer = timer ? timer + 20 : 0;
            delete root.gameplayHeldButtonTimerStarts[timer];
            delete root.gameplayPreviousPressedTimers[timer];
            if (blockedOffTimer) {
                delete root.gameplayOffButtonTimerStarts[blockedOffTimer];
                root.clearGameplayTimerValue(blockedOffTimer);
            }
            root.clearGameplayTimerValue(timer);
            return;
        }
        let wasPressed = !!root.gameplayPreviousPressedTimers[timer];
        if (pressed === wasPressed) {
            return;
        }

        let offTimer = root.gameplayKeyOffTimerForOnTimer(timer);
        if (!offTimer) {
            return;
        }

        if (pressed) {
            if (root.gameplayHeldButtonTimerStarts[timer] === undefined) {
                root.gameplayHeldButtonTimerStarts[timer] = root.renderSkinTime;
            }
            delete root.gameplayOffButtonTimerStarts[offTimer];
            root.setGameplayTimerValue(timer, root.gameplayHeldButtonTimerStarts[timer]);
            root.clearGameplayTimerValue(offTimer);
        } else {
            delete root.gameplayHeldButtonTimerStarts[timer];
            if (root.gameplayOffButtonTimerStarts[offTimer] === undefined) {
                root.gameplayOffButtonTimerStarts[offTimer] = root.renderSkinTime;
            }
            root.clearGameplayTimerValue(timer);
            root.setGameplayTimerValue(offTimer, root.gameplayOffButtonTimerStarts[offTimer]);
        }

        root.gameplayPreviousPressedTimers[timer] = pressed;
    }

    function syncGameplayKeyTimerFromColumn(timer: var) : void {
        root.setGameplayKeyTimerPressed(timer, root.gameplayKeyTimerHeld(timer));
    }

    function syncGameplayKeyTimerFromColumnState(timer: var, columnState: var) : void {
        root.setGameplayKeyTimerPressed(timer, !!columnState && !!columnState.pressed);
    }

    function setGameplayLongNoteTimerHeld(timer: var, held: var) : var {
        if (!root.gameplayScreenActive || !root.chart) {
            return;
        }
        if (!root.gameplayLongNoteTimerAllowed(timer)) {
            delete root.gameplayLongNoteTimerStarts[timer];
            root.clearGameplayTimerValue(timer);
            return;
        }

        let wasHeld = root.gameplayLongNoteTimerStarts[timer] !== undefined;
        if (held === wasHeld) {
            return;
        }

        if (held) {
            root.gameplayLongNoteTimerStarts[timer] = root.renderSkinTime;
            root.setGameplayTimerValue(timer, root.renderSkinTime);
        } else {
            delete root.gameplayLongNoteTimerStarts[timer];
            root.clearGameplayTimerValue(timer);
        }
    }

    function syncGameplayLongNoteTimerFromColumnState(timer: var, columnState: var) : void {
        root.setGameplayLongNoteTimerHeld(timer, !!columnState && !!columnState.holdingLongNote);
    }

    function gameplayColumnStateForLr2Lane(lane: var) : var {
        if (lane < 0) {
            return null;
        }

        let player = root.gameplayLanePlayer(lane >= 10 ? 2 : 1);
        let engineColumn = root.gameplayEngineColumnForLr2Lane(lane);
        return player
            && player.state
            && player.state.columnStates
            && engineColumn >= 0
            && engineColumn < player.state.columnStates.length
                ? player.state.columnStates[engineColumn]
                : null;
    }

    function gameplayColumnStateForKeyTimer(timer: var) : var {
        return root.gameplayColumnStateForLr2Lane(root.gameplayLr2LaneForKeyTimer(timer));
    }

    function gameplayColumnStateForLongNoteTimer(timer: var) : var {
        return root.gameplayColumnStateForLr2Lane(root.gameplayLr2LaneForLongNoteTimer(timer));
    }

    function syncGameplayKeyTimersFromColumns() : var {
        if (!root.gameplayScreenActive || !root.chart) {
            return;
        }

        for (let timer of root.gameplayKeyTimers) {
            root.syncGameplayKeyTimerFromColumnState(timer, root.gameplayColumnStateForKeyTimer(timer));
        }
    }

    function syncGameplayLongNoteTimersFromColumns() : var {
        if (!root.gameplayScreenActive || !root.chart) {
            return;
        }

        for (let timer of root.gameplayLongNoteTimers) {
            root.syncGameplayLongNoteTimerFromColumnState(
                timer,
                root.gameplayColumnStateForLongNoteTimer(timer));
        }
    }

    function pressGameplayButtonTimer(key: var) : var {
        if (!root.gameplayScreenActive) {
            return;
        }
        let onTimer = root.gameplayKeyOnTimerForKey(key);
        let offTimer = root.gameplayKeyOffTimerForOnTimer(onTimer);
        if (!onTimer || !offTimer) {
            return;
        }

        root.gameplayHeldButtonTimerStarts[onTimer] = root.renderSkinTime;
        root.gameplayPreviousPressedTimers[onTimer] = true;
        delete root.gameplayOffButtonTimerStarts[offTimer];
        root.setGameplayTimerValue(onTimer, root.renderSkinTime);
        root.clearGameplayTimerValue(offTimer);
    }

    function releaseGameplayButtonTimer(key: var) : var {
        let onTimer = root.gameplayKeyOnTimerForKey(key);
        let offTimer = root.gameplayKeyOffTimerForOnTimer(onTimer);
        if (!onTimer || !offTimer) {
            return;
        }

        delete root.gameplayHeldButtonTimerStarts[onTimer];
        root.gameplayPreviousPressedTimers[onTimer] = false;
        root.gameplayOffButtonTimerStarts[offTimer] = root.renderSkinTime;
        root.clearGameplayTimerValue(onTimer);
        root.setGameplayTimerValue(offTimer, root.renderSkinTime);
    }

    function addGameplayKeyTimers(result: var) : void {
        for (let keyName in root.gameplayHeldButtonTimerStarts) {
            if (root.gameplayKeyOnTimerAllowed(Number(keyName))) {
                result[keyName] = root.gameplayHeldButtonTimerStarts[keyName];
            }
        }
        for (let keyName in root.gameplayOffButtonTimerStarts) {
            let offTimer = Number(keyName);
            let onTimer = offTimer - 20;
            if (root.gameplayKeyOnTimerAllowed(onTimer) && !root.gameplayPreviousPressedTimers[onTimer]) {
                result[keyName] = root.gameplayOffButtonTimerStarts[keyName];
            }
        }
    }

    Lr2RuntimeOptionBuilder {
        id: runtimeOptions
        host: root
        selectContext: selectContext
        skinModel: skinModel
        rankingState: lr2Ranking
    }

    readonly property var builtBarRuntimeActiveOptions: runtimeOptions.buildBarActiveOptions()
    readonly property var builtBaseRuntimeActiveOptions: runtimeOptions.buildBaseActiveOptions(root.builtBarRuntimeActiveOptions)
    readonly property var builtSelectCommonRuntimeActiveOptions: runtimeOptions.buildSelectCommonActiveOptions(root.builtBaseRuntimeActiveOptions)
    readonly property var builtSelectRequiredRuntimeActiveOptions: runtimeOptions.buildSelectRequiredRuntimeActiveOptions()
    property var builtSelectRuntimeActiveOptions: []
    property var builtSelectDetailRuntimeActiveOptions: []
    readonly property var builtScreenRuntimeActiveOptions: root.effectiveScreenKey === "select"
        ? root.emptyActiveOptions
        : (root.gameplayScreenActive
            ? root.builtGameplayRuntimeActiveOptions
            : runtimeOptions.buildRuntimeActiveOptions(root.builtBaseRuntimeActiveOptions))

    property alias barActiveOptions: selectUpdateController.barActiveOptions
    property alias baseActiveOptions: selectUpdateController.baseActiveOptions
    property alias selectCommonActiveOptions: selectUpdateController.selectCommonActiveOptions
    property alias selectCommonActiveOptionsReady: selectUpdateController.selectCommonActiveOptionsReady
    property var gameplayRuntimeActiveOptionsStateParts: []
    property var builtGameplayRuntimeActiveOptions: root.emptyActiveOptions
    property alias runtimeActiveOptions: selectUpdateController.runtimeActiveOptions
    property bool selectRuntimeActiveOptionsRefreshQueued: false
    property bool selectRuntimeActiveOptionsStateReady: false
    property var selectRuntimeActiveOptionsStateParts: []
    readonly property var barTimers: ({ "0": 0 })

    function sameArrayValues(a: var, b: var) : var {
        if (a === b) {
            return true;
        }
        if (!a || !b || a.length !== b.length) {
            return false;
        }
        for (let i = 0; i < a.length; ++i) {
            if (a[i] !== b[i]) {
                return false;
            }
        }
        return true;
    }

    function sameNumberArray(a: var, b: var) : var {
        return root.sameArrayValues(a, b);
    }

    function skinUsesRuntimeOption(option: var) : var {
        return !root.usedOptionFilterActive
            || root.usedOptionLookup[Math.abs(option)] === true;
    }

    function skinUsesRuntimeOptionRange(first: var, last: var) : var {
        if (!root.usedOptionFilterActive) {
            return true;
        }
        let lookup = root.usedOptionLookup;
        for (let option = first; option <= last; ++option) {
            if (lookup[option] === true) {
                return true;
            }
        }
        return false;
    }

    function appendActiveOptionsState(parts: var, activeOptions: var) : void {
        let count = activeOptions ? activeOptions.length || 0 : 0;
        parts.push(count);
        for (let i = 0; i < count; ++i) {
            parts.push(activeOptions[i] || 0);
        }
    }

    function activeOptionPresent(option: var, activeOptions: var) : var {
        if (!activeOptions || option === undefined || option === null) {
            return false;
        }
        let lookup = activeOptions.__lookup;
        return lookup
            ? lookup[option] === true
            : activeOptions.indexOf(option) !== -1;
    }

    function activeOptionsForDsts(dsts: var, activeOptions: var) : var {
        if (!dsts || dsts.length === 0 || !dsts[0] || !activeOptions) {
            return root.emptyActiveOptions;
        }

        let first = dsts[0];
        let op1 = first.op1 || 0;
        let op2 = first.op2 || 0;
        let op3 = first.op3 || 0;
        if (op1 === 0 && op2 === 0 && op3 === 0) {
            return root.emptyActiveOptions;
        }

        let count = 0;
        let idA = 0;
        let idB = 0;
        let idC = 0;
        if (op1 !== 0) {
            let id1 = Math.abs(op1);
            let present1 = root.activeOptionPresent(id1, activeOptions);
            if ((op1 > 0 && !present1) || (op1 < 0 && present1)) {
                return root.emptyActiveOptions;
            }
            if (op1 > 0) {
                idA = id1;
                count = 1;
            }
        }
        if (op2 !== 0) {
            let id2 = Math.abs(op2);
            let present2 = root.activeOptionPresent(id2, activeOptions);
            if ((op2 > 0 && !present2) || (op2 < 0 && present2)) {
                return root.emptyActiveOptions;
            }
            if (op2 > 0 && (count === 0 || id2 !== idA)) {
                if (count === 0) {
                    idA = id2;
                } else {
                    idB = id2;
                }
                ++count;
            }
        }
        if (op3 !== 0) {
            let id3 = Math.abs(op3);
            let present3 = root.activeOptionPresent(id3, activeOptions);
            if ((op3 > 0 && !present3) || (op3 < 0 && present3)) {
                return root.emptyActiveOptions;
            }
            if (op3 > 0 && (count === 0 || id3 !== idA)
                    && (count < 2 || id3 !== idB)) {
                if (count === 0) {
                    idA = id3;
                } else if (count === 1) {
                    idB = id3;
                } else {
                    idC = id3;
                }
                ++count;
            }
        }
        if (count === 0) {
            return root.emptyActiveOptions;
        }
        if (count > 1 && idB < idA) {
            let swapAB = idA;
            idA = idB;
            idB = swapAB;
        }
        if (count > 2) {
            if (idC < idB) {
                let swapBC = idB;
                idB = idC;
                idC = swapBC;
            }
            if (idB < idA) {
                let swapAB2 = idA;
                idA = idB;
                idB = swapAB2;
            }
        }
        let ids = count === 1 ? [idA] : (count === 2 ? [idA, idB] : [idA, idB, idC]);
        root.finalizeOptionList(ids);
        return ids;
    }

    function refreshSelectPlayOptionLayout() : void {
        if (root.effectiveScreenKey !== "select") {
            return;
        }
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }

    function updateBuiltSelectRuntimeActiveOptions() : void {
        let nextOptions = root.effectiveScreenKey === "select"
            ? runtimeOptions.buildSelectGeneratedRuntimeActiveOptions()
            : [];
        if (!root.sameArrayValues(nextOptions, root.builtSelectRuntimeActiveOptions)) {
            root.builtSelectRuntimeActiveOptions = nextOptions;
        }
    }

    function updateBuiltSelectDetailRuntimeActiveOptions() : void {
        let nextOptions = root.effectiveScreenKey === "select"
            ? runtimeOptions.buildSelectDetailRuntimeActiveOptions()
            : [];
        if (!root.sameArrayValues(nextOptions, root.builtSelectDetailRuntimeActiveOptions)) {
            root.builtSelectDetailRuntimeActiveOptions = nextOptions;
        }
    }

    function selectRuntimeOptionStateParts() : var {
        if (root.effectiveScreenKey !== "select") {
            return [root.effectiveScreenKey];
        }

        let state = selectContext.selectedState;
        let stateCurrent = state && selectContext.selectedStateCurrent;
        return [
            root.effectiveScreenKey,
            root.usedOptionFilterActive ? 1 : 0,
            root.usedOptionLookup,
            root.lr2SkinUsesBeatorajaSemantics ? 1 : 0,
            root.selectReplayOptionsUsed ? 1 : 0,
            root.selectScoreOptionIdsUsed ? 1 : 0,
            root.selectEntryStatusOptionsUsed ? 1 : 0,
            root.selectDifficultyBarOptionsUsed ? 1 : 0,
            root.selectDifficultyLampOptionsUsed ? 1 : 0,
            root.selectDifficultyStateUsed ? 1 : 0,
            root.selectCourseDetailOptionsUsed ? 1 : 0,
            root.selectRankingStatusOptionsUsed ? 1 : 0,
            root.battleModeActive() ? 1 : 0,
            root.spToDpActive() ? 1 : 0,
            root.tableInfoRevision,
            selectContext.scoreGeneration,
            selectContext.listGeneration,
            selectContext.selectedDetailValueRevision,
            selectContext.focusedSelectionKey,
            selectContext.focusedSelectionTargetKey,
            selectContext.focusedItem,
            selectContext.focusedChartData,
            selectContext.focusedSelectionTarget,
            stateCurrent ? 1 : 0,
            state ? state.item : null,
            state ? state.chartData : null,
            state ? state.scoreOptionIds : null,
            selectContext.rankingMode ? 1 : 0,
            selectContext.rankingBaseItem,
            lr2Ranking.currentStatusOption,
            lr2Ranking.currentPlayerCount
        ];
    }

    function refreshSelectRuntimeActiveOptions() : var {
        root.selectRuntimeActiveOptionsRefreshQueued = false;
        let nextState = root.selectRuntimeOptionStateParts();
        if (!root.selectRuntimeActiveOptionsStateReady
                || !root.sameArrayValues(nextState, root.selectRuntimeActiveOptionsStateParts)) {
            root.selectRuntimeActiveOptionsStateReady = true;
            root.selectRuntimeActiveOptionsStateParts = nextState;
            root.updateBuiltSelectRuntimeActiveOptions();
            root.updateBuiltSelectDetailRuntimeActiveOptions();
            selectUpdateController.selectRuntimeGeneratedActiveOptions = root.builtSelectRuntimeActiveOptions;
            selectUpdateController.selectDetailRuntimeActiveOptions = root.builtSelectDetailRuntimeActiveOptions;
        }
        return selectUpdateController.refreshSelectRuntimeActiveOptions();
    }

    function flushQueuedSelectRuntimeActiveOptionsRefresh() : void {
        if (!root.selectRuntimeActiveOptionsRefreshQueued) {
            return;
        }
        root.refreshSelectRuntimeActiveOptions();
    }

    function queueSelectRuntimeActiveOptionsRefresh() : void {
        if (root.selectRuntimeActiveOptionsRefreshQueued) {
            return;
        }
        root.selectRuntimeActiveOptionsRefreshQueued = true;
        Qt.callLater(root.flushQueuedSelectRuntimeActiveOptionsRefresh);
    }

    function appendGameplayRuntimeOptionSideState(parts: var, side: var) : void {
        let score = root.gameplayScore(side);
        parts.push(side);
        if (root.skinUsesRuntimeOptionRange(118, 122)) {
            parts.push(root.gameplayGaugeOption(side));
        }
        if (root.skinUsesRuntimeOptionRange(126, 131)) {
            parts.push(root.gameplayLaneOption(score));
        }
        if (root.skinUsesRuntimeOptionRange(134, 137)) {
            parts.push(root.gameplayLaneCoverOption(side));
        }
        if (side === 1 && root.skinUsesRuntimeOptionRange(271, 273)) {
            let vars = root.generalVarsForSide(side);
            parts.push(
                vars && vars.laneCoverOn ? 1 : 0,
                vars && vars.liftOn ? 1 : 0,
                vars && vars.hiddenOn ? 1 : 0);
        }
        if (root.skinUsesRuntimeOptionRange(side === 2 ? 210 : 200, side === 2 ? 217 : 207)) {
            parts.push(root.gameplayRankOption(score, side === 2 ? 210 : 200, true));
        }
        if (side === 1 && root.skinUsesRuntimeOptionRange(220, 227)) {
            parts.push(root.gameplayRankOption(score, 220, false));
        }
        if (root.skinUsesRuntimeOptionRange(side === 2 ? 310 : 300, side === 2 ? 318 : 308)) {
            parts.push(root.gameplayExactRankOption(score, side === 2 ? 310 : 300));
        }
        if (side === 1 && root.skinUsesRuntimeOption(1240)) {
            parts.push(root.gameplayGaugeQualified(score) ? 1 : 0);
        }
        if (root.skinUsesRuntimeOptionRange(side === 2 ? 250 : 230, side === 2 ? 260 : 240)) {
            let gaugeValue = Math.floor(root.gameplayGaugeValue(score));
            parts.push(gaugeValue >= 100 ? 10 : Math.max(0, Math.floor(gaugeValue / 10)));
        }
        let judgementFirst = side === 2 ? 261 : 241;
        let judgementOption = -1;
        if (root.skinUsesRuntimeOptionRange(judgementFirst, judgementFirst + 5)) {
            judgementOption = root.gameplayJudgementOption(side, judgementFirst);
            parts.push(judgementOption);
        }
        let timingFirst = side === 2 ? 1262 : 1242;
        if (root.skinUsesRuntimeOptionRange(timingFirst, timingFirst + 1)) {
            if (judgementOption < 0) {
                judgementOption = root.gameplayJudgementOption(side, judgementFirst);
            }
            let timing = side === 2 ? root.gameplayLastJudgeTiming2 : root.gameplayLastJudgeTiming1;
            parts.push(timing !== 0 && judgementOption >= 0 && judgementOption !== judgementFirst
                ? (timing > 0 ? 1 : -1)
                : 0);
        }
        if (root.skinUsesRuntimeOptionRange(side === 2 ? 267 : 247, side === 2 ? 268 : 248)) {
            parts.push(side === 2 ? root.gameplayPoorBgaOption2 : root.gameplayPoorBgaOption1);
        }
    }

    function gameplayRuntimeOptionStateParts() : var {
        if (!root.gameplayScreenActive) {
            return root.emptyActiveOptions;
        }

        let vars = root.mainGeneralVarsRef;
        let score1 = root.gameplayScore(1);
        let chartData = root.gameplayChartData();
        let chartCount = root.chart && root.chart.chartDatas ? root.chart.chartDatas.length || 0 : 0;
        let chartIndex = root.chart && root.chart.currentChartIndex !== undefined
            ? Math.max(0, root.chart.currentChartIndex || 0)
            : 0;
        let parts = [
            root.effectiveScreenKey
        ];
        root.appendActiveOptionsState(parts, root.baseActiveOptions);
        parts.push(
            vars ? vars.bgaSize || 0 : 0,
            vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0,
            vars && vars.scoreGraphEnabled === false ? 0 : 1,
            vars ? Math.max(0, Math.min(2, vars.bgaMode || 0)) : 1,
            root.gameplayAutoplayActive() ? 1 : 0,
            root.gaugeColorOption(1),
            root.gaugeColorOption(2),
            root.activeGaugeNameForSide(1),
            root.activeGaugeNameForSide(2),
            root.gameplayGaugeTrophyOption(1),
            root.gameplayGaugeTrophyOption(2),
            root.isLoggedIn() ? 1 : 0,
            root.clearStatusOption(),
            root.gameplayReadySkinTime >= 0 ? 1 : 0,
            root.gameplayReplayActive() ? 1 : 0,
            root.battleModeActive() ? 1 : 0,
            root.spToDpActive() ? 1 : 0,
            chartIndex,
            chartCount,
            root.gameplayKeymode(),
            chartData && chartData.stageFile ? 1 : 0,
            chartData && chartData.banner ? 1 : 0,
            chartData && chartData.backBmp ? 1 : 0,
            chartData && selectContext.hasBga(chartData) ? 1 : 0,
            chartData && selectContext.hasLongNote(chartData) ? 1 : 0,
            chartData && selectContext.hasAttachedText(chartData) ? 1 : 0,
            chartData ? chartData.minBpm || 0 : 0,
            chartData ? chartData.maxBpm || 0 : 0,
            chartData && root.chartHasBpmStop(chartData) ? 1 : 0,
            chartData && chartData.isRandom ? 1 : 0,
            chartData ? selectContext.judgeOption(chartData) : 0,
            chartData ? selectContext.highLevelOption(chartData) : 0,
            chartData ? selectContext.entryDifficulty(chartData) : 0,
            root.lr2ReplayType,
            chartData && selectContext.hasReplay(chartData) ? 1 : 0);
        if (root.skinUsesRuntimeOption(270)) {
            parts.push(root.gameplayLaneCoverChangingOptionActive() ? 1 : 0);
        }
        if (root.skinUsesRuntimeOption(1008)) {
            parts.push(root.tableInfoRevision);
        }

        root.appendGameplayRuntimeOptionSideState(parts, 1);
        if (root.gameplayLanePlayer(2)) {
            root.appendGameplayRuntimeOptionSideState(parts, 2);
        }

        if (root.skinUsesRuntimeOptionRange(2241, 2246)) {
            let judgementOptions = [
                Judgement.Perfect,
                Judgement.Great,
                Judgement.Good,
                Judgement.Bad,
                Judgement.Poor,
                Judgement.EmptyPoor
            ];
            for (let judgement of judgementOptions) {
                parts.push(root.judgementCountForExist(score1, judgement) > 0 ? 1 : 0);
            }
        }
        return parts;
    }

    function refreshGameplayRuntimeActiveOptions() : var {
        if (!root.gameplayScreenActive) {
            root.gameplayRuntimeActiveOptionsStateParts = root.emptyActiveOptions;
            if (!root.sameArrayValues(root.builtGameplayRuntimeActiveOptions, root.emptyActiveOptions)) {
                root.builtGameplayRuntimeActiveOptions = root.emptyActiveOptions;
            }
            return false;
        }

        let nextState = root.gameplayRuntimeOptionStateParts();
        if (root.sameArrayValues(nextState, root.gameplayRuntimeActiveOptionsStateParts)) {
            return false;
        }
        root.gameplayRuntimeActiveOptionsStateParts = nextState;

        let nextOptions = runtimeOptions.buildRuntimeActiveOptions(root.builtBaseRuntimeActiveOptions);
        if (root.sameArrayValues(nextOptions, root.builtGameplayRuntimeActiveOptions)) {
            return false;
        }

        root.builtGameplayRuntimeActiveOptions = nextOptions;
        return selectUpdateController.refreshGameplayRuntimeActiveOptions();
    }

    readonly property var selectSkinChartWrapper: root.effectiveScreenKey === "select"
        ? selectContext.visualChartWrapper
        : null
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? root.selectSkinChartWrapper
        : selectSideEffects.renderChart
    readonly property var visualSelectChart: root.effectiveScreenKey === "select"
        ? root.selectSkinChartWrapper
        : root.renderChart
    readonly property var selectedCourseStages: {
        if (root.effectiveScreenKey === "courseResult") {
            if (root.chartDatas && root.chartDatas.length > 0) {
                return root.chartDatas;
            }
            if (root.course && root.course.loadCharts) {
                return root.course.loadCharts();
            }
        }
        let item = root.effectiveScreenKey === "select" ? selectContext.selectedStateItem : null;
        if (!item || !selectContext.isCourse(item) || !item.loadCharts) {
            return [];
        }
        return item.loadCharts();
    }

    Connections {
        target: selectContext
        function onEntryChangeSoundsRequested(count: var) : var {
            let repeats = Math.max(0, Math.round(count || 0));
            if (repeats <= 0) {
                return;
            }
            selectSideEffects.playScratchBurst(repeats);
        }
    }
    Connections {
        target: selectContext
        ignoreUnknownSignals: true
        function onFocusedItemChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
        function onFocusedChartDataChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
        function onSelectedStateItemChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
        function onSelectedStateChartDataChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
        function onSelectedStateCurrentChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
        function onSelectedDetailValueRevisionChanged() : void {
            root.queueSelectRuntimeActiveOptionsRefresh();
        }
    }
    Connections {
        target: Rg.tables
        ignoreUnknownSignals: true
        function onDataChanged() : void {
            ++root.tableInfoRevision;
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onRowsInserted() : void {
            ++root.tableInfoRevision;
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onModelReset() : void {
            ++root.tableInfoRevision;
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
    }
    Connections {
        target: Rg.profileList
        function onMainProfileChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onBattleActiveChanged() : void {
            root.refreshSelectPlayOptionLayout();
        }
    }
    Connections {
        target: Rg.profileList ? Rg.profileList.mainProfile : null
        ignoreUnknownSignals: true
        function onLoginStateChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onOnlineUserDataChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onTachiDataChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
    }
    Connections {
        target: Rg.profileList && Rg.profileList.battleProfiles ? Rg.profileList.battleProfiles : null
        function onPlayer1ProfileChanged() : void {
            root.refreshSelectPlayOptionLayout();
        }
        function onPlayer2ProfileChanged() : void {
            root.refreshSelectPlayOptionLayout();
        }
    }
    Connections {
        target: root.mainGeneralVarsRef
        ignoreUnknownSignals: true
        function onDpOptionsChanged() : void {
            root.refreshSelectPlayOptionLayout();
        }
        function onGaugeTypeChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onGaugeModeChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onBottomShiftableGaugeChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
        function onRankingProviderChanged() : void {
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
        }
    }
    onEffectiveScreenKeyChanged: {
        if (root.effectiveScreenKey !== "select") {
            root.selectPanel = 0;
            root.selectPanelHeldByStart = 0;
            root.selectPanelClosing = 0;
            root.hideReadmeImmediately();
            root.clearLr2RankingTransition();
        }
        if (root.gameplayScreenActive) {
            root.gameplayResultOpened = false;
            root.gameplayCourseResultPending = false;
            root.gameplayShowedCourseResult = false;
            root.gameplayPlayStopped = false;
            root.gameplayNothingWasHit = true;
            root.resetGameplayTimers();
            root.refreshGameplayRuntimeActiveOptions();
            root.updateGameplayStatusTimers();
            root.activateGameplayIfNeeded();
        }
        if (root.resultScreenActive) {
            root.resultTimer151SkinTime = -1;
            root.resultTimer152SkinTime = -1;
            root.updateResultOldScores();
        }
        root.updateGameplaySavedScores();
        root.handleScreenContextChanged();
    }
    onChartChanged: {
        root.gameplayRevision++;
        if (root.effectiveScreenKey !== "select") {
            root.refreshSelectRuntimeActiveOptions();
        }
        root.refreshGameplayRuntimeActiveOptions();
        root.gameplayResultOpened = false;
        root.gameplayCourseResultPending = false;
        root.gameplayShowedCourseResult = false;
        root.gameplayPlayStopped = false;
        root.gameplayNothingWasHit = true;
        root.stopGameplayLifecycle();
        root.resetGameplayTimers();
        root.updateGameplayStatusTimers();
        root.updateGameplaySavedScores();
        root.activateGameplayIfNeeded();
        if (root.effectiveScreenKey !== "select") {
            root.commitLr2RankingRequest();
        }
    }
    onScoresChanged: root.updateResultOldScores()
    onProfilesChanged: root.updateResultOldScores()
    onChartDataChanged: {
        if (root.effectiveScreenKey !== "select") {
            root.refreshSelectRuntimeActiveOptions();
        }
        root.updateResultOldScores();
    }
    onChartDatasChanged: root.updateResultOldScores()
    onCourseChanged: root.updateResultOldScores()
    Connections {
        target: root.gameplayScreenActive ? root.chart : null
        ignoreUnknownSignals: true
        function onCurrentChartIndexChanged() : void {
            root.gameplayRevision++;
            root.bumpGameplayNumberRevision(0);
            root.updateGameplayStaticNumberRevision();
            root.refreshGameplayRuntimeActiveOptions();
            root.gameplayResultOpened = false;
            root.gameplayCourseResultPending = false;
            root.gameplayPlayStopped = false;
            root.gameplayNothingWasHit = true;
            root.resetGameplayTimers();
            root.updateGameplayStatusTimers();
            root.updateGameplaySavedScores();
            root.activateGameplayIfNeeded();
        }
        function onStatusChanged() : void {
            root.gameplayRevision++;
            root.bumpGameplayNumberRevision(0);
            root.updateGameplayStaticNumberRevision();
            root.refreshGameplayRuntimeActiveOptions();
            root.handleGameplayStatusChanged();
        }
    }
    Connections {
        target: root.gameplayScreenActive ? root.gameplayScore(1) : null
        ignoreUnknownSignals: true
        function onHit(hit: var) : void {
            root.notifyGameplayReplayHit(hit);
            if (root.gameplayHitCountsAsPlayed(hit)) {
                root.gameplayNothingWasHit = false;
            }
            root.updateGameplayHitTimers(root.gameplayHitDisplaySide(1, hit), hit, 1);
            root.requestGameplayRevisionRefresh(1);
        }
        function onPointsChanged() : void {
            root.updateGameplayScorePrintTarget(1);
            root.requestGameplayRevisionRefresh(1);
        }
        function onComboChanged() : void {
            root.requestGameplayRevisionRefresh(1);
        }
        function onMaxComboChanged() : void {
            root.requestGameplayRevisionRefresh(1);
        }
        function onMaxPointsNowChanged() : void {
            root.requestGameplayRevisionRefresh(1);
        }
    }
    Connections {
        target: root.gameplayScreenActive ? root.gameplayScore(2) : null
        ignoreUnknownSignals: true
        function onHit(hit: var) : void {
            if (root.gameplayHitCountsAsPlayed(hit)) {
                root.gameplayNothingWasHit = false;
            }
            root.updateGameplayHitTimers(root.gameplayHitDisplaySide(2, hit), hit, 2);
            root.requestGameplayRevisionRefresh(2);
        }
        function onPointsChanged() : void {
            root.updateGameplayScorePrintTarget(2);
            root.requestGameplayRevisionRefresh(2);
        }
        function onComboChanged() : void {
            root.requestGameplayRevisionRefresh(2);
        }
        function onMaxComboChanged() : void {
            root.requestGameplayRevisionRefresh(2);
        }
        function onMaxPointsNowChanged() : void {
            root.requestGameplayRevisionRefresh(2);
        }
    }

    function handleScreenContextChanged() : void {
        let rankingRequestChanged = root.commitLr2RankingRequest();
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        skinTiming.skinClockRef.restartSelectInfoTimer();
        if (rankingRequestChanged) {
            lr2Ranking.applyStatsToSelectContext();
        }
    }

    readonly property bool acceptsInput: screenState.acceptsInput
    onAcceptsInputChanged: {
        if (root.effectiveScreenKey === "select" && root.acceptsInput) {
            root.selectNoScrollStartSkinTime = root.renderSkinTime;
        }
        if (root.acceptsInput && root.heldOptionPanel > 0 && !root.startHoldSuppressed) {
            root.holdSelectPanel(root.heldOptionPanel);
        }
    }

    function wrapValue(value: var, count: var) : var {
        return ((value % count) + count) % count;
    }

    function elementSourceFrameCount(src: var) : var {
        if (!src) {
            return 0;
        }
        if (src.imageSet) {
            return src.imageSetSources ? src.imageSetSources.length : 0;
        }
        return Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1);
    }

    function closeSelectPanel() : var {
        return selectPanelController.closeSelectPanel();
    }

    function startSelectPanelCloseTimer(panel: var) : var {
        return selectPanelController.startSelectPanelCloseTimer(panel);
    }

    function openSelectPanel(panel: var, heldByStart: var) : var {
        return selectPanelController.openSelectPanel(panel, heldByStart);
    }

    function toggleSelectPanel(panel: var) : var {
        return selectPanelController.toggleSelectPanel(panel);
    }

    function holdSelectPanel(panel: var) : var {
        return selectPanelController.holdSelectPanel(panel);
    }

    function releaseHeldSelectPanel(panel: var) : var {
        return selectPanelController.releaseHeldSelectPanel(panel);
    }

    function handleLr2Button(buttonId: var, delta: var, panel: var, soundPlayer: var, sourceCount: var, mouseButton: var) : var {
        return selectPanelController.handleLr2Button(buttonId, delta, panel, soundPlayer, sourceCount, mouseButton);
    }

    function triggerSelectPanelButton(buttonId: var, delta: var) : var {
        return selectPanelController.triggerSelectPanelButton(buttonId, delta);
    }

    function triggerPanelButtonForKey(p1ButtonId: var, p2ButtonId: var, key: var, delta: var) : var {
        return selectPanelController.triggerPanelButtonForKey(p1ButtonId, p2ButtonId, key, delta);
    }

    function keyUsesPlayer2(key: var) : var {
        return selectPanelController.keyUsesPlayer2(key);
    }

    function gameplayOptionSideForKey(key: var) : var {
        return selectPanelController.gameplayOptionSideForKey(key);
    }

    function gameplayOptionModifierHeldForKey(key: var) : var {
        return selectPanelController.gameplayOptionModifierHeldForKey(key);
    }

    function gameplayOptionKeyHeld(key: var) : var {
        return selectPanelController.gameplayOptionKeyHeld(key);
    }

    function isGameplayOptionModifierKey(key: var) : var {
        return selectPanelController.isGameplayOptionModifierKey(key);
    }

    function handleLr2GameplayOptionKey(key: var) : var {
        return selectPanelController.handleLr2GameplayOptionKey(key);
    }

    function releaseLr2GameplayOptionKey(key: var) : var {
        return selectPanelController.releaseLr2GameplayOptionKey(key);
    }

    function stopLr2GameplayOptionRepeat() : var {
        return selectPanelController.stopLr2GameplayOptionRepeat();
    }

    function pressLr2GameplayScratchDirection(side: var, up: var) : var {
        return selectPanelController.pressLr2GameplayScratchDirection(side, up);
    }

    function releaseLr2GameplayScratchDirection(side: var, up: var) : var {
        return selectPanelController.releaseLr2GameplayScratchDirection(side, up);
    }

    function handleLr2GameplayScratchTick(side: var, up: var, number: var) : var {
        return selectPanelController.handleLr2GameplayScratchTick(side, up, number);
    }

    function handleLr2GameplayArrow(key: var) : var {
        return selectPanelController.handleLr2GameplayArrow(key);
    }

    function handleSelectPanelKey(key: var) : var {
        return selectPanelController.handleSelectPanelKey(key);
    }

    function handleSelectPanelArrow(key: var) : var {
        return selectPanelController.handleSelectPanelArrow(key);
    }

    function handleSelectWheel(wheel: var) : var {
        return selectPanelController.handleSelectWheel(wheel);
    }

    function onMouseStateContainsPoint(src: var, state: var, mx: var, my: var) : var {
        if (!src || !state) {
            return null;
        }
        const hoverX = state.x + (src.hoverX || 0);
        const hoverY = state.y + (src.hoverY || 0);
        const hoverW = src.hoverW || state.w;
        const hoverH = src.hoverH || state.h;
        return mx >= hoverX && mx <= hoverX + hoverW && my >= hoverY && my <= hoverY + hoverH
            ? state
            : null;
    }

    readonly property var barBaseStateResolver: selectBarGeometry.barBaseStateResolver
    readonly property var barPositionMap: selectBarGeometry.barPositionMap
    readonly property bool fastBarScrollActive: selectBarGeometry.fastBarScrollActive
    readonly property real fastBarScrollX: selectBarGeometry.fastBarScrollX
    readonly property real fastBarScrollY: selectBarGeometry.fastBarScrollY
    readonly property real selectedFastBarDrawX: selectBarGeometry.selectedFastBarDrawX
    readonly property real selectedFastBarDrawY: selectBarGeometry.selectedFastBarDrawY

    Lr2GameplayFrameState {
        id: gameplayFrameState
        chart: root.chart || null
    }

    Lr2SkinTiming {
        id: skinTiming
        host: root
        skinModel: skinModel
        selectContext: selectContext

        onAutoAdvanceRequested: {
            if (root.shouldAutoAdvance) {
                globalRoot.openGameplay(root.chart);
            }
        }
    }

    Lr2SkinRuntime {
        id: skinRuntime
        skinModel: root.skinModelRef
        timerState: root.skinTimerStateRef
        screenKey: root.effectiveScreenKey
        gameplayScreen: root.gameplayScreenActive
        selectBarElementSortBase: root.selectBarElementSortBase
    }

    Lr2WallClockState {
        id: wallClockState
        sceneStartMs: skinTiming.sceneStartMs
    }

    Lr2ScreenState {
        id: screenState
        explicitKey: root.screenKey
        csvPath: root.csvPath
        hostEnabled: root.enabled
        hostVisible: root.visible
        stackActive: root.StackView.status === StackView.Active
        globalSkinTime: root.effectiveScreenKey === "select" ? root.renderSkinTime : root.globalSkinTime
        startInput: skinModel.startInput || 0
        selectSearchFocused: selectSearchState.focused
        readmeMode: root.lr2ReadmeMode
    }

    property alias skinClockResolutionMs: skinTiming.resolutionMs
    property alias selectInfoStartSkinTime: skinTiming.selectInfoStartSkinTime
    property alias selectScrollStartSkinTime: skinTiming.selectScrollStartSkinTime
    property alias selectNoScrollStartSkinTime: skinTiming.selectNoScrollStartSkinTime
    property alias selectDatabaseLoadedSkinTime: skinTiming.selectDatabaseLoadedSkinTime
    property alias selectAnimationLimit: skinTiming.selectAnimationLimit
    property alias barAnimationLimit: skinTiming.barAnimationLimit
    property alias selectInfoAnimationLimit: skinTiming.selectInfoAnimationLimit
    property alias sceneStartMs: skinTiming.sceneStartMs
    property alias skinClockRef: skinTiming.skinClockRef
    property alias skinClockNowMs: skinTiming.skinClockNowMs
    property alias globalSkinTime: skinTiming.globalSkinTime
    property alias selectLiveSkinTime: skinTiming.selectLiveSkinTime
    property alias selectSourceSkinTime: skinTiming.selectSourceSkinTime
    property alias selectInfoElapsed: skinTiming.selectInfoElapsed
    property alias renderSkinTime: skinTiming.renderSkinTime
    property alias barSkinTime: skinTiming.barSkinTime
    readonly property bool shouldAutoAdvance: skinTiming.shouldAutoAdvance
    readonly property var timers: root.resultScreenActive
        ? skinTiming.timers
        : root.zeroTimers

    onRenderSkinTimeChanged: {
        if (root.effectiveScreenKey === "select"
                && selectHoverState.hasPoint
                && root.globalSkinTime < root.selectAnimationLimit) {
            selectHoverState.refreshVisibleState();
        }
    }

    readonly property bool selectReplayOptionsUsed: selectUpdateController.selectReplayOptionsUsed
    readonly property bool selectScoreOptionIdsUsed: selectUpdateController.selectScoreOptionIdsUsed
    readonly property bool selectEntryStatusOptionsUsed: selectUpdateController.selectEntryStatusOptionsUsed
    readonly property bool selectDifficultyBarOptionsUsed: selectUpdateController.selectDifficultyBarOptionsUsed
    readonly property bool selectDifficultyLampOptionsUsed: selectUpdateController.selectDifficultyLampOptionsUsed
    readonly property bool selectDifficultyStateUsed: selectUpdateController.selectDifficultyStateUsed
    readonly property bool selectCourseDetailOptionsUsed: selectUpdateController.selectCourseDetailOptionsUsed
    readonly property bool selectRankingStatusOptionsUsed: selectUpdateController.selectRankingStatusOptionsUsed

    Lr2SelectContext {
        id: selectContext
        updatesActive: root.screenUpdatesActive && root.effectiveScreenKey === "select"
        enabled: updatesActive
        barBodyTypes: skinModel.barBodyTypes || []
        barTitleTypes: skinModel.barTitleTypes || []
        barLampVariants: skinModel.barLampVariants || []
        useBeatorajaBarTextTypes: root.lr2SkinUsesBeatorajaSemantics
        useBeatorajaSelectOptions: root.lr2SkinUsesBeatorajaSemantics
        scoreOptionIdsUsed: root.selectScoreOptionIdsUsed
        difficultyStateUsed: root.selectDifficultyStateUsed
        difficultyLampStateUsed: root.selectDifficultyLampOptionsUsed
        generalVars: root.mainGeneralVarsRef
        barRowCount: skinModel.barRows ? skinModel.barRows.length : 0
        barCenter: skinModel.barCenter
        stageFileSourceUsed: skinModel.usesStageFileSource
        backBmpSourceUsed: skinModel.usesBackBmpSource
        bannerSourceUsed: skinModel.usesBannerSource

        Component.onCompleted: {
            root.selectContextRef = selectContext;
        }

        onBarMoveStartMsChanged: {
            if (root.screenUpdatesActive && root.effectiveScreenKey === "select" && barMoveStartMs > 0) {
                root.selectScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onVisualMoveActiveChanged: {
            if (root.screenUpdatesActive && root.effectiveScreenKey === "select" && !visualMoveActive && !scrollFixedPointDragging) {
                root.selectNoScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onScrollFixedPointDraggingChanged: {
            if (root.screenUpdatesActive && root.effectiveScreenKey === "select" && !scrollFixedPointDragging && !visualMoveActive) {
                root.selectNoScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onOpenedFolder: {
            if (root.screenUpdatesActive && root.effectiveScreenKey === "select") {
                root.selectDatabaseLoadedSkinTime = root.renderSkinTime;
                if (!visualMoveActive && !scrollFixedPointDragging) {
                    root.selectNoScrollStartSkinTime = root.renderSkinTime;
                }
            }
        }
    }

    Lr2SelectUpdateController {
        id: selectUpdateController
        skinModel: skinModel
        skinRuntime: skinRuntime
        screenUpdatesActive: root.screenUpdatesActive
        effectiveScreenKey: root.effectiveScreenKey
        componentReady: root.componentReady
        rankingMode: selectContext.rankingMode
        selectPanel: root.selectPanel
        parseActiveOptions: root.parseActiveOptions
        barRuntimeActiveOptions: root.builtBarRuntimeActiveOptions
        baseRuntimeActiveOptions: root.builtBaseRuntimeActiveOptions
        selectCommonRuntimeActiveOptions: root.builtSelectCommonRuntimeActiveOptions
        selectRequiredRuntimeActiveOptions: root.builtSelectRequiredRuntimeActiveOptions
        screenRuntimeActiveOptions: root.builtScreenRuntimeActiveOptions
        gameplayScreen: root.gameplayScreenActive
    }

    Lr2SelectKeyNavigationFilter {
        target: root
        selectScrollReady: screenState.selectScrollReady
        navigationController: selectContext.navigationController
    }

    Lr2PlayContext {
        id: playContext
        enabled: root.gameplayScreenActive
        screenRoot: root
        renderSkinTime: root.renderSkinTime
        scratchRotationSides: skinModel.scratchRotationSides || 0
    }

    ScoreReplayer {
        id: gameplayTargetScoreReplayer
        hitEvents: {
            let score = root.gameplayTargetSavedScore();
            return score && score.replayData ? score.replayData.hitEvents : [];
        }
    }

    ScoreReplayer {
        id: gameplayBestScoreReplayer
        hitEvents: {
            let score = root.gameplayBestSavedScore();
            return score && score.replayData ? score.replayData.hitEvents : [];
        }
    }

    Lr2SkinModel {
        id: skinModel
        csvPath: root.csvPath
        settingValues: root.skinSettings || {}
        activeOptions: root.parseActiveOptions

        onSkinLoaded: {
            if (root.effectiveScreenKey === "select") {
                selectContext.resetSortSourceFrameCount();
            }
            root.queueSkinClockRestartAfterLoad();
            root.openSelectIfNeeded();
            root.activateGameplayIfNeeded();
            root.refreshLr2SkinSettingItems();
            selectUpdateController.refreshBaseActiveOptions();
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
            root.updateResultOldScores();
        }
    }

    readonly property var skinModelRef: skinModel

    onCsvPathChanged: root.openSelectIfNeeded()
    onSkinSettingsChanged: {
        root.refreshLr2SkinSettingItems();
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }
    onScreenKeyChanged: {
        root.openSelectIfNeeded();
        root.activateGameplayIfNeeded();
        root.refreshLr2SkinSettingItems();
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }

    Component.onCompleted: {
        root.componentReady = true;
        selectSideEffects.ready = true;
        root.commitLr2RankingRequest();
        skinTiming.restartSkinClock();
        root.openSelectIfNeeded();
        root.activateGameplayIfNeeded();
        root.updateGameplaySavedScores();
        root.refreshLr2SkinSettingItems();
        selectUpdateController.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        root.refreshGameplayRuntimeActiveOptions();
    }

    function pauseScreenActivity() : void {
        readmeState.pauseActivity();
        lr2Ranking.pauseActivity();
        skinTiming.pauseActivity();
        if (root.effectiveScreenKey === "select") {
            root.stopSelectAudio();
        }
        if (root.gameplayScreenActive) {
            root.stopGameplayLifecycle();
        }
    }

    function stopSelectAudio() : void {
        selectSideEffects.stopAudio();
        openFolderSound.stop();
        closeFolderSound.stop();
        optionOpenSound.stop();
        optionCloseSound.stop();
        optionChangeSound.stop();
    }

    function playOneShot(player: var) : var {
        if (!root.enabled || !player || !player.source) {
            return;
        }
        if (player.playOverlapping) {
            player.playOverlapping();
            return;
        }
        player.stop();
        player.play();
    }

    function reloadCurrentSelectFolder() : var {
        if (root.effectiveScreenKey !== "select" || selectContext.historyStack.length === 0) {
            return false;
        }
        let currentFolder = selectContext.historyStack[selectContext.historyStack.length - 1];
        selectContext.open(currentFolder, selectContext.focusedItem);
        return true;
    }

    function selectGoBack() : var {
        if (root.closeLr2Ranking()) {
            return;
        }
        let before = selectContext.historyStack.length;
        selectContext.goBack();
        if (selectContext.historyStack.length < before) {
            root.playOneShot(closeFolderSound);
        }
    }

    function selectGoForward(item: var, autoplay: var, replay: var, replayScore: var) : void {
        let targetItem = item === undefined ? selectContext.activationItem() : item;
        let before = selectContext.historyStack.length;
        selectContext.goForward(targetItem, autoplay, replay, replayScore);
        if (selectContext.historyStack.length > before) {
            root.playOneShot(openFolderSound);
        }
    }

    function playSelectScratch() : void {
        selectSideEffects.playScratch();
    }

    function resultInputReady() : var {
        return root.enabled && root.resultScreenActive && root.acceptsInput;
    }

    function closeResultScreen() : var {
        if (!root.resultInputReady()) {
            return false;
        }
        if (root.resultTimer151SkinTime < 0 && root.renderSkinTime < root.resultGraphEndSkinTime) {
            root.resultTimer151SkinTime = root.renderSkinTime;
            return true;
        }
        sceneStack.pop();
        return true;
    }

    function submitSelectSearch() : void { selectSearchState.submit(); }

    Keys.onUpPressed: (event) => {
        if (root.lr2ReadmeMode === 1) {
            event.accepted = true;
            root.scrollReadmeBy(0, Math.max(1, root.lr2ReadmeLineSpacing));
            return;
        }
        if (root.handleLr2GameplayArrow(Qt.Key_Up)) {
            event.accepted = true;
            return;
        }
        if (!screenState.selectScrollReady) return;
        event.accepted = true;
        selectContext.navigationController.decrementViewIndex(event.isAutoRepeat);
    }
    Keys.onDownPressed: (event) => {
        if (root.lr2ReadmeMode === 1) {
            event.accepted = true;
            root.scrollReadmeBy(0, -Math.max(1, root.lr2ReadmeLineSpacing));
            return;
        }
        if (root.handleLr2GameplayArrow(Qt.Key_Down)) {
            event.accepted = true;
            return;
        }
        if (!screenState.selectScrollReady) return;
        event.accepted = true;
        selectContext.navigationController.incrementViewIndex(event.isAutoRepeat);
    }
    Keys.onLeftPressed: (event) => {
        if (root.handleLr2GameplayArrow(Qt.Key_Left)) {
            event.accepted = true;
            return;
        }
        if (root.handleSelectPanelArrow(Qt.Key_Left)) {
            event.accepted = true;
            return;
        }
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoBack();
    }
    Keys.onRightPressed: (event) => {
        if (root.handleLr2GameplayArrow(Qt.Key_Right)) {
            event.accepted = true;
            return;
        }
        if (root.handleSelectPanelArrow(Qt.Key_Right)) {
            event.accepted = true;
            return;
        }
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoForward();
    }
    Keys.onReturnPressed: (event) => {
        if (root.lr2ReadmeMode > 0) {
            event.accepted = true;
            root.closeReadme();
            return;
        }
        if (root.closeResultScreen()) {
            event.accepted = true;
            return;
        }
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoForward();
    }

    property var lastNavigateKey: []

    function navigate(number: var, type: var, up: var, key: var) : var {
        if (!root.selectScrollReady() || root.lastNavigateKey[root.lastNavigateKey.length - 1] !== key) {
            return;
        }
        if (type === InputTranslator.AnalogScratchTick) {
            selectContext.queueWheelSteps(up ? 1 : -1);
            return;
        }
        let func = up ? selectContext.decrementViewIndex : selectContext.incrementViewIndex;
        if (type === InputTranslator.ButtonTick) {
            if (number === 0 || number >= 10) {
                func(true);
            }
        } else {
            func(false);
        }
    }

    function resetLr2SelectScratchRepeat() : void {
        root.selectTargetScratchDirection = 0;
        root.selectTargetScratchNextMs = 0;
    }

    function releaseLr2SelectScratchRepeat(up: var) : void {
        let sameDirectionStillHeld = up
            ? (Input.col1sUp || Input.col2sUp)
            : (Input.col1sDown || Input.col2sDown);
        if (!sameDirectionStillHeld
                && root.selectTargetScratchDirection === (up ? 1 : -1)) {
            root.resetLr2SelectScratchRepeat();
        }
    }

    function handleLr2SelectScratchTick(side: var, up: var, number: var, type: var) : var {
        if (!root.selectInputReady() || root.selectPanel !== 1) {
            return false;
        }
        let direction = up ? 1 : -1;
        let now = Date.now();
        let firstTick = number === 0 || root.selectTargetScratchDirection !== direction;
        if (!firstTick && now < root.selectTargetScratchNextMs) {
            return true;
        }
        root.selectTargetScratchDirection = direction;
        root.selectTargetScratchNextMs = now
            + (firstTick
                ? root.selectTargetScratchInitialRepeatMillis
                : root.selectTargetScratchRepeatMillis);
        root.handleLr2Button(77, up ? 1 : -1, root.selectPanel, selectSideEffects.scratchSoundPlayer);
        return true;
    }

    Input.onCol1sDownTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(1, false, number, type)
                && !root.handleLr2GameplayScratchTick(1, false, number)) {
            root.navigate(number, type, false, BmsKey.Col1sDown);
        }
    }
    Input.onCol1sUpTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(1, true, number, type)
                && !root.handleLr2GameplayScratchTick(1, true, number)) {
            root.navigate(number, type, true, BmsKey.Col1sUp);
        }
    }
    Input.onCol2sDownTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(2, false, number, type)
                && !root.handleLr2GameplayScratchTick(2, false, number)) {
            root.navigate(number, type, false, BmsKey.Col2sDown);
        }
    }
    Input.onCol2sUpTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(2, true, number, type)
                && !root.handleLr2GameplayScratchTick(2, true, number)) {
            root.navigate(number, type, true, BmsKey.Col2sUp);
        }
    }
    Input.onCol1sDownPressed: {
        root.pressLr2GameplayScratchDirection(1, false);
        if (root.selectScrollReady() && root.selectPanel <= 0) {
            root.lastNavigateKey.push(BmsKey.Col1sDown);
        }
    }
    Input.onCol1sUpPressed: {
        root.pressLr2GameplayScratchDirection(1, true);
        if (root.selectScrollReady() && root.selectPanel <= 0) {
            root.lastNavigateKey.push(BmsKey.Col1sUp);
        }
    }
    Input.onCol2sDownPressed: {
        root.pressLr2GameplayScratchDirection(2, false);
        if (root.selectScrollReady() && root.selectPanel <= 0) {
            root.lastNavigateKey.push(BmsKey.Col2sDown);
        }
    }
    Input.onCol2sUpPressed: {
        root.pressLr2GameplayScratchDirection(2, true);
        if (root.selectScrollReady() && root.selectPanel <= 0) {
            root.lastNavigateKey.push(BmsKey.Col2sUp);
        }
    }
    Input.onCol1sDownReleased: {
        root.releaseLr2GameplayScratchDirection(1, false);
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sDown);
        root.releaseLr2SelectScratchRepeat(false);
    }
    Input.onCol1sUpReleased: {
        root.releaseLr2GameplayScratchDirection(1, true);
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sUp);
        root.releaseLr2SelectScratchRepeat(true);
    }
    Input.onCol2sDownReleased: {
        root.releaseLr2GameplayScratchDirection(2, false);
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sDown);
        root.releaseLr2SelectScratchRepeat(false);
    }
    Input.onCol2sUpReleased: {
        root.releaseLr2GameplayScratchDirection(2, true);
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sUp);
        root.releaseLr2SelectScratchRepeat(true);
    }
    Input.onCol11Pressed: if (root.selectNavigationReady() && root.selectPanel <= 0) root.selectGoForward()
    Input.onCol17Pressed: if (root.selectNavigationReady() && root.selectPanel <= 0) root.selectGoForward()
    Input.onCol21Pressed: if (root.selectNavigationReady() && root.selectPanel <= 0) root.selectGoForward()
    Input.onCol27Pressed: if (root.selectNavigationReady() && root.selectPanel <= 0) root.selectGoForward()
    Input.onButtonPressed: (key) => {
        if (root.handleLr2GameplayOptionKey(key)) {
            return;
        }
        if (root.handleResultGaugeSelectKey(key)) {
            return;
        }
        if (root.closeResultScreen()) {
            return;
        }
        selectPanelController.pressSelectHeldButtonTimer(key);
        if (root.handleSelectPanelKey(key)) {
            return;
        }
        if (root.selectInputReady() && root.selectPanel > 0) {
            return;
        }
        if (root.isLr2RankingKey(key) && root.openLr2Ranking()) {
            return;
        }
        if (root.selectNavigationReady()
                && root.selectPanel <= 0
                && (key === BmsKey.Col16 || key === BmsKey.Col26)) {
            root.selectGoForward(undefined, true);
            return;
        }
        if (root.selectNavigationReady()
                && root.selectPanel <= 0
                && (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26)) {
            root.selectGoBack();
        }
    }
    Input.onButtonReleased: (key) => {
        root.releaseLr2GameplayOptionKey(key);
        selectPanelController.releaseSelectHeldButtonTimer(key);
        if (root.isLr2RankingKey(key)) {
            root.closeLr2Ranking();
        }
    }

    Lr2SelectSideEffects {
        id: selectSideEffects
        host: root
        skinModel: skinModel
        previewSource: selectContext.selectedPreviewAudioSource
        active: root.selectAudioActive
    }

    AudioPlayer {
        id: openFolderSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "f-open" : ""
    }

    AudioPlayer {
        id: closeFolderSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "f-close" : ""
    }

    AudioPlayer {
        id: gameplayReadySound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "playready" : ""
        onPlayingChanged: {
            if (!playing
                    && root.gameplayStartArmed
                    && root.enabled
                    && root.gameplayScreenActive
                    && root.chart
                    && root.chartStatusIs(root.chart.status, ChartRunner.Ready)) {
                skinTiming.gameplayStartTimer.restart();
            }
        }
    }

    AudioPlayer {
        id: gameplayStopSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "playstop" : ""
    }

    AudioPlayer {
        id: optionOpenSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "o-open" : ""
    }

    AudioPlayer {
        id: optionCloseSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "o-close" : ""
    }

    AudioPlayer {
        id: optionChangeSound
        source: root.mainGeneralVarsRef ? root.mainGeneralVarsRef.soundsetPath + "o-change" : ""
    }

    Lr2SkinSliderState {
        id: skinSliderState
        screenRoot: root
        selectContext: selectContext
        valueResolver: root.skinValueResolverRef
        optionChangeSound: optionChangeSound
        skinTiming: root.skinTimingRef
    }

    Lr2SelectSearchState {
        id: selectSearchState
        screenRoot: root
        selectContext: selectContext
        skinTiming: root.skinTimingRef
    }

    Lr2RankingState {
        id: lr2Ranking
        host: root
        selectContext: selectContext
        optionOpenSound: optionOpenSound
        optionCloseSound: optionCloseSound
    }

    Lr2SkinValueResolver {
        id: valueResolver
        screenRoot: root
        selectContext: selectContext
        playContext: playContext
        rankingState: lr2Ranking
    }

    Lr2SelectPanelController {
        id: selectPanelController
        screenRoot: root
        selectContext: selectContext
        selectHoverState: selectHoverState
        scratchSound: selectSideEffects.scratchSoundPlayer
        optionOpenSound: optionOpenSound
        optionCloseSound: optionCloseSound
        optionChangeSound: optionChangeSound
    }

    Lr2SelectBarGeometry {
        id: selectBarGeometry
        screenRoot: root
        skinModel: skinModel
        selectContext: selectContext
        selectedOffset: selectContext.selectedOffset
        visibleBarSlotOffset: selectContext.visibleBarSlotOffset
        selectVisualState: selectContext.visualStateObject
    }

    readonly property real skinW: Math.max(1, skinModel.skinWidth || 640)
    readonly property real skinH: Math.max(1, skinModel.skinHeight || 480)
    readonly property real skinVisualScaleX: root.width > 0 ? Math.max(0.0001, root.width / skinW) : 1.0
    readonly property real skinVisualScaleY: root.height > 0 ? Math.max(0.0001, root.height / skinH) : 1.0
    readonly property real skinScale: 1.0

    // LR2 stretches its authored canvas to the actual screen aspect.
    Lr2SkinScene {
        id: skinScene
        anchors.fill: parent
        screenRoot: root
        skinModel: skinModel
        playContext: playContext
        selectContext: selectContext
        selectBarGeometry: selectBarGeometry
    }
}
