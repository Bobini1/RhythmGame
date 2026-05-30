pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml

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
    readonly property var lr2SkinMetadata: root.parseLr2SkinMetadata()
    readonly property string lr2SkinFamily: root.lr2SkinMetadata.format === "beatoraja" ? "beatoraja" : "lr2"
    readonly property bool lr2SkinUsesBeatorajaSemantics: root.lr2SkinFamily === "beatoraja"
    readonly property var emptyActiveOptions: []
    readonly property var zeroTimers: ({ "0": 0 })
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property bool usedOptionFilterActive: !!skinModel
        && !!skinModel.usedOptions
        && skinModel.usedOptions.length > 0
    readonly property var usedOptionLookup: {
        let result = {};
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        for (let option of used) {
            result[Math.abs(option)] = true;
        }
        return result;
    }
    readonly property bool usedElementOptionFilterAvailable: !!skinModel
        && skinModel.usedElementOptions !== undefined
    readonly property var usedElementOptionLookup: {
        let result = {};
        let used = root.usedElementOptionFilterAvailable && skinModel.usedElementOptions
            ? skinModel.usedElementOptions
            : [];
        for (let option of used) {
            result[Math.abs(option)] = true;
        }
        return result;
    }
    property int gameplayRevision: 0
    property int gameplayNumberRevision1: 0
    property int gameplayNumberRevision2: 0
    property int gameplayStaticNumberRevision: 0
    property string gameplayStaticNumberRevisionKey: "static|0"
    property int gameplayTimerRevision: 0
    property bool gameplayRevisionRefreshPending: false
    property bool gameplayNumberRevision1Pending: false
    property bool gameplayNumberRevision2Pending: false
    property bool gameplayTimerRevisionPending: false
    property bool gameplayRuntimeRefreshPending: false
    property var gameplayRuntimeActiveOptions: []
    property var selectRuntimeActiveOptions: []
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
    property bool gameplayShowedCourseResult: false
    property bool gameplayPlayStopped: false
    property bool gameplayNothingWasHit: true
    property bool gameplayStartArmed: false
    property alias gameplayFrameStateRef: gameplayFrameState
    readonly property bool stackScreenActive: screenState.stackActive
    readonly property bool screenUpdatesActive: screenState.updatesActive
    readonly property int lr2CurrentFps: skinTiming.currentFps
    readonly property var lr2InitialClockNow: wallClockState.initialNow
    property alias skinTimingRef: skinTiming
    property alias skinTimerStateRef: skinTiming.skinTimerStateRef
    property alias skinRuntimeRef: skinRuntime
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
    property alias resultOldScoresRevision: resultState.resultOldScoresRevision
    property alias resultOldScoresRequest: resultState.resultOldScoresRequest
    property alias resultTimer151SkinTime: resultState.resultTimer151SkinTime
    property alias resultTimer152SkinTime: resultState.resultTimer152SkinTime
    property alias resultGraphStartSkinTime: resultState.resultGraphStartSkinTime
    property alias resultGraphEndSkinTime: resultState.resultGraphEndSkinTime
    property alias resultTimingStatsCache: resultState.resultTimingStatsCache
    property alias resultJudgeTimingCountsCache: resultState.resultJudgeTimingCountsCache
    readonly property var gameplayKeyTimers: [
        100, 101, 102, 103, 104, 105, 106, 107,
        110, 111, 112, 113, 114, 115, 116, 117
    ]
    readonly property var gameplayLongNoteTimers: [
        70, 71, 72, 73, 74, 75, 76, 77,
        80, 81, 82, 83, 84, 85, 86, 87
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
        case "EX_HARD":
            return "EXHARD";
        case "EXHARD_DAN":
        case "EX_HARD_DAN":
            return "EXHARDDAN";
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
        model: root.isGameplayScreen() && root.chart ? root.gameplayKeyTimers : []

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
        model: root.isGameplayScreen() && root.chart ? root.gameplayLongNoteTimers : []

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
        selectSideEffects.update();
        if (root.effectiveScreenKey === "select"
                && root.acceptsInput
                && root.heldOptionPanel > 0
                && !root.startHoldSuppressed) {
            root.holdSelectPanel(root.heldOptionPanel);
        }
        root.forceActiveFocus();
    }

    Shortcut {
        enabled: root.screenUpdatesActive && !root.selectSearchHasFocus()
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
            if (root.isGameplayScreen() && root.handleGameplayEscape()) {
                return;
            }
            sceneStack.pop();
        }
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
        sequence: "F1"
        onActivated: root.toggleMainHelp()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
        sequence: "F2"
        onActivated: root.toggleSelectedReadme()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select"
        sequence: "F4"
        onActivated: globalRoot.toggleFullScreen()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
        sequence: "F5"
        onActivated: root.openLr2InternetRanking()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
        sequence: "F8"
        onActivated: root.reloadCurrentSelectFolder()
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
        sequence: "3"
        onActivated: root.triggerSelectPanelButton(308, 1)
    }

    Shortcut {
        enabled: root.screenUpdatesActive && root.effectiveScreenKey === "select" && !root.selectSearchHasFocus()
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
        return root.isGameplayScreen()
            && !!root.chart
            && root.playerIsAutoPlayer(root.chart.player1)
            && (!root.chart.player2 || root.playerIsAutoPlayer(root.chart.player2));
    }

    function gameplayReplayActive() : var {
        if (!root.isGameplayScreen() || !root.chart) {
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
        } else if (root.isResultScreen()) {
            options.push(root.resultClearOption(), 350);
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
    property alias lr2SkinSettingsRevision: skinSettingsState.revision
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
        tracking: root.selectHoverTracking
        skinScale: root.skinScale
    }
    property alias selectHoverElements: selectHoverState.elements
    readonly property int selectHoverElementCount: selectHoverState.elementCount
    readonly property var selectHoverCandidateKeys: selectHoverState.candidateKeys
    property alias selectHoverVisibleByIndex: selectHoverState.visibleByIndex
    readonly property string selectHoverVisibleSignature: selectHoverState.visibleSignature
    property alias selectHoverSkinX: selectHoverState.skinX
    property alias selectHoverSkinY: selectHoverState.skinY
    readonly property bool selectHoverHasPoint: selectHoverState.hasPoint
    property alias selectSliderFixedPoint: skinSliderState.selectSliderFixedPoint
    property alias selectScratchSoundReady: selectSideEffects.scratchSoundReady
    readonly property bool selectAudioActive: root.screenUpdatesActive
        && root.effectiveScreenKey === "select"

    onSelectAudioActiveChanged: {
        if (!root.selectAudioActive) {
            root.stopSelectAudio();
        }
    }

    function selectHoverPointInSkinCoordinates() : var {
        return selectHoverState.pointInSkinCoordinates();
    }

    function updateSelectHoverPoint(x: var, y: var) : var {
        return selectHoverState.updatePoint(x, y);
    }

    function clearSelectHoverPoint() : void {
        selectHoverState.clearPoint();
    }

    function registerSelectHoverElement(elementIndex: var, src: var, enabled: var) : void {
        selectHoverState.registerElement(elementIndex, src, enabled);
    }

    function unregisterSelectHoverElement(elementIndex: var) : void {
        selectHoverState.unregisterElement(elementIndex);
    }

    function clearSelectHoverState() : void {
        selectHoverState.clearVisibleState();
    }

    function refreshSelectHoverState() : void {
        selectHoverState.refreshVisibleState();
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
        selectContext.flushFocusedStateRefresh();
        return root.openReadmePath(
            selectContext.attachedTextFile(selectContext.selectedChartData()));
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

    function mainGeneralVars() { return optionState.mainGeneralVars(); }
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
    function observedSelectButtonSourceCount(buttonId: var) : var {
        return selectPanelController.observedSelectButtonSourceCount(buttonId);
    }
    function registerSelectButtonSource(elementIndex: var, src: var) : void {
        selectPanelController.registerSelectButtonSource(elementIndex, src);
    }
    function unregisterSelectButtonSource(elementIndex: var) : void {
        selectPanelController.unregisterSelectButtonSource(elementIndex);
    }
    function selectOptionSourceCount(buttonId: var, sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        return count > 1 ? count : root.observedSelectButtonSourceCount(buttonId);
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
    function setHidSudIndex(side: var, index: var) : void { optionState.setHidSudIndex(side, index); }

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

    function lr2BgaEnabled() : var { return optionState.lr2BgaEnabled(); }

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

    readonly property int lr2TargetPercent: optionState.lr2TargetPercent
    function setTargetPercent(percent: var) : void { optionState.setTargetPercent(percent); }

    readonly property int lr2HiSpeedP1: optionState.lr2HiSpeedP1
    readonly property int lr2HiSpeedP2: optionState.lr2HiSpeedP2
    function noteGameplayOptionChanged() : var {
        if (!root.isGameplayScreen()) {
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
    function adjustScratchCoverNumber(side: var, amount: var) : void {
        optionState.adjustScratchCoverNumber(side, amount);
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
    function rankingProviderEnum() : var { return lr2Ranking.providerEnum(); }
    function lr2RankingMatchesCurrentChart() : var { return lr2Ranking.matchesCurrentChart(); }
    function lr2LocalRankingEntry() : var { return lr2Ranking.localEntry(); }
    function lr2RankingEntries() : var { return lr2Ranking.entries(); }
    function lr2RankingClearCounts(entries: var) : var { return lr2Ranking.clearCounts(entries); }
    function lr2RankingPlayerCount(entries: var) : var { return lr2Ranking.playerCount(entries); }
    function lr2RankingTotalPlayCount(entries: var) : var { return lr2Ranking.totalPlayCount(entries); }
    function lr2RankingProfileUserId() : var { return lr2Ranking.profileUserId(); }
    function lr2RankingPlayerRank(entries: var) : var { return lr2Ranking.playerRank(entries); }
    function lr2RankingSnapshot() : var { return lr2Ranking.snapshot(); }
    function lr2RankingEntryAt(index: var) : var { return lr2Ranking.entryAt(index); }
    function lr2RankingEntryName(index: var) : var { return lr2Ranking.entryName(index); }
    function lr2RankingEntryClearValue(index: var) : var { return lr2Ranking.entryClearValue(index); }
    function lr2RankingEntryExScore(index: var) : var { return lr2Ranking.entryExScore(index); }
    function lr2RankingClearCount() : var { return lr2Ranking.clearCount.apply(lr2Ranking, arguments); }
    function lr2RankingClearPercentValue() : var { return lr2Ranking.clearPercentValue.apply(lr2Ranking, arguments); }
    function applyRankingStatsToSelectContext() : void { lr2Ranking.applyStatsToSelectContext(); }
    function handleRankingModelChanged(tryOpenRanking: var, tryOpenInternetRanking: var) : void { lr2Ranking.handleModelChanged(tryOpenRanking, tryOpenInternetRanking); }
    function lr2RankingStatusOption() : var { return lr2Ranking.statusOption(); }
    function startLr2RankingTransition(action: var) : var { return lr2Ranking.startTransition(action); }
    function clearLr2RankingTransition() : void { lr2Ranking.clearTransition(); }
    function enterLr2RankingPostSwapTimer() : void { lr2Ranking.enterPostSwapTimer(); }
    function performLr2RankingOpen() : var { return lr2Ranking.performOpen(); }
    function performLr2RankingClose() : var { return lr2Ranking.performClose(); }
    function advanceLr2RankingTransition() : void { lr2Ranking.advanceTransition(); }
    function requestLr2RankingFetch(chart: var) : var { return lr2Ranking.requestFetch(chart); }
    function lr2TachiKeymode(chart: var) : var { return lr2Ranking.tachiKeymode(chart); }
    function lr2InternetRankingUrl(chart: var) : var { return lr2Ranking.internetRankingUrl(chart); }
    function finishOpenLr2InternetRanking() : var { return lr2Ranking.finishOpenInternetRanking(); }
    function openLr2InternetRanking() : var {
        selectContext.flushFocusedStateRefresh();
        return lr2Ranking.openInternetRanking();
    }
    function finishOpenLr2Ranking() : var { return lr2Ranking.finishOpenRanking(); }
    function openLr2Ranking() : var {
        selectContext.flushFocusedStateRefresh();
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

    function isSelectScrollSlider(src: var) : var {
        return skinSliderState.isSelectScrollSlider(src);
    }

    function isLr2GenericSlider(src: var) : var {
        return skinSliderState.isLr2GenericSlider(src);
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

    property alias selectSearchInputItem: selectSearchState.inputItem

    function isSelectSearchText(src: var) : var { return selectSearchState.isText(src); }
    function selectSearchTextState(src: var, dsts: var) : var { return selectSearchState.textState(src, dsts); }
    function textPrefix(text: var, position: var) : var { return selectSearchState.textPrefix(text, position); }
    function selectSearchHasFocus() : var { return selectSearchState.hasFocus(); }

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
        if (state && state.sortId !== undefined) {
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
        // #DST_BAR_* command positions configure that renderer instead of
        // participating in the outer skin draw order independently.
        if (root.isSelectBarElement(type, src)) {
            return root.selectBarElementSortBase
                + root.selectBarElementLayer(type, src)
                + root.fallbackSortId(dsts) * 0.000001
                + index * 0.000000001;
        }

        // OpenLR2 stamps every non-bar DST keyframe with the skin command
        // counter and globally sorts DrawingBuf by that value before drawing.
        if (type === 8) {
            return root.staticNoteElementSortId() + index * 0.000001;
        }
        return root.fallbackSortId(dsts) + index * 0.000001;
    }

    function dstsUseActiveOptions(dsts: var) : var {
        return timelineResolver.usesActiveOptionsFor(dsts);
    }

    function activeOptionsForElementDsts(dsts: var) : var {
        if (!root.dstsUseActiveOptions(dsts)) {
            return root.emptyActiveOptions;
        }
        return root.activeOptionsForDsts(dsts, root.runtimeActiveOptions);
    }

    function dstsUseSelectPanelTimer(dsts: var) : var {
        const timer = timelineResolver.firstTimerFor(dsts);
        return (timer >= 21 && timer <= 26)
            || (timer >= 31 && timer <= 36);
    }

    function elementUsesLiveDstClock(dsts: var) : var {
        return root.effectiveScreenKey === "select"
            && (root.dstsUseSelectPanelTimer(dsts)
                || timelineResolver.loopsContinuouslyFor(dsts));
    }

    function elementUsesLiveSourceClock(src: var) : var {
        return root.effectiveScreenKey === "select"
            && timelineResolver.sourceCyclesContinuously(src);
    }

    function elementUsesLiveSelectClock(src: var, dsts: var) : var {
        return root.elementUsesLiveDstClock(dsts)
            || root.elementUsesLiveSourceClock(src);
    }

    function skinTimeForElement(src: var, dsts: var) : var {
        return root.elementUsesLiveSelectClock(src, dsts)
            ? root.selectSourceSkinTime
            : root.renderSkinTime;
    }

    function noteFieldUsesActiveOptions() : var {
        return root.skinRuntimeRef
            ? root.skinRuntimeRef.noteFieldUsesActiveOptions()
            : false;
    }

    function noteFieldUsesTimers() : var {
        return root.skinRuntimeRef
            ? root.skinRuntimeRef.noteFieldUsesTimers()
            : false;
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
        options.__key = undefined;
        lookup[option] = true;
    }

    function skinUsesOption(option: var) : var {
        return !root.usedOptionFilterActive || !!root.usedOptionLookup[Math.abs(option)];
    }

    function skinUsesAnyOption(options: var) : var {
        if (!root.usedOptionFilterActive) {
            return true;
        }
        for (let i = 0; i < options.length; ++i) {
            if (root.usedOptionLookup[Math.abs(options[i])]) {
                return true;
            }
        }
        return false;
    }

    function skinUsesOptionRange(first: var, last: var) : var {
        if (!root.usedOptionFilterActive) {
            return true;
        }
        for (let option = first; option <= last; ++option) {
            if (root.usedOptionLookup[option]) {
                return true;
            }
        }
        return false;
    }

    function skinUsesSelectElementOption(option: var) : var {
        if (!root.usedElementOptionFilterAvailable) {
            return root.skinUsesOption(option);
        }
        return !!root.usedElementOptionLookup[Math.abs(option)];
    }

    function skinUsesAnySelectElementOption(options: var) : var {
        if (!root.usedElementOptionFilterAvailable) {
            return root.skinUsesAnyOption(options);
        }
        for (let i = 0; i < options.length; ++i) {
            if (root.usedElementOptionLookup[Math.abs(options[i])]) {
                return true;
            }
        }
        return false;
    }

    function skinUsesSelectElementOptionRange(first: var, last: var) : var {
        if (!root.usedElementOptionFilterAvailable) {
            return root.skinUsesOptionRange(first, last);
        }
        for (let option = first; option <= last; ++option) {
            if (root.usedElementOptionLookup[option]) {
                return true;
            }
        }
        return false;
    }

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

    function activeGaugeNameForSide(side: var) : var {
        if (root.isGameplayScreen()) {
            let activeName = root.gameplayActiveGaugeName(root.gameplayScore(side));
            if (activeName.length > 0) {
                return activeName;
            }
        }
        return root.configuredGaugeName(side);
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
        return !!(Rg.profileList && Rg.profileList.battleActive);
    }

    function spToDpActive() : var {
        let vars = root.mainGeneralVars();
        return !!vars && vars.dpOptions === DpOptions.Battle;
    }

    function laneCoverNumber(side: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return 0;
        }
        if (root.isGameplayScreen()) {
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
        if (!root.isGameplayScreen()) {
            return 0;
        }
        let vars = root.generalVarsForSide(side);
        return vars && vars.liftOn
            ? -root.cachedGameplayLaneOffsetHeight(side) * root.lr2Clamp01(vars.liftRatio)
            : 0;
    }

    function computeGameplayDstOffsetLaneCoverY(side: var) : var {
        if (!root.isGameplayScreen()) {
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
        if (!root.isGameplayScreen()) {
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
        if (!root.isGameplayScreen()) {
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

    function gameplayDstOffsetLiftY(side: var) : var {
        return side === 2 ? root.gameplayDstOffsetLiftY2 : root.gameplayDstOffsetLiftY1;
    }

    function gameplayDstOffsetLaneCoverY(side: var) : var {
        return side === 2 ? root.gameplayDstOffsetLaneCoverY2 : root.gameplayDstOffsetLaneCoverY1;
    }

    function gameplayDstOffsetHiddenY(side: var) : var {
        return side === 2 ? root.gameplayDstOffsetHiddenY2 : root.gameplayDstOffsetHiddenY1;
    }

    function gameplayDstOffsetHiddenA(side: var) : var {
        return side === 2 ? root.gameplayDstOffsetHiddenA2 : root.gameplayDstOffsetHiddenA1;
    }

    function normalizedDstOffsetSide(sideHint: var) : var {
        return sideHint === 2 ? 2 : 1;
    }

    function copyTimelineStateForDstOffsets(timelineState: var) : var {
        return {
            x: timelineState.x || 0,
            y: timelineState.y || 0,
            w: timelineState.w || 0,
            h: timelineState.h || 0,
            a: timelineState.a === undefined ? 255 : timelineState.a,
            r: timelineState.r === undefined ? 255 : timelineState.r,
            g: timelineState.g === undefined ? 255 : timelineState.g,
            b: timelineState.b === undefined ? 255 : timelineState.b,
            angle: timelineState.angle || 0,
            center: timelineState.center || 0,
            sortId: timelineState.sortId || 0,
            blend: timelineState.blend || 0,
            filter: timelineState.filter || 0,
            op1: timelineState.op1 || 0,
            op2: timelineState.op2 || 0,
            op3: timelineState.op3 || 0,
            op4: timelineState.op4 || 0
        };
    }

    function applyLr2DstOffsets(timelineState: var, dsts: var, sideHint: var) : var {
        if (!timelineState || !dsts || dsts.length === 0 || !dsts[0] || !dsts[0].offsets || dsts[0].offsets.length === 0) {
            return timelineState;
        }
        let side = root.normalizedDstOffsetSide(sideHint || 0);
        let liftY = root.gameplayDstOffsetLiftY(side);
        let laneCoverY = root.gameplayDstOffsetLaneCoverY(side);
        let hiddenY = root.gameplayDstOffsetHiddenY(side);
        let hiddenA = root.gameplayDstOffsetHiddenA(side);
        let adjustedState = root.copyTimelineStateForDstOffsets(timelineState);
        let offsets = dsts[0].offsets;
        for (let i = 0; i < offsets.length; ++i) {
            let id = Number(offsets[i]);
            let offsetY = 0;
            let offsetA = 0;
            if (id === 3 || id === 50) {
                offsetY = liftY;
            } else if (id === 4 || id === 51) {
                offsetY = laneCoverY;
            } else if (id === 5) {
                offsetY = hiddenY;
                offsetA = hiddenA;
            }
            adjustedState.y += offsetY;
            adjustedState.a += offsetA;
        }
        return adjustedState;
    }

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

    function isGameplayScreen() : var {
        return root.gameplayScreenActive;
    }

    function isResultScreen() : var {
        return root.resultScreenActive;
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

    function gameplayProfiles() : var {
        return [
            root.chart && root.chart.player1 ? root.chart.player1.profile : null,
            root.chart && root.chart.player2 ? root.chart.player2.profile : null
        ];
    }

    function stopGameplayLifecycle() : void {
        root.gameplayStartArmed = false;
        skinTiming.stopGameplayStartTimer();
        gameplayReadySound.stop();
        gameplayStopSound.stop();
    }

    function startGameplayWhenReady() : var {
        if (!root.enabled
                || !root.isGameplayScreen()
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
            skinTiming.restartGameplayStartTimer();
        }
    }

    function activateGameplayIfNeeded() : var {
        if (!root.enabled
                || !root.isGameplayScreen()
                || !root.chart
                || StackView.status !== StackView.Active) {
            return;
        }

        root.forceActiveFocus();
        if (root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
            if (root.isCourseGameplay() && !root.gameplayShowedCourseResult) {
                root.gameplayShowedCourseResult = true;
                let profiles = root.gameplayProfiles();
                let chartDatas = root.chart.chartDatas;
                let course = root.chart.course;
                Qt.callLater(() => {
                    if (root.enabled && root.isGameplayScreen() && root.chart && root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
                        globalRoot.openCourseResult(root.chart.finish(), profiles, chartDatas, course);
                    }
                });
            } else {
                Qt.callLater(() => {
                    if (root.enabled && root.isGameplayScreen() && root.chart && root.chartStatusIs(root.chart.status, ChartRunner.Finished)) {
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
                || !root.isGameplayScreen()
                || !root.chart
                || root.gameplayResultOpened) {
            return;
        }

        root.gameplayResultOpened = true;
        root.gameplayStartArmed = false;
        skinTiming.stopGameplayStartTimer();

        let chartData = root.gameplayChartData();
        let profiles = root.gameplayProfiles();
        let scores = root.chart instanceof ChartRunner ? root.chart.finish() : root.chart.proceed();
        globalRoot.openResult(scores, profiles, chartData);
    }

    function handleGameplayStatusChanged() : var {
        root.updateGameplayStatusTimers();
        if (!root.enabled || !root.isGameplayScreen() || !root.chart) {
            return;
        }
        if (root.chartStatusIs(root.chart.status, ChartRunner.Ready)) {
            root.startGameplayWhenReady();
        } else if (root.chartStatusIs(root.chart.status, ChartRunner.Running)) {
            root.gameplayStartArmed = false;
            skinTiming.stopGameplayStartTimer();
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
            return selectContext.selectedChartData();
        }
        return resultState.displayChartData();
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
    function resultGaugeValue(side: var) : var { return resultState.resultGaugeValue(side); }
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
        if (!root.chart) {
            return null;
        }
        return side === 2 ? root.chart.player2 : root.chart.player1;
    }

    function gameplayKeymode() : var {
        let chartData = root.gameplayChartData();
        return chartData ? chartData.keymode || 0 : 0;
    }

    function gameplayUsesDoublePlayLanes() : var {
        let keymode = root.gameplayKeymode();
        return keymode === 10 || keymode === 14;
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

    function gameplayTargetSavedScore() : var {
        let player = root.gameplayPlayer(1);
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        switch (vars ? vars.scoreTarget : ScoreTarget.BestScore) {
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

    function gameplayHighScorePoints() : var {
        root.gameplayScoresRevision;
        return root.gameplayBestSavedScore() ? Math.floor(gameplayBestScoreReplayer.points || 0) : 0;
    }

    function gameplayTargetScorePoints() : var {
        root.gameplayScoresRevision;
        if (root.battleModeActive()) {
            return root.gameplayExScore(root.gameplayScore(2));
        }
        if (root.gameplayTargetSavedScore()) {
            return Math.floor(gameplayTargetScoreReplayer.points || 0);
        }
        let score = root.gameplayScore(1);
        return score ? Math.floor((score.maxPointsNow || 0) * root.gameplayTargetFraction()) : 0;
    }

    function gameplayTargetFinalPoints() : var {
        if (root.battleModeActive()) {
            return 0;
        }
        let targetScore = root.gameplayTargetSavedScore();
        if (targetScore) {
            return root.gameplaySavedScorePoints(targetScore);
        }
        let score = root.gameplayScore(1);
        return score ? Math.floor((score.maxPoints || 0) * root.gameplayTargetFraction()) : 0;
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

        if (!root.isGameplayScreen()) {
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

    function gameplayPoorBgaOption(side: var, baseOption: var) : var {
        let skinTime = side === 2 ? root.gameplayLastMissSkinTime2 : root.gameplayLastMissSkinTime1;
        return skinTime >= 0 && root.renderSkinTime - skinTime < 1000 ? baseOption + 1 : baseOption;
    }

    function gameplayPoorBgaVisible() : var {
        root.gameplayRevision;
        return root.gameplayPoorBgaOption(1, 247) === 248;
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
        return root.isGameplayScreen()
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
        if (!root.isGameplayScreen()) {
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
        root.flushGameplayRuntimeRefresh();
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

        if (refreshTimers && skinTiming.commitGameplayTimerChanges()) {
            root.bumpGameplayTimerRevision();
        }
        if (refreshNumbers1) {
            root.bumpGameplayNumberRevision(1);
        }
        if (refreshNumbers2) {
            root.bumpGameplayNumberRevision(2);
        }
        if (refreshGameplay && root.isGameplayScreen()) {
            root.bumpGameplayRevision();
            root.refreshGameplayRuntimeActiveOptions();
        }
    }

    function updateGameplayStaticNumberRevision() : void {
        root.gameplayStaticNumberRevision++;
        let chartData = root.gameplayChartData();
        let chartKey = chartData ? (chartData.md5 || chartData.sha256 || chartData.title || "") : "";
        root.gameplayStaticNumberRevisionKey = "static|" + root.gameplayStaticNumberRevision + "|" + chartKey;
    }

    function gameplayNumberRevisionKind(src: var) : var {
        if (!root.gameplayScreenActive) {
            return 0;
        }
        if (src && src.nowCombo) {
            let nowComboSide = src.side || (src.timer === 47 ? 2 : 1);
            return nowComboSide === 2 ? 7 : 6;
        }

        let num = src ? (src.num || 0) : 0;
        if (num === 20 || (num >= 160 && num <= 164)) {
            return 4;
        }
        if (num === 11 || num === 15) {
            return 2;
        }
        if (num === 10 || num === 12 || num === 13 || num === 14
                || (num >= 310 && num <= 315)) {
            return 1;
        }
        if (num === 108 || num === 128 || (num >= 150 && num <= 158)) {
            return 3;
        }
        if ((num >= 120 && num <= 136) || num === 526 || num === 521
                || (num >= 510 && num <= 519) || (num >= 1610 && num <= 1699)) {
            return 2;
        }
        if ((num >= 100 && num <= 116) || num === 407
                || (num >= 410 && num <= 427)
                || (num >= 500 && num <= 509) || num === 520 || num === 522
                || num === 525 || num === 527 || (num >= 1510 && num <= 1599)) {
            return 1;
        }
        if (num === 42 || num === 90 || num === 91 || num === 92
                || num === 106 || num === 126 || num === 165
                || num === 290 || num === 291
                || (num >= 350 && num <= 365) || num === 368
                || num === 1163 || num === 1164) {
            return 5;
        }
        return 3;
    }

    function gameplayNumberRevisionForKind(kind: var) : var {
        switch (kind) {
        case 1:
            return "p1|" + root.gameplayNumberRevision1 + "|" + root.gameplayJudgeRevision1;
        case 2:
            return "p2|" + root.gameplayNumberRevision2 + "|" + root.gameplayJudgeRevision2;
        case 3:
            return "x|" + root.gameplayNumberRevision1 + "|" + root.gameplayNumberRevision2
                + "|" + root.gameplayScoresRevision;
        case 4:
            return "time|" + root.renderSkinTime;
        case 5:
            return root.gameplayStaticNumberRevisionKey;
        case 6:
            return "j1|" + root.gameplayJudgeRevision1;
        case 7:
            return "j2|" + root.gameplayJudgeRevision2;
        default:
            return 0;
        }
    }

    function setGameplayTimerValue(timer: var, skinTime: var) : var {
        if (!timer || skinTime < 0) {
            return;
        }
        if (skinTiming.setGameplayTimerValue(timer, skinTime)) {
            root.requestGameplayTimerRevision();
        }
    }

    function clearGameplayTimerValue(timer: var) : var {
        if (!timer) {
            return;
        }
        if (skinTiming.clearGameplayTimerValue(timer)) {
            root.requestGameplayTimerRevision();
        }
    }

    function resetGameplayTimerValues() : void {
        if (skinTiming.resetGameplayTimerValues()) {
            root.gameplayTimerRevision++;
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
    }

    function updateGameplayStatusTimers() : var {
        if (!root.isGameplayScreen() || !root.chart || root.chart.status === undefined) {
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
            return column >= 0 && column <= 6 ? column + 11 : -1;
        }
        let rightSide = side === 2 || (!root.gameplayPlayer(2) && column >= 8);
        if (rightSide) {
            if (column === 15) {
                return 10;
            }
            return column >= 8 && column <= 14 ? column + 3 : -1;
        }
        if (column === 7) {
            return 0;
        }
        return column >= 0 && column <= 6 ? column + 1 : -1;
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
            skinTiming.restartGameplayPoorBgaOptionTimer();
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

    function gameplayRhythmTimerSkinTime() : var {
        if (root.isGameplayScreen()) {
            return gameplayFrameState.rhythmTimerSkinTime;
        }
        let player = root.gameplayPlayer(1) || root.gameplayPlayer(2);
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

    function resetGameplayFrameSamples() : void {
        gameplayFrameState.reset();
    }

    function refreshGameplayFrameSamples(frameSkinTime: var) : void {
        let sampleSkinTime = frameSkinTime === undefined ? root.renderSkinTime : frameSkinTime;
        gameplayFrameState.refresh(sampleSkinTime);
    }

    function addGameplayTimers(result: var) : var {
        if (!root.isGameplayScreen()) {
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
        root.addGameplayTimer(result, 140, root.gameplayRhythmTimerSkinTime());
        root.addGameplayKeyTimers(result);
        root.addGameplayEffectTimers(result);
    }

    function addGameplayEffectTimers(result: var) : void {
        for (let keyName in root.gameplayHitTimerStarts) {
            result[keyName] = root.gameplayHitTimerStarts[keyName];
        }
        for (let keyName in root.gameplayLongNoteTimerStarts) {
            result[keyName] = root.gameplayLongNoteTimerStarts[keyName];
        }
    }

    function initialGameplayOffButtonTimers() : var {
        let result = {};
        for (let timer = 120; timer <= 127; ++timer) {
            result[timer] = 0;
        }
        for (let timer = 130; timer <= 137; ++timer) {
            result[timer] = 0;
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
            return 106;
        case BmsKey.Col17:
            return 107;
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
            return 116;
        case BmsKey.Col27:
            return 117;
        default:
            return 0;
        }
    }

    function gameplayKeyOffTimerForOnTimer(timer: var) : var {
        return ((timer >= 100 && timer <= 107) || (timer >= 110 && timer <= 117))
            ? timer + 20
            : 0;
    }

    function gameplayKeyTimerHeld(timer: var) : var {
        let columnState = root.gameplayColumnStateForKeyTimer(timer);
        return !!columnState && !!columnState.pressed;
    }

    function setGameplayKeyTimerPressed(timer: var, pressed: var) : var {
        if (!root.isGameplayScreen() || !root.chart) {
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
        if (!root.isGameplayScreen() || !root.chart) {
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
        if (!root.isGameplayScreen() || !root.chart) {
            return;
        }

        for (let timer of root.gameplayKeyTimers) {
            root.syncGameplayKeyTimerFromColumnState(timer, root.gameplayColumnStateForKeyTimer(timer));
        }
    }

    function syncGameplayLongNoteTimersFromColumns() : var {
        if (!root.isGameplayScreen() || !root.chart) {
            return;
        }

        for (let timer of root.gameplayLongNoteTimers) {
            root.syncGameplayLongNoteTimerFromColumnState(
                timer,
                root.gameplayColumnStateForLongNoteTimer(timer));
        }
    }

    function pressGameplayButtonTimer(key: var) : var {
        if (!root.isGameplayScreen()) {
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
            result[keyName] = root.gameplayHeldButtonTimerStarts[keyName];
        }
        for (let keyName in root.gameplayOffButtonTimerStarts) {
            let onTimer = Number(keyName) - 20;
            if (!root.gameplayPreviousPressedTimers[onTimer]) {
                result[keyName] = root.gameplayOffButtonTimerStarts[keyName];
            }
        }
    }

    Lr2RuntimeOptionBuilder {
        id: runtimeOptions
        host: root
        selectContext: selectContext
        skinModel: skinModel
    }

    readonly property var builtBarRuntimeActiveOptions: runtimeOptions.buildBarActiveOptions()
    readonly property var builtBaseRuntimeActiveOptions: runtimeOptions.buildBaseActiveOptions(root.builtBarRuntimeActiveOptions)
    readonly property var builtSelectCommonRuntimeActiveOptions: runtimeOptions.buildSelectCommonActiveOptions(root.builtBaseRuntimeActiveOptions)
    property var builtSelectRuntimeActiveOptions: []
    property var builtScreenRuntimeActiveOptions: []

    property var barActiveOptions: []
    property var baseActiveOptions: []
    property string baseActiveOptionsKey: ""
    property var selectCommonActiveOptions: []
    property string selectCommonActiveOptionsKey: ""
    property bool selectCommonActiveOptionsReady: false
    property string selectRuntimeActiveOptionsKey: ""
    property string gameplayRuntimeActiveOptionsStateKey: ""
    property string gameplayRuntimeActiveOptionsKey: ""
    property var runtimeActiveOptions: []
    readonly property var barTimers: ({ "0": 0 })

    function sameNumberArray(a: var, b: var) : var {
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

    function numberArrayKey(values: var) : var {
        if (!values || values.length === 0) {
            return "";
        }
        if (values.__key !== undefined) {
            return values.__key;
        }
        values.__key = values.join(",");
        return values.__key;
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
            if (root.activeOptionPresent(id1, activeOptions)) {
                idA = id1;
                count = 1;
            }
        }
        if (op2 !== 0) {
            let id2 = Math.abs(op2);
            if (root.activeOptionPresent(id2, activeOptions)
                    && (count === 0 || id2 !== idA)) {
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
            if (root.activeOptionPresent(id3, activeOptions)
                    && (count === 0 || id3 !== idA)
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

    function refreshBaseActiveOptions() : var {
        return selectUpdateController.refreshBaseActiveOptions();
    }

    function updateGeneratedSelectRuntimeActiveOptions() : void {
        let next = root.effectiveScreenKey === "select"
            ? runtimeOptions.buildSelectRuntimeActiveOptions(root.builtSelectCommonRuntimeActiveOptions)
            : [];
        if (!root.sameNumberArray(root.builtSelectRuntimeActiveOptions, next)) {
            root.builtSelectRuntimeActiveOptions = next;
        }
    }

    function updateGeneratedScreenRuntimeActiveOptions() : void {
        let next = root.effectiveScreenKey === "select"
            ? []
            : runtimeOptions.buildRuntimeActiveOptions(root.builtBaseRuntimeActiveOptions);
        if (!root.sameNumberArray(root.builtScreenRuntimeActiveOptions, next)) {
            root.builtScreenRuntimeActiveOptions = next;
        }
    }

    function refreshSelectRuntimeActiveOptions() : void {
        root.updateGeneratedSelectRuntimeActiveOptions();
        root.updateGeneratedScreenRuntimeActiveOptions();
        selectUpdateController.refreshSelectRuntimeActiveOptions();
    }

    function refreshSelectRankingStatusOptions() : void {
        if (root.selectUsesRankingStatusOptions()) {
            root.refreshSelectRuntimeActiveOptions();
        }
    }

    function refreshSelectPlayOptionLayout() : void {
        if (root.effectiveScreenKey !== "select") {
            return;
        }
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }

    function appendGameplayRuntimeOptionSideKeyParts(parts: var, side: var) : void {
        let score = root.gameplayScore(side);
        let gaugeValue = Math.floor(root.gameplayGaugeValue(score));
        let gaugeBucket = gaugeValue >= 100 ? 10 : Math.max(0, Math.floor(gaugeValue / 10));
        let judgementOption = root.gameplayJudgementOption(side, side === 2 ? 261 : 241);
        let timing = side === 2 ? root.gameplayLastJudgeTiming2 : root.gameplayLastJudgeTiming1;
        let vars = root.generalVarsForSide(side);

        parts.push(
            side,
            root.gameplayGaugeOption(side),
            root.gameplayLaneOption(score),
            root.gameplayLaneCoverOption(side),
            side === 1 && vars && vars.laneCoverOn ? 1 : 0,
            side === 1 && vars && vars.liftOn ? 1 : 0,
            side === 1 && vars && vars.hiddenOn ? 1 : 0,
            root.gameplayRankOption(score, side === 2 ? 210 : 200, true),
            side === 1 ? root.gameplayRankOption(score, 220, false) : -1,
            root.gameplayExactRankOption(score, side === 2 ? 310 : 300),
            side === 1 && root.gameplayGaugeQualified(score) ? 1 : 0,
            gaugeBucket,
            judgementOption,
            timing !== 0 && judgementOption >= 0 && judgementOption !== (side === 2 ? 261 : 241)
                ? (timing > 0 ? 1 : -1)
                : 0,
            root.gameplayPoorBgaOption(side, side === 2 ? 267 : 247));
    }

    function gameplayRuntimeOptionStateKey() : var {
        if (!root.isGameplayScreen()) {
            return "";
        }

        let vars = root.mainGeneralVars();
        let score1 = root.gameplayScore(1);
        let chartData = root.gameplayChartData();
        let chartCount = root.chart && root.chart.chartDatas ? root.chart.chartDatas.length || 0 : 0;
        let chartIndex = root.chart && root.chart.currentChartIndex !== undefined
            ? Math.max(0, root.chart.currentChartIndex || 0)
            : 0;
        let parts = [
            root.effectiveScreenKey,
            root.baseActiveOptionsKey,
            vars ? vars.bgaSize || 0 : 0,
            vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0,
            vars && vars.scoreGraphEnabled === false ? 0 : 1,
            vars && vars.bgaOn === false ? 0 : 1,
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
            chartData ? chartData.keymode || 0 : 0,
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
            chartData && selectContext.hasReplay(chartData) ? 1 : 0,
            root.gameplayLaneCoverChangingOptionActive() ? 1 : 0
        ];

        root.appendGameplayRuntimeOptionSideKeyParts(parts, 1);
        if (root.gameplayLanePlayer(2)) {
            root.appendGameplayRuntimeOptionSideKeyParts(parts, 2);
        }

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
        return parts.join("|");
    }

    function refreshGameplayRuntimeActiveOptions() : var {
        let nextKey = root.gameplayRuntimeOptionStateKey();
        if (nextKey === root.gameplayRuntimeActiveOptionsStateKey) {
            return false;
        }
        root.gameplayRuntimeActiveOptionsStateKey = nextKey;
        root.updateGeneratedScreenRuntimeActiveOptions();
        return selectUpdateController.refreshGameplayRuntimeActiveOptions();
    }

    function selectChartRevisionKey(focusRevision: var, scoreRevision: var, listRevision: var, chartRevision: var) : var {
        return focusRevision + "|" + scoreRevision + "|" + listRevision + "|" + chartRevision;
    }

    property alias selectRevision: selectUpdateController.selectRevision
    property alias selectDetailRevision: selectUpdateController.selectDetailRevision
    readonly property string selectChartContentRevision: root.effectiveScreenKey === "select"
        ? root.selectChartRevisionKey(
            selectContext.focusRevision,
            selectContext.scoreRevision,
            selectContext.listRevision,
            selectContext.visualChartContentRevision)
        : ""
    readonly property string selectChartWrapperContentRevision: root.effectiveScreenKey === "select"
        ? selectContext.visualChartContentRevision
        : ""
    readonly property var selectChartWrapperState: root.effectiveScreenKey === "select"
        ? ({
            chartRevision: root.selectChartContentRevision,
            wrapperRevision: root.selectChartWrapperContentRevision,
            wrapper: selectContext.visualChartWrapper
        })
        : null
    readonly property var selectSkinChartWrapper: root.selectChartWrapperState
        ? root.selectChartWrapperState.wrapper
        : null
    readonly property int selectRuntimeKeymode: root.effectiveScreenKey === "select"
        ? runtimeOptions.chartKeymode(selectContext.selectedChartData(), selectContext.selectedItem())
        : 0
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? root.selectSkinChartWrapper
        : selectSideEffects.renderChart
    readonly property var visualSelectChart: root.effectiveScreenKey === "select"
        ? root.selectSkinChartWrapper
        : root.renderChart
    readonly property var selectedCourseStages: {
        if (root.effectiveScreenKey === "select") {
            selectContext.focusRevision;
        }
        if (root.effectiveScreenKey === "courseResult") {
            if (root.chartDatas && root.chartDatas.length > 0) {
                return root.chartDatas;
            }
            if (root.course && root.course.loadCharts) {
                return root.course.loadCharts();
            }
        }
        let item = root.effectiveScreenKey === "select" ? selectContext.selectedItem() : null;
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
        function onRankingModeChanged() : void {
            if (root.refreshBaseActiveOptions()) {
                root.refreshSelectRuntimeActiveOptions();
            }
        }
    }
    Connections {
        target: lr2Ranking.rankingModel
        function onLoadingChanged() : void { root.refreshSelectRankingStatusOptions(); }
        function onRankingEntriesChanged() : void { root.refreshSelectRankingStatusOptions(); }
        function onPlayerCountChanged() : void { root.refreshSelectRankingStatusOptions(); }
        function onScoreCountChanged() : void { root.refreshSelectRankingStatusOptions(); }
        function onClearCountsChanged() : void { root.refreshSelectRankingStatusOptions(); }
        function onMd5Changed() : void { root.refreshSelectRankingStatusOptions(); }
    }
    Connections {
        target: Rg.profileList
        function onBattleActiveChanged() : void {
            root.refreshSelectPlayOptionLayout();
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
        target: root.mainGeneralVars()
        ignoreUnknownSignals: true
        function onDpOptionsChanged() : void {
            root.refreshSelectPlayOptionLayout();
        }
    }
    onSelectPanelChanged: {
        if (root.refreshBaseActiveOptions()) {
            root.refreshSelectRuntimeActiveOptions();
        }
    }
    onLr2RankingTransitionPhaseChanged: {
        if (root.refreshBaseActiveOptions()) {
            root.refreshSelectRuntimeActiveOptions();
        }
    }
    onParseActiveOptionsChanged: {
        if (root.refreshBaseActiveOptions()) {
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
        if (root.isGameplayScreen()) {
            root.gameplayResultOpened = false;
            root.gameplayShowedCourseResult = false;
            root.gameplayPlayStopped = false;
            root.gameplayNothingWasHit = true;
            root.resetGameplayTimers();
            root.refreshGameplayRuntimeActiveOptions();
            root.updateGameplayStatusTimers();
            root.activateGameplayIfNeeded();
        }
        if (root.isResultScreen()) {
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
        target: root.isGameplayScreen() ? root.chart : null
        ignoreUnknownSignals: true
        function onCurrentChartIndexChanged() : void {
            root.gameplayRevision++;
            root.bumpGameplayNumberRevision(0);
            root.updateGameplayStaticNumberRevision();
            root.refreshGameplayRuntimeActiveOptions();
            root.gameplayResultOpened = false;
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
        target: root.isGameplayScreen() ? root.gameplayScore(1) : null
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
        target: root.isGameplayScreen() ? root.gameplayScore(2) : null
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
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        root.restartSelectInfoTimer();
        if (rankingRequestChanged) {
            root.applyRankingStatsToSelectContext();
        }
        selectSideEffects.update();
    }

    function updateSelectSideEffects() : void {
        selectSideEffects.update();
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

    function optionText(labels: var, index: var) : var {
        return valueResolver.optionText(labels, index);
    }

    function clearLabelForLamp(lamp: var) : var {
        return valueResolver.clearLabelForLamp(lamp);
    }

    function courseStage(index: var) : var {
        return valueResolver.courseStage(index);
    }

    function chartTitle(chart: var) : var {
        return valueResolver.chartTitle(chart);
    }

    function chartSubtitle(chart: var) : var {
        return valueResolver.chartSubtitle(chart);
    }

    function lr2SelectOptionText(st: var) : var {
        return valueResolver.lr2SelectOptionText(st);
    }

    function resolveText(st: var) : var {
        return valueResolver.resolveText(st);
    }

    function resolveGameplayNumber(num: var) : var {
        return valueResolver.resolveGameplayNumber(num);
    }

    function resultCompareResult() : var {
        return valueResolver.resultCompareResult();
    }

    function resolveResultSideNumber(num: var, result: var) : var {
        return valueResolver.resolveResultSideNumber(num, result);
    }

    function resolveResultTargetSideNumber(num: var, side: var) : var {
        return valueResolver.resolveResultTargetSideNumber(num, side);
    }

    function resolveResultNumber(num: var) : var {
        return valueResolver.resolveResultNumber(num);
    }

    function resolveNumber(num: var) : var {
        return valueResolver.resolveNumber(num);
    }

    function numberValue(src: var) : var {
        return valueResolver.numberValue(src);
    }

    function imageSetValue(imageSetRef: var, sourceCount: var) : var {
        return valueResolver.imageSetValue(imageSetRef, sourceCount);
    }

    function imageSetSourceFor(src: var) : var {
        return valueResolver.imageSetSourceFor(src);
    }

    function numberForceHidden(src: var) : var {
        return valueResolver.numberForceHidden(src);
    }

    function numberAnimationRevision(src: var) : var {
        return valueResolver.numberAnimationRevision(src);
    }

    function resolveBarGraph(type: var) : var {
        return valueResolver.resolveBarGraph(type);
    }

    function normalizedBarValue(value: var, maximum: var) : var {
        return valueResolver.normalizedBarValue(value, maximum);
    }

    function resultBarGraphValue(type: var) : var {
        return valueResolver.resultBarGraphValue(type);
    }

    function barDistributionGraphSourceAnimates(src: var) : var {
        if (!src || (src.cycle || 0) <= 0) {
            return false;
        }
        let segments = (src.graphType || 0) === 0 ? 11 : 28;
        let frames = Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1);
        return Math.floor(frames / segments) > 1;
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

    function buttonUsesSplitArrows(buttonId: var) : var {
        return selectPanelController.buttonUsesSplitArrows(buttonId);
    }

    function imageSetButtonId(src: var) : var {
        return selectPanelController.imageSetButtonId(src);
    }

    function elementButtonId(src: var) : var {
        return selectPanelController.elementButtonId(src);
    }

    function elementButtonPanel(src: var) : var {
        return selectPanelController.elementButtonPanel(src);
    }

    function elementButtonClickEnabled(src: var) : var {
        return selectPanelController.elementButtonClickEnabled(src);
    }

    function elementButtonPanelMatches(src: var) : var {
        return selectPanelController.elementButtonPanelMatches(src);
    }

    function buttonMouseDelta(src: var, mouseX: var, width: var) : var {
        return selectPanelController.buttonMouseDelta(src, mouseX, width);
    }

    function buttonFrame(src: var) : var {
        return selectPanelController.buttonFrame(src);
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

    function currentSelectHeldButtonSkinTime() : var {
        return selectPanelController.currentSelectHeldButtonSkinTime();
    }

    function selectHeldButtonTimerForKey(key: var) : var {
        return selectPanelController.selectHeldButtonTimerForKey(key);
    }

    function isSelectHeldButtonTimer(timer: var) : var {
        return selectPanelController.isSelectHeldButtonTimer(timer);
    }

    function pressSelectHeldButtonTimer(key: var) : var {
        return selectPanelController.pressSelectHeldButtonTimer(key);
    }

    function releaseSelectHeldButtonTimer(key: var) : var {
        return selectPanelController.releaseSelectHeldButtonTimer(key);
    }

    function addHeldButtonTimer(result: var, timer: var, held: var) : var {
        return selectPanelController.addHeldButtonTimer(result, timer, held);
    }

    function addHeldButtonTimers(result: var) : var {
        return selectPanelController.addHeldButtonTimers(result);
    }

    function selectHeldButtonTimerFireTime(timer: var, liveClock: var) : var {
        return selectPanelController.selectHeldButtonTimerFireTime(timer, liveClock);
    }

    function spriteSkinTime(src: var, dsts: var) : var {
        return selectPanelController.spriteSkinTime(src, dsts);
    }

    function buttonPanelMatches(src: var) : var {
        return selectPanelController.buttonPanelMatches(src);
    }

    function handleLr2Button(buttonId: var, delta: var, panel: var, soundPlayer: var, sourceCount: var) : var {
        return selectPanelController.handleLr2Button(buttonId, delta, panel, soundPlayer, sourceCount);
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

    function isNowJudgeSprite(src: var) : var {
        return root.isGameplayScreen()
            && src
            && (src.timer === 46 || src.timer === 47)
            && (src.w || 0) > 0
            && (src.h || 0) > 0
            && (src.op1 || 0) === 0;
    }

    function nowJudgeComboValue(src: var) : var {
        if (!src) {
            return 0;
        }
        return src.timer === 47 ? root.gameplayJudgeCombo2 : root.gameplayJudgeCombo1;
    }

    function nowJudgeOffsetX(src: var, dsts: var) : var {
        if (!root.isNowJudgeSprite(src)) {
            return 0;
        }
        let combo = root.nowJudgeComboValue(src);
        if (combo <= 0) {
            return 0;
        }

        let digits = Math.abs(Math.round(combo)).toString().length;
        let dst = dsts && dsts.length > 0 ? dsts[0] : null;
        let baseH = dst && dst.h ? Math.abs(dst.h) : (src.h || 30);
        let comboDigitW = Math.max(1, Math.round(baseH * 22 / 30));
        return -digits * comboDigitW * 0.5;
    }

    readonly property int selectScrollSpriteStateOverride: 1
    readonly property int gameplayProgressSpriteStateOverride: 2
    readonly property int gameplayLaneCoverSpriteStateOverride: 3
    readonly property int numberRefSpriteStateOverride: 4
    readonly property int genericSliderSpriteStateOverride: 5

    function spriteSliderPositionForKind(kind: var, src: var) : var {
        switch (kind) {
        case root.selectScrollSpriteStateOverride:
            return skinSliderState.selectScrollPosition(src);
        case root.gameplayProgressSpriteStateOverride:
            return skinSliderState.gameplayProgressPosition(src);
        case root.gameplayLaneCoverSpriteStateOverride:
            return skinSliderState.gameplayLaneCoverPosition(src);
        case root.numberRefSpriteStateOverride:
            return skinSliderState.numberRefSliderPosition(src);
        case root.genericSliderSpriteStateOverride:
            return skinSliderState.genericPosition(src);
        default:
            return 0;
        }
    }

    function spriteForceHidden(src: var, elementIndex: var) : var {
        if (src && src.onMouse) {
            root.selectHoverVisibleSignature;
            return root.selectHoverVisibleByIndex[String(elementIndex)] !== true;
        }
        return false;
    }

    function setSelectScrollFromSliderTrack(src: var, track: var, pointerX: var, pointerY: var) : void {
        skinSliderState.setSelectScrollFromTrack(src, track, pointerX, pointerY);
    }

    function setLr2GenericSliderFromTrack(src: var, track: var, pointerX: var, pointerY: var) : void {
        skinSliderState.setGenericFromTrack(src, track, pointerX, pointerY);
    }

    readonly property var barBaseStateResolver: selectBarGeometry.barBaseStateResolver
    readonly property var barPositionMap: selectBarGeometry.barPositionMap
    readonly property bool fastBarScrollActive: selectBarGeometry.fastBarScrollActive
    readonly property real fastBarScrollX: selectBarGeometry.fastBarScrollX
    readonly property real fastBarScrollY: selectBarGeometry.fastBarScrollY
    readonly property real selectedFastBarDrawX: selectBarGeometry.selectedFastBarDrawX
    readonly property real selectedFastBarDrawY: selectBarGeometry.selectedFastBarDrawY

    function selectedBarRow() : var {
        return selectBarGeometry.selectedBarRow();
    }

    function barClickStart() : var {
        return selectBarGeometry.barClickStart();
    }

    function barClickEnd() : var {
        return selectBarGeometry.barClickEnd();
    }

    function barRowCanClick(row: var) : var {
        return selectBarGeometry.barRowCanClick(row);
    }

    function barBaseStateAt(row: var) : var {
        return barBaseStateResolver ? barBaseStateResolver.stateAt(row) : null;
    }

    function barRowScrollDelta(row: var) : var {
        return selectBarGeometry.barRowScrollDelta(row);
    }

    function handleBarRowClick(row: var, mouse: var) : var {
        return selectBarGeometry.handleBarRowClick(row, mouse);
    }

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
    readonly property var timers: root.isResultScreen()
        ? skinTiming.timers
        : root.zeroTimers

    onRenderSkinTimeChanged: {
        if (root.effectiveScreenKey === "select"
                && root.selectHoverHasPoint
                && root.globalSkinTime < root.selectAnimationLimit) {
            root.refreshSelectHoverState();
        }
    }

    readonly property bool selectReplayOptionsUsed: root.skinUsesAnySelectElementOption([
            196, 197, 1196, 1197, 1199, 1200,
            1202, 1203, 1205, 1206, 1207, 1208
        ])
    readonly property bool selectScoreOptionIdsUsed: root.skinUsesSelectElementOptionRange(105, 130)
        || root.skinUsesAnySelectElementOption([144, 145, 1100, 1102, 1103, 1104, 1128])
    readonly property bool selectEntryStatusOptionsUsed: root.skinUsesSelectElementOptionRange(100, 130)
        || root.skinUsesSelectElementOptionRange(200, 207)
        || root.skinUsesAnySelectElementOption([1100, 1102, 1103, 1104])
    readonly property bool selectDifficultyBarOptionsUsed: root.skinUsesSelectElementOptionRange(70, 79)
        || root.skinUsesSelectElementOptionRange(500, 565)
    readonly property bool selectCourseDetailOptionsUsed: root.skinUsesAnySelectElementOption([290, 293])
        || root.skinUsesSelectElementOptionRange(580, 589)
        || root.skinUsesSelectElementOptionRange(700, 755)
    readonly property bool selectRankingStatusOptionsUsed: root.skinUsesSelectElementOptionRange(600, 616)

    function selectUsesReplayOptions() : var {
        return root.selectReplayOptionsUsed;
    }

    function selectUsesScoreOptionIds() : var {
        return root.selectScoreOptionIdsUsed;
    }

    function selectUsesEntryStatusOptions() : var {
        return root.selectEntryStatusOptionsUsed;
    }

    function selectUsesDifficultyBarOptions() : var {
        return root.selectDifficultyBarOptionsUsed;
    }

    function selectUsesCourseDetailOptions() : var {
        return root.selectCourseDetailOptionsUsed;
    }

    function selectUsesRankingStatusOptions() : var {
        return root.selectRankingStatusOptionsUsed;
    }

    function updateSelectAnimationLimits() : void {
        skinTiming.updateSelectAnimationLimits();
    }

    function restartSkinClock() : void {
        skinTiming.restartSkinClock();
    }

    function restartSelectInfoTimer() : void {
        skinTiming.restartSelectInfoTimer();
    }

    function quantizedSkinClock(now: var) : var {
        return skinTiming.quantizedSkinClock(now);
    }

    function gameplayTimerFireTime(timer: var) : var {
        return skinTiming.gameplayTimerFireTime(timer);
    }

    function resultTimerFireTime(timer: var) : var {
        return skinTiming.resultTimerFireTime(timer);
    }

    function selectTimerFireTime(timer: var, liveClock: var) : var {
        return skinTiming.selectTimerFireTime(timer, liveClock);
    }

    function selectTimerCanFire(timer: var) : var {
        return skinTiming.selectTimerCanFire(timer);
    }

    function skinTimerCanFire(timer: var) : var {
        return skinTiming.skinTimerCanFire(timer);
    }

    function skinTimerFireTime(timer: var, liveClock: var) : var {
        return skinTiming.skinTimerFireTime(timer, liveClock);
    }

    Lr2SelectContext {
        id: selectContext
        updatesActive: root.screenUpdatesActive && root.effectiveScreenKey === "select"
        enabled: updatesActive
        barTitleTypes: skinModel.barTitleTypes || []
        useBeatorajaBarTextTypes: root.lr2SkinUsesBeatorajaSemantics
        useBeatorajaSelectOptions: root.lr2SkinUsesBeatorajaSemantics
        barRowCount: skinModel.barRows ? skinModel.barRows.length : 0
        barCenter: skinModel.barCenter

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
        scoreRevision: selectContext.scoreRevision
        focusRevision: selectContext.focusRevision
        selectPanel: root.selectPanel
        lr2RankingMd5: root.lr2RankingMd5
        lr2RankingRequestMd5: root.lr2RankingRequestMd5
        parseActiveOptions: root.parseActiveOptions
        barRuntimeActiveOptions: root.builtBarRuntimeActiveOptions
        baseRuntimeActiveOptions: root.builtBaseRuntimeActiveOptions
        selectCommonRuntimeActiveOptions: root.builtSelectCommonRuntimeActiveOptions
        selectRuntimeGeneratedActiveOptions: root.builtSelectRuntimeActiveOptions
        screenRuntimeActiveOptions: root.builtScreenRuntimeActiveOptions
        gameplayScreen: root.isGameplayScreen()
        selectedKeymode: root.selectRuntimeKeymode
        spToDpActive: root.spToDpActive()
        battleModeActive: root.battleModeActive()
        onLr2RankingRequestMd5Changed: if (root.lr2RankingRequestMd5 !== lr2RankingRequestMd5) root.lr2RankingRequestMd5 = lr2RankingRequestMd5
        onRuntimeActiveOptionsChanged: if (root.runtimeActiveOptions !== runtimeActiveOptions) root.runtimeActiveOptions = runtimeActiveOptions
        onBarActiveOptionsChanged: if (root.barActiveOptions !== barActiveOptions) root.barActiveOptions = barActiveOptions
        onBaseActiveOptionsChanged: if (root.baseActiveOptions !== baseActiveOptions) root.baseActiveOptions = baseActiveOptions
        onBaseActiveOptionsKeyChanged: if (root.baseActiveOptionsKey !== baseActiveOptionsKey) root.baseActiveOptionsKey = baseActiveOptionsKey
        onSelectCommonActiveOptionsChanged: if (root.selectCommonActiveOptions !== selectCommonActiveOptions) root.selectCommonActiveOptions = selectCommonActiveOptions
        onSelectCommonActiveOptionsKeyChanged: if (root.selectCommonActiveOptionsKey !== selectCommonActiveOptionsKey) root.selectCommonActiveOptionsKey = selectCommonActiveOptionsKey
        onSelectCommonActiveOptionsReadyChanged: if (root.selectCommonActiveOptionsReady !== selectCommonActiveOptionsReady) root.selectCommonActiveOptionsReady = selectCommonActiveOptionsReady
        onSelectRuntimeActiveOptionsChanged: if (root.selectRuntimeActiveOptions !== selectRuntimeActiveOptions) root.selectRuntimeActiveOptions = selectRuntimeActiveOptions
        onSelectRuntimeActiveOptionsKeyChanged: if (root.selectRuntimeActiveOptionsKey !== selectRuntimeActiveOptionsKey) root.selectRuntimeActiveOptionsKey = selectRuntimeActiveOptionsKey
        onGameplayRuntimeActiveOptionsChanged: if (root.gameplayRuntimeActiveOptions !== gameplayRuntimeActiveOptions) root.gameplayRuntimeActiveOptions = gameplayRuntimeActiveOptions
        onGameplayRuntimeActiveOptionsKeyChanged: if (root.gameplayRuntimeActiveOptionsKey !== gameplayRuntimeActiveOptionsKey) root.gameplayRuntimeActiveOptionsKey = gameplayRuntimeActiveOptionsKey
        onRankingStatsApplyRequested: root.applyRankingStatsToSelectContext()
        onSelectSideEffectsUpdateRequested: root.updateSelectSideEffects()
    }

    Lr2SelectKeyNavigationFilter {
        target: root
        selectScrollReady: screenState.selectScrollReady
        navigationController: selectContext.navigationController
    }

    Lr2PlayContext {
        id: playContext
        enabled: root.isGameplayScreen()
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
            root.refreshBaseActiveOptions();
            root.refreshSelectRuntimeActiveOptions();
            root.refreshGameplayRuntimeActiveOptions();
            root.updateResultOldScores();
        }
    }

    readonly property var skinModelRef: skinModel

    onCsvPathChanged: root.openSelectIfNeeded()
    onSkinSettingsChanged: {
        root.refreshLr2SkinSettingItems();
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }
    onScreenKeyChanged: {
        root.openSelectIfNeeded();
        root.activateGameplayIfNeeded();
        root.refreshLr2SkinSettingItems();
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }

    Component.onCompleted: {
        root.componentReady = true;
        selectSideEffects.ready = true;
        root.commitLr2RankingRequest();
        root.restartSkinClock();
        root.openSelectIfNeeded();
        root.activateGameplayIfNeeded();
        selectSideEffects.update();
        root.updateGameplaySavedScores();
        root.refreshLr2SkinSettingItems();
        root.refreshBaseActiveOptions();
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
        if (root.isGameplayScreen()) {
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
        return root.enabled && root.isResultScreen() && root.acceptsInput;
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
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoBack();
    }
    Keys.onRightPressed: (event) => {
        if (root.handleLr2GameplayArrow(Qt.Key_Right)) {
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
        if (root.closeResultScreen()) {
            return;
        }
        root.pressSelectHeldButtonTimer(key);
        if (root.handleSelectPanelKey(key)) {
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
        if (root.selectNavigationReady() && (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26)) {
            root.selectGoBack();
        }
    }
    Input.onButtonReleased: (key) => {
        root.releaseLr2GameplayOptionKey(key);
        root.releaseSelectHeldButtonTimer(key);
        if (root.isLr2RankingKey(key)) {
            root.closeLr2Ranking();
        }
    }

    Lr2SelectSideEffects {
        id: selectSideEffects
        host: root
        selectContext: selectContext
        skinModel: skinModel
        active: root.selectAudioActive
        selectRevision: root.selectRevision
    }

    AudioPlayer {
        id: openFolderSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "f-open" : ""
    }

    AudioPlayer {
        id: closeFolderSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "f-close" : ""
    }

    AudioPlayer {
        id: gameplayReadySound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "playready" : ""
        onPlayingChanged: {
            if (!playing
                    && root.gameplayStartArmed
                    && root.enabled
                    && root.isGameplayScreen()
                    && root.chart
                    && root.chartStatusIs(root.chart.status, ChartRunner.Ready)) {
                skinTiming.restartGameplayStartTimer();
            }
        }
    }

    AudioPlayer {
        id: gameplayStopSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "playstop" : ""
    }

    AudioPlayer {
        id: optionOpenSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "o-open" : ""
    }

    AudioPlayer {
        id: optionCloseSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "o-close" : ""
    }

    AudioPlayer {
        id: optionChangeSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "o-change" : ""
    }

    Lr2SkinSliderState {
        id: skinSliderState
        screenRoot: root
        selectContext: selectContext
        optionChangeSound: optionChangeSound
    }

    Lr2SelectSearchState {
        id: selectSearchState
        screenRoot: root
        selectContext: selectContext
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
    }

    Lr2SelectPanelController {
        id: selectPanelController
        screenRoot: root
        selectContext: selectContext
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
    }
}
