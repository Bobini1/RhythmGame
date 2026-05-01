pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

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
    property var selectContextRef: null
    property bool componentReady: false
    readonly property var emptyActiveOptions: []
    readonly property var zeroTimers: ({ "0": 0 })
    readonly property var usedOptionLookup: {
        let result = {};
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        for (let option of used) {
            result[Math.abs(option)] = true;
        }
        return result;
    }
    property int gameplayRevision: 0
    property bool gameplayRevisionQueued: false
    property int gameplayTimerRevision: 0
    property bool gameplayTimerRevisionQueued: false
    property var gameplayTimerValues: ({ "0": 0 })
    property var gameplayRuntimeActiveOptions: []
    property bool gameplayRuntimeActiveOptionsRefreshQueued: false
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
    readonly property bool stackScreenActive: screenState.stackActive
    readonly property bool screenUpdatesActive: screenState.updatesActive
    readonly property int lr2CurrentFps: skinTiming.currentFps
    readonly property var lr2InitialClockNow: wallClockState.initialNow
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

    onEnabledChanged: {
        if (root.effectiveScreenKey === "decide" && enabled) {
            Qt.callLater(() => sceneStack.pop());
        }
        if (enabled) {
            root.openSelectIfNeeded();
            Qt.callLater(root.activateGameplayIfNeeded);
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
            Qt.callLater(root.activateGameplayIfNeeded);
            root.refreshSelectRuntimeActiveOptions();
            Qt.callLater(root.refreshGameplayRuntimeActiveOptions);
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
                function onPressedChanged() {
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
                function onHoldingLongNoteChanged() {
                    root.syncGameplayLongNoteTimerFromColumnState(timerId, columnState);
                }
            }
        }
    }

    function openSelectIfNeeded() {
        if (root.effectiveScreenKey === "select" && !root.screenUpdatesActive) {
            return;
        }
        if (root.effectiveScreenKey === "select" && selectContext.historyStack.length === 0) {
            root.selectScratchSoundReady = false;
            selectContext.openRoot();
            Qt.callLater(() => root.selectScratchSoundReady = true);
        } else if (root.effectiveScreenKey === "select" && !root.selectScratchSoundReady) {
            Qt.callLater(() => root.selectScratchSoundReady = true);
        }
        root.updateSelectSideEffects();
        if (root.effectiveScreenKey === "select"
                && root.acceptsInput
                && root.anyStartHeld
                && !root.startHoldSuppressed) {
            root.holdSelectPanel(1);
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

    StackView.onActivated: {
        Qt.callLater(root.activateGameplayIfNeeded);
    }
    
    readonly property string effectiveScreenKey: screenState.effectiveKey
    function playerIsAutoPlayer(player) {
        return !!player && player instanceof AutoPlayer;
    }

    function gameplayAutoplayActive() {
        return root.isGameplayScreen()
            && !!root.chart
            && root.playerIsAutoPlayer(root.chart.player1)
            && (!root.chart.player2 || root.playerIsAutoPlayer(root.chart.player2));
    }

    function gameplayReplayActive() {
        if (!root.isGameplayScreen() || !root.chart) {
            return false;
        }
        let p1 = root.gameplayPlayer(1);
        let p2 = root.gameplayPlayer(2);
        return !!((p1 && p1.replayedScore) || (p2 && p2.replayedScore));
    }

    function optionLookupFor(options) {
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

    function finalizeOptionList(options) {
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
    readonly property int selectTargetScratchInitialRepeatMillis: selectPanelController.selectTargetScratchInitialRepeatMillis
    readonly property int selectTargetScratchRepeatMillis: selectPanelController.selectTargetScratchRepeatMillis
    property alias selectTargetScratchDirection: selectPanelController.selectTargetScratchDirection
    property alias selectTargetScratchNextMs: selectPanelController.selectTargetScratchNextMs
    readonly property bool anyStartHeld: selectPanelController.anyStartHeld
    property alias startHoldSuppressed: selectPanelController.startHoldSuppressed

    Lr2OptionState {
        id: optionState
        host: root
    }

    readonly property var lr2GaugeLabels: optionState.lr2GaugeLabels
    readonly property var lr2GaugeValues: optionState.lr2GaugeValues
    readonly property var lr2RandomLabels: optionState.lr2RandomLabels
    readonly property var lr2RandomValues: optionState.lr2RandomValues
    readonly property var lr2RandomSupportedIndexes: optionState.lr2RandomSupportedIndexes
    readonly property var lr2HiSpeedFixLabels: optionState.lr2HiSpeedFixLabels
    readonly property var lr2HiSpeedFixValues: optionState.lr2HiSpeedFixValues
    readonly property var lr2DpOptionLabels: optionState.lr2DpOptionLabels
    readonly property var lr2DpOptionSupportedIndexes: optionState.lr2DpOptionSupportedIndexes
    readonly property var lr2GaugeAutoShiftLabels: optionState.lr2GaugeAutoShiftLabels
    readonly property var lr2GaugeAutoShiftSupportedIndexes: optionState.lr2GaugeAutoShiftSupportedIndexes
    readonly property var lr2BattleLabels: optionState.lr2BattleLabels
    readonly property var lr2TargetLabels: optionState.lr2TargetLabels
    readonly property var lr2TargetValues: optionState.lr2TargetValues
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
        skinHeight: root.skinH
        skinTime: root.selectLiveSkinTime
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
        tracking: root.selectHoverTracking
        skinScale: root.skinScale
        zeroTimers: root.zeroTimers
        emptyActiveOptions: root.emptyActiveOptions
    }
    property alias selectHoverElements: selectHoverState.elements
    readonly property int selectHoverElementCount: selectHoverState.elementCount
    readonly property var selectHoverCandidateKeys: selectHoverState.candidateKeys
    property alias selectHoverVisibleByIndex: selectHoverState.visibleByIndex
    readonly property string selectHoverVisibleSignature: selectHoverState.visibleSignature
    property alias selectHoverRevision: selectHoverState.revision
    property alias selectHoverRefreshQueued: selectHoverState.refreshQueued
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

    function selectHoverPointInSkinCoordinates() {
        return selectHoverState.pointInSkinCoordinates();
    }

    function updateSelectHoverPoint(x, y) {
        return selectHoverState.updatePoint(x, y);
    }

    function clearSelectHoverPoint() {
        selectHoverState.clearPoint();
    }

    function registerSelectHoverElement(elementIndex, src, dsts, enabled) {
        selectHoverState.registerElement(elementIndex, src, dsts, enabled);
    }

    function unregisterSelectHoverElement(elementIndex) {
        selectHoverState.unregisterElement(elementIndex);
    }

    function scheduleSelectHoverRefresh() {
        selectHoverState.scheduleRefresh();
    }

    function runSelectHoverRefresh() {
        selectHoverState.runRefresh();
    }

    function clearSelectHoverCache() {
        selectHoverState.clearCache();
    }

    function refreshSelectHoverCache() {
        selectHoverState.refreshCache();
    }

    function copyObject(object) {
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

    function localizedName(value) { return skinSettingsState.localizedName(value); }
    function lr2SkinTypeScreenKey(type) { return skinSettingsState.skinTypeScreenKey(type); }
    function profileRoot() { return skinSettingsState.profileRoot(); }
    function lr2ConfiguredThemeName(screen) { return skinSettingsState.configuredThemeName(screen); }
    function lr2ThemeFamilyForScreen(screen) { return skinSettingsState.themeFamilyForScreen(screen); }
    function lr2ScreenObject(screen) { return skinSettingsState.screenObject(screen); }
    function lr2AvailableThemeNamesForScreen(screen) { return skinSettingsState.availableThemeNamesForScreen(screen); }
    function lr2SettingDestinationForScreen(screen) { return skinSettingsState.settingDestinationForScreen(screen); }
    function currentLr2SkinPreviewScreen() { return skinSettingsState.currentPreviewScreen(); }
    function defaultLr2SkinPreviewScreen() { return skinSettingsState.defaultPreviewScreen(); }
    function setLr2SkinPreviewScreen(screen) { return skinSettingsState.setPreviewScreen(screen); }
    function lr2SkinPreviewTitle() { return skinSettingsState.previewTitle(); }
    function lr2SkinPreviewMaker() { return skinSettingsState.previewMaker(); }
    function normalizeLr2SkinSetting(item, family) { return skinSettingsState.normalizeSetting(item, family); }
    function buildLr2SkinSettingItems() { return skinSettingsState.buildItems(); }
    function refreshLr2SkinSettingItems() { skinSettingsState.refreshItems(); }
    function lr2SkinCustomMaxOffset() { return skinSettingsState.maxOffset(); }
    function lr2SkinCustomPosition() { return skinSettingsState.position(); }
    function setLr2SkinCustomPosition(position) { return skinSettingsState.setPosition(position); }
    function lr2SkinSettingAtVisibleRow(row) { return skinSettingsState.settingAtVisibleRow(row); }
    function lr2SkinSettingCurrentValue(item) { return skinSettingsState.currentValue(item); }
    function lr2SkinSettingName(row) { return skinSettingsState.settingName(row); }
    function lr2SkinSettingValueText(row) { return skinSettingsState.settingValueText(row); }
    function changeLr2SkinSetting(row, delta) { return skinSettingsState.changeSetting(row, delta); }
    function changeLr2Soundset(delta) { return skinSettingsState.changeSoundset(delta); }
    function changeLr2SelectedTheme(delta) { return skinSettingsState.changeSelectedTheme(delta); }
    function queueSkinClockRestartAfterLoad() { skinSettingsState.queueSkinClockRestartAfterLoad(); }

    function setArrayValue(array, index, value) { return optionState.setArrayValue(array, index, value); }
    function sliderInitialValue(type) { return optionState.sliderInitialValue(type); }
    function sliderRawValue(type) { return optionState.sliderRawValue(type); }
    function setSliderRawValue(type, value) { optionState.setSliderRawValue(type, value); }
    function lr2SliderNumber(num) { return optionState.lr2SliderNumber(num); }

    function openReadmeText(text) {
        readmeState.openText(text);
    }

    function openReadmePath(path) {
        return readmeState.openPath(path);
    }

    function toggleSelectedReadme() {
        if (root.lr2ReadmeMode > 0) {
            root.closeReadme();
            return true;
        }
        if (root.effectiveScreenKey !== "select") {
            return false;
        }
        return root.openReadmePath(
            selectContext.attachedTextFile(selectContext.selectedChartData()));
    }

    function openHelpFile(index) {
        let helpFiles = skinModel.helpFiles || [];
        if (index < 0 || index >= helpFiles.length) {
            return false;
        }
        return root.openReadmePath(helpFiles[index]);
    }

    function toggleMainHelp() {
        if (root.lr2ReadmeMode > 0) {
            root.closeReadme();
            return true;
        }
        if (root.effectiveScreenKey !== "select") {
            return false;
        }
        return root.openHelpFile(0);
    }

    function closeReadme() {
        readmeState.close();
    }

    function readmeLinesForSource(src) {
        return readmeState.linesForSource(src);
    }

    function readmeContentHeight() {
        return readmeState.contentHeight();
    }

    function clampReadmeOffsets() {
        readmeState.clampOffsets();
    }

    function scrollReadmeBy(dx, dy) {
        return readmeState.scrollBy(dx, dy);
    }

    function hideReadmeImmediately() {
        readmeState.hideImmediately();
    }

    function mainGeneralVars() { return optionState.mainGeneralVars(); }
    function profileForSide(side) { return optionState.profileForSide(side); }
    function generalVarsForSide(side) { return optionState.generalVarsForSide(side); }
    function indexOfValue(values, value) { return optionState.indexOfValue(values, value); }
    function cycleArrayIndex(index, count, delta) { return optionState.cycleArrayIndex(index, count, delta); }
    function wrappedListValue(values, index) { return optionState.wrappedListValue(values, index); }
    function arrayContains(values, value) { return optionState.arrayContains(values, value); }
    function cycleSupportedIndex(current, delta, supportedIndexes, count) { return optionState.cycleSupportedIndex(current, delta, supportedIndexes, count); }

    readonly property int lr2GaugeIndexP1: optionState.lr2GaugeIndexP1
    readonly property int lr2GaugeIndexP2: optionState.lr2GaugeIndexP2
    function setGaugeIndex(side, index) { optionState.setGaugeIndex(side, index); }

    readonly property int lr2RandomIndexP1: optionState.lr2RandomIndexP1
    readonly property int lr2RandomIndexP2: optionState.lr2RandomIndexP2
    function setRandomIndex(side, index) { optionState.setRandomIndex(side, index); }

    readonly property int lr2HidSudIndexP1: optionState.lr2HidSudIndexP1
    readonly property int lr2HidSudIndexP2: optionState.lr2HidSudIndexP2
    function setHidSudIndex(side, index) { optionState.setHidSudIndex(side, index); }

    readonly property int lr2HiSpeedFixIndex: optionState.lr2HiSpeedFixIndex
    function setHiSpeedFixIndex(index) { optionState.setHiSpeedFixIndex(index); }

    readonly property int lr2BattleIndex: optionState.lr2BattleIndex
    function ensureBattleProfiles() { return optionState.ensureBattleProfiles(); }
    function setBattleIndex(index) { optionState.setBattleIndex(index); }

    readonly property int lr2DpOptionIndex: optionState.lr2DpOptionIndex
    function setDpOptionIndex(index) { optionState.setDpOptionIndex(index); }

    readonly property int lr2FlipIndex: optionState.lr2FlipIndex
    function setFlipIndex(index) { optionState.setFlipIndex(index); }

    readonly property int lr2LaneCoverIndex: optionState.lr2LaneCoverIndex
    function setLaneCoverIndex(index) { optionState.setLaneCoverIndex(index); }

    readonly property int lr2BgaIndex: optionState.lr2BgaIndex
    readonly property int lr2BeatorajaBgaIndex: optionState.lr2BeatorajaBgaIndex
    function setBgaIndex(index) { optionState.setBgaIndex(index); }

    readonly property int lr2BgaSizeIndex: optionState.lr2BgaSizeIndex
    function setBgaSizeIndex(index) { optionState.setBgaSizeIndex(index); }

    readonly property int lr2GaugeAutoShiftIndex: optionState.lr2GaugeAutoShiftIndex
    function setGaugeAutoShiftIndex(index) { optionState.setGaugeAutoShiftIndex(index); }

    readonly property int lr2ScoreGraphIndex: optionState.lr2ScoreGraphIndex
    function setScoreGraphIndex(index) { optionState.setScoreGraphIndex(index); }

    readonly property int lr2GhostIndex: optionState.lr2GhostIndex
    function setGhostIndex(index) { optionState.setGhostIndex(index); }

    function lr2BgaEnabled() { return optionState.lr2BgaEnabled(); }

    readonly property int lr2ScoreTargetIndex: optionState.lr2ScoreTargetIndex
    function setScoreTargetIndex(index) { optionState.setScoreTargetIndex(index); }

    readonly property int lr2BeatorajaTargetIndex: optionState.lr2BeatorajaTargetIndex
    function setBeatorajaTargetIndex(index) { optionState.setBeatorajaTargetIndex(index); }

    readonly property int lr2TargetPercent: optionState.lr2TargetPercent
    function setTargetPercent(percent) { optionState.setTargetPercent(percent); }

    readonly property int lr2HiSpeedP1: optionState.lr2HiSpeedP1
    readonly property int lr2HiSpeedP2: optionState.lr2HiSpeedP2
    function setHiSpeedNumber(side, value) { optionState.setHiSpeedNumber(side, value); }
    function nextLr2HiSpeedNumber(current, steps) { return optionState.nextLr2HiSpeedNumber(current, steps); }
    function adjustHiSpeedNumber(side, steps) { optionState.adjustHiSpeedNumber(side, steps); }
    function adjustLaneCoverRatio(side, steps) { optionState.adjustLaneCoverRatio(side, steps); }
    function adjustOffset(delta) { optionState.adjustOffset(delta); }

    function isLoggedIn() { return optionState.isLoggedIn(); }
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

    function commitLr2RankingRequest(chart) { return lr2Ranking.commitRequest(chart); }
    function rankingProviderEnum() { return lr2Ranking.providerEnum(); }
    function lr2RankingMatchesCurrentChart() { return lr2Ranking.matchesCurrentChart(); }
    function lr2LocalRankingEntry() { return lr2Ranking.localEntry(); }
    function lr2RankingEntries() { return lr2Ranking.entries(); }
    function lr2RankingClearCounts(entries) { return lr2Ranking.clearCounts(entries); }
    function lr2RankingPlayerCount(entries) { return lr2Ranking.playerCount(entries); }
    function lr2RankingTotalPlayCount(entries) { return lr2Ranking.totalPlayCount(entries); }
    function lr2RankingProfileUserId() { return lr2Ranking.profileUserId(); }
    function lr2RankingPlayerRank(entries) { return lr2Ranking.playerRank(entries); }
    function lr2RankingSnapshot() { return lr2Ranking.snapshot(); }
    function lr2RankingEntryAt(index) { return lr2Ranking.entryAt(index); }
    function lr2RankingEntryName(index) { return lr2Ranking.entryName(index); }
    function lr2RankingEntryClearValue(index) { return lr2Ranking.entryClearValue(index); }
    function lr2RankingEntryExScore(index) { return lr2Ranking.entryExScore(index); }
    function lr2RankingClearCount() { return lr2Ranking.clearCount.apply(lr2Ranking, arguments); }
    function lr2RankingClearPercentValue() { return lr2Ranking.clearPercentValue.apply(lr2Ranking, arguments); }
    function applyRankingStatsToSelectContext() { lr2Ranking.applyStatsToSelectContext(); }
    function handleRankingModelChanged(tryOpenRanking, tryOpenInternetRanking) { lr2Ranking.handleModelChanged(tryOpenRanking, tryOpenInternetRanking); }
    function lr2RankingStatusOption() { return lr2Ranking.statusOption(); }
    function startLr2RankingTransition(action) { return lr2Ranking.startTransition(action); }
    function clearLr2RankingTransition() { lr2Ranking.clearTransition(); }
    function enterLr2RankingPostSwapTimer() { lr2Ranking.enterPostSwapTimer(); }
    function performLr2RankingOpen() { return lr2Ranking.performOpen(); }
    function performLr2RankingClose() { return lr2Ranking.performClose(); }
    function advanceLr2RankingTransition() { lr2Ranking.advanceTransition(); }
    function requestLr2RankingFetch(chart) { return lr2Ranking.requestFetch(chart); }
    function lr2TachiKeymode(chart) { return lr2Ranking.tachiKeymode(chart); }
    function lr2InternetRankingUrl(chart) { return lr2Ranking.internetRankingUrl(chart); }
    function finishOpenLr2InternetRanking() { return lr2Ranking.finishOpenInternetRanking(); }
    function openLr2InternetRanking() { return lr2Ranking.openInternetRanking(); }
    function finishOpenLr2Ranking() { return lr2Ranking.finishOpenRanking(); }
    function openLr2Ranking() { return lr2Ranking.openRanking(); }
    function closeLr2Ranking() { return lr2Ranking.closeRanking(); }
    function isLr2RankingKey(key) { return key === BmsKey.Col14 || key === BmsKey.Col24; }

    function clearStatusOption() { return optionState.clearStatusOption(); }
    function clearStatusIsBest() { return optionState.clearStatusIsBest(); }

    Rectangle {
        anchors.fill: parent
        color: "black"
        z: -1000000
    }

    function inferScreenKey(path) {
        return screenState.inferKey(path);
    }

    function isSelectScrollSlider(src) {
        return skinSliderState.isSelectScrollSlider(src);
    }

    function isLr2GenericSlider(src) {
        return skinSliderState.isLr2GenericSlider(src);
    }

    function isGameplayProgressSlider(src) {
        return skinSliderState.isGameplayProgressSlider(src);
    }

    function isGameplayLaneCoverSlider(src) {
        return skinSliderState.isGameplayLaneCoverSlider(src);
    }

    function sliderTrackState(src, dsts, skinTime) {
        return skinSliderState.trackState(src, dsts, skinTime);
    }

    function sliderPositionFromPointer(src, track, pointerX, pointerY) {
        return skinSliderState.positionFromPointer(src, track, pointerX, pointerY);
    }

    function panelMatches(panel) {
        if (panel > 0) {
            return root.selectPanel === panel;
        }
        if (panel < 0) {
            return root.selectPanel === 0;
        }
        return true;
    }

    property alias selectSearchInputItem: selectSearchState.inputItem

    function isSelectSearchText(src) { return selectSearchState.isText(src); }
    function selectSearchTextState(src, dsts) { return selectSearchState.textState(src, dsts); }
    function textPrefix(text, position) { return selectSearchState.textPrefix(text, position); }
    function selectSearchHasFocus() { return selectSearchState.hasFocus(); }

    function selectInputReady() {
        return screenState.selectInputReady;
    }

    function selectPointerInputReady() {
        return screenState.selectPointerInputReady;
    }

    function selectScrollReady() {
        return screenState.selectScrollReady;
    }

    function selectPointerScrollReady() {
        return screenState.selectPointerScrollReady;
    }

    function selectNavigationReady() {
        return screenState.selectNavigationReady;
    }

    function focusSelectSearch() { selectSearchState.focusInput(); }
    function clearSelectSearchFocus() { selectSearchState.clearFocus(); }
    function resetSelectSearch() { selectSearchState.reset(); }

    function fallbackSortId(dsts) {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return 0;
        }
        return dsts[0].sortId || 0;
    }

    function timelineSortId(dsts, skinTime, activeOptions, timers) {
        let state = Lr2Timeline.getCurrentState(
            dsts,
            skinTime,
            timers || root.timers,
            activeOptions || root.runtimeActiveOptions);
        if (state && state.sortId !== undefined) {
            return state.sortId;
        }
        return root.fallbackSortId(dsts);
    }

    function noteElementSortId(skinTime, activeOptions, timers) {
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

    function staticNoteElementSortId() {
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

    function elementZ(type, index, src, dsts) {
        // OpenLR2 stamps every DST keyframe with the skin command counter and
        // globally sorts DrawingBuf by that value before drawing.
        if (type === 8) {
            return root.staticNoteElementSortId() + index * 0.000001;
        }
        return root.fallbackSortId(dsts) + index * 0.000001;
    }

    function dstsUseActiveOptions(dsts) {
        return Lr2Timeline.dstsUseActiveOptions(dsts);
    }

    function elementUsesTimers(src, dsts) {
        return Lr2Timeline.dstsUseDynamicTimer(dsts) || Lr2Timeline.srcUsesDynamicTimer(src);
    }

    function elementUsesSkinTime(src, dsts) {
        return !Lr2Timeline.canUseStaticState(dsts)
            || (!!src && (Lr2Timeline.srcCyclesContinuously(src)
                || (src.resultChartType || 0) > 0));
    }

    function elementUsesLiveDstClock(dsts) {
        return root.effectiveScreenKey === "select"
            && Lr2Timeline.dstsLoopContinuously(dsts);
    }

    function elementUsesLiveSourceClock(src) {
        return root.effectiveScreenKey === "select"
            && Lr2Timeline.srcCyclesContinuously(src);
    }

    function elementUsesLiveSelectClock(src, dsts) {
        return root.elementUsesLiveDstClock(dsts)
            || root.elementUsesLiveSourceClock(src);
    }

    function skinTimeForElement(src, dsts) {
        return root.elementUsesLiveSelectClock(src, dsts)
            ? root.selectSourceSkinTime
            : root.renderSkinTime;
    }

    function dstCollectionUsesActiveOptions(collection) {
        if (!collection) {
            return false;
        }
        for (let i = 0; i < collection.length; ++i) {
            if (Lr2Timeline.dstsUseActiveOptions(collection[i])) {
                return true;
            }
        }
        return false;
    }

    function dstCollectionUsesTimers(collection) {
        if (!collection) {
            return false;
        }
        for (let i = 0; i < collection.length; ++i) {
            if (Lr2Timeline.dstsUseDynamicTimer(collection[i])) {
                return true;
            }
        }
        return false;
    }

    function sourceCollectionUsesTimers(collection) {
        if (!collection) {
            return false;
        }
        for (let i = 0; i < collection.length; ++i) {
            if (Lr2Timeline.srcUsesDynamicTimer(collection[i])) {
                return true;
            }
        }
        return false;
    }

    function noteFieldUsesActiveOptions() {
        return root.dstCollectionUsesActiveOptions(skinModel.noteDsts)
            || root.dstCollectionUsesActiveOptions(skinModel.lineDsts);
    }

    function noteFieldUsesTimers() {
        return root.dstCollectionUsesTimers(skinModel.noteDsts)
            || root.dstCollectionUsesTimers(skinModel.lineDsts)
            || root.sourceCollectionUsesTimers(skinModel.noteSources)
            || root.sourceCollectionUsesTimers(skinModel.autoNoteSources)
            || root.sourceCollectionUsesTimers(skinModel.mineSources)
            || root.sourceCollectionUsesTimers(skinModel.autoMineSources)
            || root.sourceCollectionUsesTimers(skinModel.lnStartSources)
            || root.sourceCollectionUsesTimers(skinModel.autoLnStartSources)
            || root.sourceCollectionUsesTimers(skinModel.lnEndSources)
            || root.sourceCollectionUsesTimers(skinModel.autoLnEndSources)
            || root.sourceCollectionUsesTimers(skinModel.lnBodySources)
            || root.sourceCollectionUsesTimers(skinModel.autoLnBodySources)
            || root.sourceCollectionUsesTimers(skinModel.lnBodyActiveSources)
            || root.sourceCollectionUsesTimers(skinModel.autoLnBodyActiveSources)
            || root.sourceCollectionUsesTimers(skinModel.lineSources);
    }

    function addOption(options, option) {
        if (option === undefined || option === null) {
            return;
        }
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        if (used.length > 0 && !root.usedOptionLookup[Math.abs(option)]) {
            return;
        }
        let lookup = root.optionLookupFor(options);
        if (lookup[option] === true) {
            return;
        }
        options.push(option);
        lookup[option] = true;
    }

    function skinUsesOption(option) {
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        return used.length === 0 || !!root.usedOptionLookup[Math.abs(option)];
    }

    function skinUsesAnyOption(options) {
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        if (used.length === 0) {
            return true;
        }
        for (let i = 0; i < options.length; ++i) {
            if (root.usedOptionLookup[Math.abs(options[i])]) {
                return true;
            }
        }
        return false;
    }

    function skinUsesOptionRange(first, last) {
        let used = skinModel && skinModel.usedOptions ? skinModel.usedOptions : [];
        if (used.length === 0) {
            return true;
        }
        for (let option = first; option <= last; ++option) {
            if (root.usedOptionLookup[option]) {
                return true;
            }
        }
        return false;
    }

    function configuredGaugeName(side) {
        let vars = root.generalVarsForSide(side);
        return String(vars ? vars.gaugeType : "").toUpperCase();
    }

    function gameplayActiveGauge(score) {
        if (!score || !score.gauges || score.gauges.length === 0) {
            return null;
        }
        let gauges = score.gauges;
        for (let gauge of gauges) {
            if (gauge && (gauge.gauge || 0) > (gauge.threshold || 0)) {
                return gauge;
            }
        }
        return gauges[gauges.length - 1] || null;
    }

    function gameplayActiveGaugeName(score) {
        let gauge = root.gameplayActiveGauge(score);
        return String(gauge && gauge.name ? gauge.name : "").toUpperCase();
    }

    function activeGaugeNameForSide(side) {
        if (root.isGameplayScreen()) {
            let activeName = root.gameplayActiveGaugeName(root.gameplayScore(side));
            if (activeName.length > 0) {
                return activeName;
            }
        }
        return root.configuredGaugeName(side);
    }

    function gaugeNameIsSurvival(name) {
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

    function gaugeNameUsesBeatorajaExOption(name) {
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

    function gaugeNameUsesExGaugeSprites(name) {
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

    function addGaugeExOption(options, side) {
        let gauge = root.activeGaugeNameForSide(side);
        if (root.gaugeNameUsesBeatorajaExOption(gauge)) {
            root.addOption(options, side === 2 ? 1047 : 1046);
        }
    }

    function gaugeColorOption(side) {
        let red = root.gaugeNameIsSurvival(root.activeGaugeNameForSide(side));
        return side === 2 ? (red ? 45 : 44) : (red ? 43 : 42);
    }

    function gameplayGaugeTrophyOption(side) {
        let gauge = root.activeGaugeNameForSide(side);
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

    function battleModeActive() {
        return !!(Rg.profileList && Rg.profileList.battleActive);
    }

    function spToDpActive() {
        let vars = root.mainGeneralVars();
        return !!vars && vars.dpOptions === DpOptions.Battle;
    }

    function laneCoverNumber(side) {
        let vars = root.generalVarsForSide(side);
        return vars && vars.laneCoverOn
            ? Math.round((vars.laneCoverRatio || 0) * 1000)
            : 0;
    }

    function liftNumber(side) {
        let vars = root.generalVarsForSide(side);
        return vars && vars.liftOn ? Math.round((vars.liftRatio || 0) * 1000) : 0;
    }

    function hiddenNumber(side) {
        let vars = root.generalVarsForSide(side);
        return vars && vars.hiddenOn ? Math.round((vars.hiddenRatio || 0) * 1000) : 0;
    }

    function hiSpeedInteger(side) {
        let value = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        return Math.floor(value / 100);
    }

    function hiSpeedAfterDot(side) {
        let value = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        return Math.floor(value) % 100;
    }

    function durationNumber(side, green) {
        let vars = root.generalVarsForSide(side);
        let duration = vars && vars.noteScreenTimeMillis > 0 ? vars.noteScreenTimeMillis : 1000;
        return Math.round(duration * (green ? 1.0 : 0.6));
    }

    function durationNumberForBpm(side, bpm, green, cover) {
        let safeBpm = Math.max(1, bpm || 0);
        let hiSpeed = Math.max(0.01, (side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1) / 100);
        let vars = root.generalVarsForSide(side);
        let visible = cover && vars && vars.laneCoverOn ? 1 - (vars.laneCoverRatio || 0) : 1;
        return Math.round((240000 / safeBpm / hiSpeed) * visible * (green ? 1.0 : 0.6));
    }

    function bpmDurationNumber(num, chartData) {
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

    function chartLengthSeconds(chartData) {
        return chartData ? Math.floor(Math.max(0, chartData.length || 0) / 1000000000) : -1;
    }

    function chartPlayableNoteCount(chartData) {
        if (!chartData) {
            return 0;
        }
        return (chartData.normalNoteCount || 0)
            + (chartData.scratchCount || 0)
            + (chartData.lnCount || 0)
            + (chartData.bssCount || 0);
    }

    function chartDensityNumber(chartData, propertyName, afterDot) {
        if (!chartData) {
            return -1;
        }
        let value = Math.max(0, chartData[propertyName] || 0);
        return afterDot ? Math.floor(value * 100) % 100 : Math.floor(value);
    }

    function chartHasBpmStop(chartData) {
        let changes = chartData && chartData.bpmChanges ? chartData.bpmChanges : [];
        for (let i = 0; i < changes.length; ++i) {
            if ((changes[i].bpm || 0) === 0) {
                return true;
            }
        }
        return false;
    }

    function clearTypeValue(clearType) {
        switch (String(clearType || "NOPLAY")) {
        case "FAILED":
            return 1;
        case "AEASY":
            return 2;
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

    function dateTimeNumber(num) {
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

    function updateLr2DateTimeNumbers() {
        wallClockState.update();
    }

    function isGameplayScreen() {
        return screenState.isGameplayScreen();
    }

    function chartStatusValue(status) {
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

    function chartStatusIs(status, expected) {
        return root.chartStatusValue(status) === expected;
    }

    function chartStatusAtLeast(status, minimum) {
        return root.chartStatusValue(status) >= minimum;
    }

    function gameplayStatusAtLeast(minimum) {
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

    function isCourseGameplay() {
        return !!(root.chart && root.chart.chartDatas && root.chart.currentChartIndex !== undefined);
    }

    function gameplayProfiles() {
        return [
            root.chart && root.chart.player1 ? root.chart.player1.profile : null,
            root.chart && root.chart.player2 ? root.chart.player2.profile : null
        ];
    }

    function stopGameplayLifecycle() {
        root.gameplayStartArmed = false;
        skinTiming.stopGameplayStartTimer();
        gameplayReadySound.stop();
        gameplayStopSound.stop();
    }

    function startGameplayWhenReady() {
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

    function activateGameplayIfNeeded() {
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

    function openGameplayStageResult() {
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

    function handleGameplayStatusChanged() {
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

    function gameplayHitCountsAsPlayed(hit) {
        if (!hit || !hit.points) {
            return false;
        }
        let judgement = hit.points.judgement;
        return judgement !== Judgement.Poor
            && judgement !== Judgement.EmptyPoor
            && judgement !== Judgement.MineHit
            && judgement !== Judgement.MineAvoided;
    }

    function handleGameplayEscape() {
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

    function gameplayChartData() {
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

    function isResultScreen() { return screenState.isResultScreen(); }
    function resultScore(side) { return resultState.resultScore(side); }
    function resultData(side) { return resultState.resultData(side); }
    function resultProfile(side) { return resultState.resultProfile(side); }
    function resultChartData() { return resultState.resultChartData(); }
    function displayChartData() {
        if (root.effectiveScreenKey === "select") {
            return selectContext.cachedSelectedChartData;
        }
        return resultState.displayChartData();
    }
    function resultClearOption() { return resultState.resultClearOption(); }
    function resultTotalNotes(result) { return resultState.resultTotalNotes(result); }
    function resultJudgementCount(result, judgement) { return resultState.resultJudgementCount(result, judgement); }
    function resultPoorCount(result) { return resultState.resultPoorCount(result); }
    function resultBadPoor(result) { return resultState.resultBadPoor(result); }
    function emptyJudgeTimingCounts() { return resultState.emptyJudgeTimingCounts(); }
    function judgementTimingBucket(judgement) { return resultState.judgementTimingBucket(judgement); }
    function hitDeviationNanos(hit) { return resultState.hitDeviationNanos(hit); }
    function hitDeviationMillis(hit) { return resultState.hitDeviationMillis(hit); }
    function judgementUpdatesJudgeTimingValue(judgement) { return resultState.judgementUpdatesJudgeTimingValue(judgement); }
    function cloneJudgeTimingCounts(counts) { return resultState.cloneJudgeTimingCounts(counts); }
    function emptyJudgeLaneValues() { return resultState.emptyJudgeLaneValues(); }
    function nowJudgeValue(judgement) { return resultState.nowJudgeValue(judgement); }
    function laneJudgeValue(judgement, timing) { return resultState.laneJudgeValue(judgement, timing); }
    function setGameplayJudgeLaneValue(scoreSide, hit, value) { resultState.setGameplayJudgeLaneValue(scoreSide, hit, value); }
    function gameplayJudgeValueForId(num) { return resultState.gameplayJudgeValueForId(num); }
    function recordGameplayJudgeTiming(scoreSide, hit) { resultState.recordGameplayJudgeTiming(scoreSide, hit); }
    function judgeTimingCount(counts, bucket, early) { return resultState.judgeTimingCount(counts, bucket, early); }
    function judgeTimingNumberFromCounts(num, counts) { return resultState.judgeTimingNumberFromCounts(num, counts); }
    function resultCacheKey(score) { return resultState.resultCacheKey(score); }
    function resultJudgeTimingCounts(side) { return resultState.resultJudgeTimingCounts(side); }
    function resultTimingStats(side) { return resultState.resultTimingStats(side); }
    function signedAfterDot(value) { return resultState.signedAfterDot(value); }
    function resultGaugeInfo(side) { return resultState.resultGaugeInfo(side); }
    function resultGaugeValue(side) { return resultState.resultGaugeValue(side); }
    function gaugeAfterDot(value) { return resultState.gaugeAfterDot(value); }
    function resultExScore(result) { return resultState.resultExScore(result); }
    function resultLr2Score(result) { return resultState.resultLr2Score(result); }
    function resultScorePrint(result) { return resultState.resultScorePrint(result); }
    function resultRateInteger(result) { return resultState.resultRateInteger(result); }
    function resultRateDecimal(result) { return resultState.resultRateDecimal(result); }
    function resultScoreRateInteger(points, result) { return resultState.resultScoreRateInteger(points, result); }
    function resultScoreRateDecimal(points, result) { return resultState.resultScoreRateDecimal(points, result); }
    function resultRawRank(result) { return resultState.resultRawRank(result); }
    function resultRankDelta(result) { return resultState.resultRankDelta(result); }
    function resultRankOptionForResult(result, baseOption) { return resultState.resultRankOptionForResult(result, baseOption); }
    function resultOldScores(side) { return resultState.resultOldScores(side); }
    function resultBestScoreByPoints(scores) { return resultState.resultBestScoreByPoints(scores); }
    function resultOldBestScore(side) { return resultState.resultOldBestScore(side); }
    function resultLastOldScore(side) { return resultState.resultLastOldScore(side); }
    function resultTargetSavedScore(side) { return resultState.resultTargetSavedScore(side); }
    function resultTargetFraction(side) { return resultState.resultTargetFraction(side); }
    function resultTargetPoints(side) { return resultState.resultTargetPoints(side); }
    function resultHighScorePoints(side) { return resultState.resultHighScorePoints(side); }
    function resultTargetMaxPoints(side) { return resultState.resultTargetMaxPoints(side); }
    function resultOldBestResult(side) { return resultState.resultOldBestResult(side); }
    function resultUpdatedBestResult(side) { return resultState.resultUpdatedBestResult(side); }
    function resultScoreImproved(side) { return resultState.resultScoreImproved(side); }
    function resultComboImproved(side) { return resultState.resultComboImproved(side); }
    function resultBadPoorImproved(side) { return resultState.resultBadPoorImproved(side); }
    function updateResultOldScores() { resultState.updateResultOldScores(); }
    function gameplayPlayer(side) {
        if (!root.chart) {
            return null;
        }
        return side === 2 ? root.chart.player2 : root.chart.player1;
    }

    function gameplayKeymode() {
        let chartData = root.gameplayChartData();
        return chartData ? chartData.keymode || 0 : 0;
    }

    function gameplayUsesDoublePlayLanes() {
        let keymode = root.gameplayKeymode();
        return keymode === 10 || keymode === 14;
    }

    function gameplayLanePlayer(side) {
        if (!root.chart) {
            return null;
        }
        if (side === 2) {
            return root.chart.player2 || (root.gameplayUsesDoublePlayLanes() ? root.chart.player1 : null);
        }
        return root.chart.player1;
    }

    function gameplayEngineColumnForLr2Lane(lane) {
        if (lane >= 10) {
            if (root.chart && root.chart.player2) {
                return lane === 10 ? 7 : lane - 11;
            }
            return lane === 10 ? 15 : 8 + lane - 11;
        }
        return lane === 0 ? 7 : lane - 1;
    }

    function gameplayLr2LaneForKeyTimer(timer) {
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

    function gameplayLr2LaneForLongNoteTimer(timer) {
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

    function gameplayCoursePlayer(side) {
        if (!root.chart) {
            return null;
        }
        return side === 2 ? root.chart.coursePlayer2 : root.chart.coursePlayer1;
    }

    function gameplayScore(side) {
        let player = root.gameplayPlayer(side);
        return player ? player.score : null;
    }

    function gameplayTotalNotes(score) {
        if (!score) {
            return 0;
        }
        let maxPoints = Math.floor(score.maxPoints || 0);
        if (maxPoints > 0) {
            return Math.floor(maxPoints / 2);
        }
        return Math.max(0, score.maxHits || 0);
    }

    function gameplayCurrentNotes(score) {
        if (!score) {
            return 0;
        }
        return Math.floor(Math.max(0, score.maxPointsNow || 0) / 2);
    }

    function gameplayExScore(score) {
        return score ? Math.floor(score.points || 0) : 0;
    }

    function gameplaySavedScorePoints(score) {
        return score && score.result ? Math.floor(score.result.points || 0) : 0;
    }

    function gameplayScoreRateInteger(points, score) {
        let denominator = root.gameplayTotalNotes(score) * 2;
        return denominator > 0 ? Math.floor(points * 100 / denominator) : 0;
    }

    function gameplayScoreRateDecimal(points, score) {
        let denominator = root.gameplayTotalNotes(score) * 2;
        return denominator > 0 ? Math.floor(points * 10000 / denominator) % 100 : 0;
    }

    function gameplayBestSavedScore() {
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

    function gameplayLastSavedScore() {
        root.gameplayScoresRevision;
        let scores = root.gameplayScores1 || [];
        return scores.length > 0 ? scores[0] : null;
    }

    function gameplayTargetSavedScore() {
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

    function gameplayTargetFraction() {
        let player = root.gameplayPlayer(1);
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        return vars ? (vars.targetScoreFraction || 0) : 0;
    }

    function gameplayHighScorePoints() {
        root.gameplayScoresRevision;
        return root.gameplayBestSavedScore() ? Math.floor(gameplayBestScoreReplayer.points || 0) : 0;
    }

    function gameplayTargetScorePoints() {
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

    function gameplayTargetFinalPoints() {
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

    function resetGameplayScoreReplayers() {
        gameplayTargetScoreReplayer.resetPoints();
        gameplayBestScoreReplayer.resetPoints();
    }

    function updateGameplaySavedScores() {
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

    function notifyGameplayReplayHit(hit) {
        if (root.gameplayTargetSavedScore()) {
            gameplayTargetScoreReplayer.notifyHit(hit);
        }
        if (root.gameplayBestSavedScore()) {
            gameplayBestScoreReplayer.notifyHit(hit);
        }
    }

    function gameplayJudgementCount(score, judgement) {
        return score && score.judgementCount ? score.judgementCount(judgement) : 0;
    }

    function gameplayPoorCount(score) {
        return root.gameplayJudgementCount(score, 0) + root.gameplayJudgementCount(score, 1);
    }

    function gameplayLr2Score(score) {
        let totalNotes = root.gameplayTotalNotes(score);
        if (totalNotes <= 0) {
            return 0;
        }
        let pgreat = root.gameplayJudgementCount(score, 5);
        let great = root.gameplayJudgementCount(score, 4);
        let good = root.gameplayJudgementCount(score, 3);
        return Math.floor((good + (great + pgreat * 2) * 2) * 50000 / totalNotes);
    }

    function gameplayScorePrint(score, chartData) {
        let value = root.gameplayLr2Score(score);
        let keymode = chartData ? chartData.keymode : 0;
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function lr2LinearValueByTime(from, to, start, end, now) {
        if (from === to) {
            return from;
        }
        if (now <= end && start <= now && start < end) {
            let ratio = (now - start) / (end - start);
            return (1.0 - ratio) * from + ratio * to;
        }
        return start < now ? to : from;
    }

    function gameplayScorePrintTargetValue(side) {
        return root.gameplayScorePrint(root.gameplayScore(side), root.gameplayChartData());
    }

    function gameplayDisplayedScorePrint(side) {
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

    function resetGameplayScorePrint() {
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

    function updateGameplayScorePrintTarget(side) {
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

    function gameplayCombo(side, maxCombo) {
        let coursePlayer = root.gameplayCoursePlayer(side);
        if (coursePlayer) {
            return maxCombo ? (coursePlayer.maxCombo || 0) : (coursePlayer.combo || 0);
        }
        let score = root.gameplayScore(side);
        return score ? (maxCombo ? (score.maxCombo || 0) : (score.combo || 0)) : 0;
    }

    function gameplayGaugeValue(score) {
        let gauge = root.gameplayActiveGauge(score);
        return gauge ? (gauge.gauge || 0) : 0;
    }

    function gameplayRateInteger(score, currentOnly) {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        return denominator > 0 ? Math.floor(root.gameplayExScore(score) * 100 / denominator) : 0;
    }

    function gameplayRateDecimal(score, currentOnly) {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        return denominator > 0 ? Math.floor(root.gameplayExScore(score) * 10000 / denominator) % 100 : 0;
    }

    function gameplayRankDelta(score) {
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

    function gameplayTimeSeconds(side, remaining) {
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

    function gameplayRawRank(score, currentOnly) {
        let notes = currentOnly ? root.gameplayCurrentNotes(score) : root.gameplayTotalNotes(score);
        let denominator = notes * 2;
        if (denominator <= 0) {
            return -1;
        }
        return Math.floor(root.gameplayExScore(score) * 9 / denominator);
    }

    function gameplayRankOption(score, baseOption, currentOnly) {
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

    function gameplayExactRankOption(score, baseOption) {
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

    function addGameplayGaugeRangeOption(options, score, baseOption) {
        let gauge = Math.floor(root.gameplayGaugeValue(score));
        let bucket = gauge >= 100 ? 10 : Math.max(0, Math.floor(gauge / 10));
        root.addOption(options, baseOption + bucket);
    }

    function gameplayGaugeQualified(score) {
        let gauge = root.gameplayActiveGauge(score);
        return gauge ? (gauge.gauge || 0) > (gauge.threshold || 0) : false;
    }

    function gameplayGaugeOption(side) {
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

    function gameplayLaneOption(score) {
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

    function gameplayLaneCoverOption(side) {
        let vars = root.generalVarsForSide(side);
        let hidden = !!vars && !!vars.hiddenOn;
        let sudden = !!vars && !!vars.laneCoverOn;
        if (hidden && sudden) {
            return 137;
        }
        if (hidden) {
            return 135;
        }
        if (sudden) {
            return 136;
        }
        return 134;
    }

    function gameplayJudgementOption(side, baseOption) {
        let judgement = side === 2 ? root.gameplayLastJudgement2 : root.gameplayLastJudgement1;
        return judgement >= 0 && judgement <= 5 ? baseOption + (5 - judgement) : -1;
    }

    function gameplayPoorBgaOption(side, baseOption) {
        let skinTime = side === 2 ? root.gameplayLastMissSkinTime2 : root.gameplayLastMissSkinTime1;
        return skinTime >= 0 && root.renderSkinTime - skinTime < 1000 ? baseOption + 1 : baseOption;
    }

    function gameplayPoorBgaVisible() {
        root.gameplayRevision;
        return root.gameplayPoorBgaOption(1, 247) === 248;
    }

    function judgementCountForExist(resultOrScore, judgement) {
        if (!resultOrScore) {
            return 0;
        }
        if (resultOrScore.judgementCount) {
            return resultOrScore.judgementCount(judgement);
        }
        return root.resultJudgementCount(resultOrScore, judgement);
    }

    function gameplaySudChanging(side) {
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

    function queueGameplayRevision() {
        if (!root.isGameplayScreen()) {
            root.gameplayRevision++;
            return;
        }
        if (root.gameplayRevisionQueued) {
            return;
        }
        root.gameplayRevisionQueued = true;
        Qt.callLater(function() {
            root.gameplayRevisionQueued = false;
            root.gameplayRevision++;
        });
    }

    function queueGameplayTimerRevision() {
        if (!root.isGameplayScreen()) {
            root.gameplayTimerRevision++;
            return;
        }
        if (root.gameplayTimerRevisionQueued) {
            return;
        }
        root.gameplayTimerRevisionQueued = true;
        Qt.callLater(function() {
            root.gameplayTimerRevisionQueued = false;
            root.gameplayTimerRevision++;
        });
    }

    function setGameplayTimerValue(timer, skinTime) {
        if (!timer || skinTime < 0) {
            return;
        }
        root.gameplayTimerValues[timer] = skinTime;
        root.queueGameplayTimerRevision();
    }

    function clearGameplayTimerValue(timer) {
        if (!timer || root.gameplayTimerValues[timer] === undefined) {
            return;
        }
        delete root.gameplayTimerValues[timer];
        root.queueGameplayTimerRevision();
    }

    function resetGameplayTimerValues() {
        let values = { "0": 0 };
        for (let timer = 120; timer <= 127; ++timer) {
            values[timer] = 0;
        }
        for (let timer = 130; timer <= 137; ++timer) {
            values[timer] = 0;
        }
        root.gameplayTimerValues = values;
        root.gameplayTimerRevisionQueued = false;
        root.gameplayTimerRevision++;
    }

    function resetGameplayTimers() {
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

    function updateGameplayStatusTimers() {
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
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
    }

    function gameplayLr2LaneForHit(side, hit) {
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

    function gameplayHitDisplaySide(scoreSide, hit) {
        if (scoreSide === 2) {
            return 2;
        }
        return root.gameplayLr2LaneForHit(scoreSide, hit) >= 10 ? 2 : 1;
    }

    function gameplayNoteForHit(side, hit) {
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

    function updateGameplayHitEffectTimers(side, hit) {
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

    function gameplayJudgementFromHit(hit) {
        return hit && hit.points && hit.points.judgement !== undefined
            ? hit.points.judgement
            : -1;
    }

    function gameplayJudgeComboForHit(scoreSide, judgement) {
        return judgement >= Judgement.Good && judgement <= Judgement.Perfect
            ? root.gameplayCombo(scoreSide, false)
            : 0;
    }

    function updateGameplayHitTimers(displaySide, hit, scoreSide) {
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

    function addGameplayTimer(result, timer, skinTime) {
        if (skinTime >= 0) {
            result[timer] = skinTime;
        }
    }

    function gameplayRhythmTimerSkinTime() {
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

    function addGameplayTimers(result) {
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

    function addGameplayEffectTimers(result) {
        for (let keyName in root.gameplayHitTimerStarts) {
            result[keyName] = root.gameplayHitTimerStarts[keyName];
        }
        for (let keyName in root.gameplayLongNoteTimerStarts) {
            result[keyName] = root.gameplayLongNoteTimerStarts[keyName];
        }
    }

    function initialGameplayOffButtonTimers() {
        let result = {};
        for (let timer = 120; timer <= 127; ++timer) {
            result[timer] = 0;
        }
        for (let timer = 130; timer <= 137; ++timer) {
            result[timer] = 0;
        }
        return result;
    }

    function gameplayKeyOnTimerForKey(key) {
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

    function gameplayKeyOffTimerForOnTimer(timer) {
        return ((timer >= 100 && timer <= 107) || (timer >= 110 && timer <= 117))
            ? timer + 20
            : 0;
    }

    function gameplayKeyTimerHeld(timer) {
        let columnState = root.gameplayColumnStateForKeyTimer(timer);
        return !!columnState && !!columnState.pressed;
    }

    function setGameplayKeyTimerPressed(timer, pressed) {
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

    function syncGameplayKeyTimerFromColumn(timer) {
        root.setGameplayKeyTimerPressed(timer, root.gameplayKeyTimerHeld(timer));
    }

    function syncGameplayKeyTimerFromColumnState(timer, columnState) {
        root.setGameplayKeyTimerPressed(timer, !!columnState && !!columnState.pressed);
    }

    function setGameplayLongNoteTimerHeld(timer, held) {
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

    function syncGameplayLongNoteTimerFromColumnState(timer, columnState) {
        root.setGameplayLongNoteTimerHeld(timer, !!columnState && !!columnState.holdingLongNote);
    }

    function gameplayColumnStateForLr2Lane(lane) {
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

    function gameplayColumnStateForKeyTimer(timer) {
        return root.gameplayColumnStateForLr2Lane(root.gameplayLr2LaneForKeyTimer(timer));
    }

    function gameplayColumnStateForLongNoteTimer(timer) {
        return root.gameplayColumnStateForLr2Lane(root.gameplayLr2LaneForLongNoteTimer(timer));
    }

    function syncGameplayKeyTimersFromColumns() {
        if (!root.isGameplayScreen() || !root.chart) {
            return;
        }

        for (let timer of root.gameplayKeyTimers) {
            root.syncGameplayKeyTimerFromColumnState(timer, root.gameplayColumnStateForKeyTimer(timer));
        }
    }

    function syncGameplayLongNoteTimersFromColumns() {
        if (!root.isGameplayScreen() || !root.chart) {
            return;
        }

        for (let timer of root.gameplayLongNoteTimers) {
            root.syncGameplayLongNoteTimerFromColumnState(
                timer,
                root.gameplayColumnStateForLongNoteTimer(timer));
        }
    }

    function pressGameplayButtonTimer(key) {
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

    function releaseGameplayButtonTimer(key) {
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

    function addGameplayKeyTimers(result) {
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

    // Bar delegates get per-row state from the select context; keep their option set stable.
    property var barActiveOptions: []
    property var baseActiveOptions: []
    readonly property var runtimeActiveOptions: root.isGameplayScreen()
        ? root.gameplayRuntimeActiveOptions
        : (root.effectiveScreenKey === "select"
            ? root.selectRuntimeActiveOptions
            : runtimeOptions.buildRuntimeActiveOptions(root.baseActiveOptions))
    readonly property var barTimers: ({ "0": 0 })

    function sameNumberArray(a, b) {
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

    function refreshBaseActiveOptions() {
        let nextBar = runtimeOptions.buildBarActiveOptions();
        let barChanged = !root.sameNumberArray(root.barActiveOptions, nextBar);
        if (barChanged) {
            root.barActiveOptions = nextBar;
        }

        let nextBase = runtimeOptions.buildBaseActiveOptions(nextBar);
        let baseChanged = !root.sameNumberArray(root.baseActiveOptions, nextBase);
        if (baseChanged) {
            root.baseActiveOptions = nextBase;
        }
        return barChanged || baseChanged;
    }

    function refreshSelectRuntimeActiveOptions() {
        if (!root.screenUpdatesActive || root.effectiveScreenKey !== "select") {
            return;
        }
        let next = runtimeOptions.buildRuntimeActiveOptions(root.baseActiveOptions);
        if (!root.sameNumberArray(root.selectRuntimeActiveOptions, next)) {
            root.selectRuntimeActiveOptions = next;
        }
    }

    function scheduleSelectRuntimeActiveOptionsRefresh() {
        root.refreshSelectRuntimeActiveOptions();
    }

    function scheduleSelectRankingStatusOptionsRefresh() {
        if (root.selectUsesRankingStatusOptions()) {
            root.scheduleSelectRuntimeActiveOptionsRefresh();
        }
    }

    function refreshGameplayRuntimeActiveOptions() {
        if (!root.isGameplayScreen()) {
            return;
        }
        root.gameplayRuntimeActiveOptions = runtimeOptions.buildRuntimeActiveOptions(root.baseActiveOptions);
    }

    function scheduleGameplayRuntimeActiveOptionsRefresh() {
        if (!root.isGameplayScreen()) {
            return;
        }
        if (root.gameplayRuntimeActiveOptionsRefreshQueued) {
            return;
        }
        root.gameplayRuntimeActiveOptionsRefreshQueued = true;
        Qt.callLater(function() {
            root.gameplayRuntimeActiveOptionsRefreshQueued = false;
            root.refreshGameplayRuntimeActiveOptions();
        });
    }

    readonly property int selectRevision: selectContext.scoreRevision
        + selectContext.focusRevision
    property alias deferredSelectChart: selectSideEffects.deferredChart
    property alias activePreviewSource: selectSideEffects.activePreviewSource
    property alias pendingPreviewRevision: selectSideEffects.pendingPreviewRevision
    property alias pendingPreviewRequest: selectSideEffects.pendingPreviewRequest
    property alias pendingPreviewSource: selectSideEffects.pendingPreviewSource
    property alias selectSideEffectsReady: selectSideEffects.ready
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? selectContext.cachedSelectedChartWrapper
        : selectSideEffects.renderChart
    readonly property var visualSelectChart: root.effectiveScreenKey === "select"
        ? selectContext.cachedSelectedChartWrapper
        : root.renderChart
    readonly property var selectedCourseStages: {
        let revision = root.effectiveScreenKey === "select" ? selectContext.selectionRevision : 0;
        if (root.effectiveScreenKey === "courseResult") {
            if (root.chartDatas && root.chartDatas.length > 0) {
                return root.chartDatas;
            }
            if (root.course && root.course.loadCharts) {
                return root.course.loadCharts();
            }
        }
        let item = root.effectiveScreenKey === "select" ? selectContext.focusedItem : null;
        if (!item || !selectContext.isCourse(item) || !item.loadCharts) {
            return [];
        }
        return item.loadCharts();
    }

    onSelectRevisionChanged: {
        root.handleCommittedSelectState();
        root.scheduleSelectRuntimeActiveOptionsRefresh();
    }
    Connections {
        target: selectContext
        function onFocusRevisionChanged() {
            if (root.effectiveScreenKey === "select") {
                root.restartSelectInfoTimer();
            }
        }

        function onSelectionRevisionChanged() {
            root.playSelectScratch();
        }
        function onRankingModeChanged() {
            if (root.refreshBaseActiveOptions()) {
                root.scheduleSelectRuntimeActiveOptionsRefresh();
            }
        }
    }
    Connections {
        target: lr2Ranking.rankingModel
        function onLoadingChanged() { root.scheduleSelectRankingStatusOptionsRefresh(); }
        function onRankingEntriesChanged() { root.scheduleSelectRankingStatusOptionsRefresh(); }
        function onPlayerCountChanged() { root.scheduleSelectRankingStatusOptionsRefresh(); }
        function onScoreCountChanged() { root.scheduleSelectRankingStatusOptionsRefresh(); }
        function onClearCountsChanged() { root.scheduleSelectRankingStatusOptionsRefresh(); }
        function onMd5Changed() { root.scheduleSelectRankingStatusOptionsRefresh(); }
    }
    onSelectPanelChanged: {
        if (root.refreshBaseActiveOptions()) {
            root.scheduleSelectRuntimeActiveOptionsRefresh();
        }
    }
    onLr2RankingTransitionPhaseChanged: {
        if (root.refreshBaseActiveOptions()) {
            root.scheduleSelectRuntimeActiveOptionsRefresh();
        }
    }
    onParseActiveOptionsChanged: {
        if (root.refreshBaseActiveOptions()) {
            root.scheduleSelectRuntimeActiveOptionsRefresh();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
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
            Qt.callLater(root.updateGameplayStatusTimers);
            Qt.callLater(root.activateGameplayIfNeeded);
        }
        if (root.isResultScreen()) {
            root.resultTimer151SkinTime = -1;
            root.resultTimer152SkinTime = -1;
            Qt.callLater(root.updateResultOldScores);
        }
        Qt.callLater(root.updateGameplaySavedScores);
        root.handleScreenContextChanged();
    }
    onChartChanged: {
        root.gameplayRevision++;
        root.scheduleGameplayRuntimeActiveOptionsRefresh();
        root.gameplayResultOpened = false;
        root.gameplayShowedCourseResult = false;
        root.gameplayPlayStopped = false;
        root.gameplayNothingWasHit = true;
        root.stopGameplayLifecycle();
        root.resetGameplayTimers();
        Qt.callLater(root.updateGameplayStatusTimers);
        Qt.callLater(root.updateGameplaySavedScores);
        Qt.callLater(root.activateGameplayIfNeeded);
        if (root.effectiveScreenKey !== "select") {
            root.handleExternalChartChanged();
        }
    }
    onScoresChanged: Qt.callLater(root.updateResultOldScores)
    onProfilesChanged: Qt.callLater(root.updateResultOldScores)
    onChartDataChanged: Qt.callLater(root.updateResultOldScores)
    onChartDatasChanged: Qt.callLater(root.updateResultOldScores)
    onCourseChanged: Qt.callLater(root.updateResultOldScores)
    Connections {
        target: root.isGameplayScreen() ? root.chart : null
        ignoreUnknownSignals: true
        function onCurrentChartIndexChanged() {
            root.gameplayRevision++;
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
            root.gameplayResultOpened = false;
            root.gameplayPlayStopped = false;
            root.gameplayNothingWasHit = true;
            root.resetGameplayTimers();
            Qt.callLater(root.updateGameplayStatusTimers);
            Qt.callLater(root.updateGameplaySavedScores);
            Qt.callLater(root.activateGameplayIfNeeded);
        }
        function onStatusChanged() {
            root.gameplayRevision++;
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
            root.handleGameplayStatusChanged();
        }
    }
    Connections {
        target: root.isGameplayScreen() ? root.gameplayScore(1) : null
        ignoreUnknownSignals: true
        function onHit(hit) {
            root.notifyGameplayReplayHit(hit);
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
            if (root.gameplayHitCountsAsPlayed(hit)) {
                root.gameplayNothingWasHit = false;
            }
            root.updateGameplayHitTimers(root.gameplayHitDisplaySide(1, hit), hit, 1);
        }
        function onPointsChanged() {
            root.updateGameplayScorePrintTarget(1);
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onComboChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onMaxComboChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onMaxPointsNowChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
    }
    Connections {
        target: root.isGameplayScreen() ? root.gameplayScore(2) : null
        ignoreUnknownSignals: true
        function onHit(hit) {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
            if (root.gameplayHitCountsAsPlayed(hit)) {
                root.gameplayNothingWasHit = false;
            }
            root.updateGameplayHitTimers(root.gameplayHitDisplaySide(2, hit), hit, 2);
        }
        function onPointsChanged() {
            root.updateGameplayScorePrintTarget(2);
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onComboChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onMaxComboChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
        function onMaxPointsNowChanged() {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
    }

    function handleCommittedSelectState() {
        if (root.commitLr2RankingRequest()) {
            root.applyRankingStatsToSelectContext();
        }
        root.updateSelectSideEffects();
    }

    function handleScreenContextChanged() {
        let rankingRequestChanged = root.commitLr2RankingRequest();
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        root.restartSelectInfoTimer();
        if (rankingRequestChanged) {
            root.applyRankingStatsToSelectContext();
        }
        root.updateSelectSideEffects();
    }

    function handleExternalChartChanged() {
        root.updateDisplayedSelectChart();
        root.commitLr2RankingRequest();
    }

    function updateDisplayedSelectChart() {
        selectSideEffects.updateDisplayedChart();
    }

    // Previews and ranking fetches are committed side effects; visual selection stays reactive.
    function updateSelectSideEffects() {
        selectSideEffects.update();
    }
    readonly property bool acceptsInput: screenState.acceptsInput
    onAcceptsInputChanged: {
        if (root.effectiveScreenKey === "select" && root.acceptsInput) {
            root.selectNoScrollStartSkinTime = root.renderSkinTime;
        }
        if (root.acceptsInput && root.anyStartHeld && !root.startHoldSuppressed) {
            root.holdSelectPanel(1);
        }
    }

    function optionText(labels, index) {
        return valueResolver.optionText(labels, index);
    }

    function clearLabelForLamp(lamp) {
        return valueResolver.clearLabelForLamp(lamp);
    }

    function courseStage(index) {
        return valueResolver.courseStage(index);
    }

    function chartTitle(chart) {
        return valueResolver.chartTitle(chart);
    }

    function chartSubtitle(chart) {
        return valueResolver.chartSubtitle(chart);
    }

    function lr2SelectOptionText(st) {
        return valueResolver.lr2SelectOptionText(st);
    }

    function resolveText(st, revision) {
        return valueResolver.resolveText(st, revision);
    }

    function resolveGameplayNumber(num) {
        return valueResolver.resolveGameplayNumber(num);
    }

    function resultCompareResult() {
        return valueResolver.resultCompareResult();
    }

    function resolveResultSideNumber(num, result) {
        return valueResolver.resolveResultSideNumber(num, result);
    }

    function resolveResultTargetSideNumber(num, side) {
        return valueResolver.resolveResultTargetSideNumber(num, side);
    }

    function resolveResultNumber(num) {
        return valueResolver.resolveResultNumber(num);
    }

    function resolveNumber(num) {
        return valueResolver.resolveNumber(num);
    }

    function numberValue(src, revision) {
        return valueResolver.numberValue(src, revision);
    }

    function imageSetValue(imageSetRef, sourceCount) {
        return valueResolver.imageSetValue(imageSetRef, sourceCount);
    }

    function imageSetSourceFor(src) {
        return valueResolver.imageSetSourceFor(src);
    }

    function numberForceHidden(src) {
        return valueResolver.numberForceHidden(src);
    }

    function numberAnimationRevision(src) {
        return valueResolver.numberAnimationRevision(src);
    }

    function resolveBarGraph(type) {
        return valueResolver.resolveBarGraph(type);
    }

    function normalizedBarValue(value, maximum) {
        return valueResolver.normalizedBarValue(value, maximum);
    }

    function resultBarGraphValue(type) {
        return valueResolver.resultBarGraphValue(type);
    }

    function sourceHasFrameAnimation(src) {
        return src
            && (src.cycle || 0) > 0
            && Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1) > 1;
    }

    function wrapValue(value, count) {
        return ((value % count) + count) % count;
    }

    function elementSourceFrameCount(src) {
        if (!src) {
            return 0;
        }
        if (src.imageSet) {
            return src.imageSetSources ? src.imageSetSources.length : 0;
        }
        return Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1);
    }

    function observeSelectSortButton(src) {
        return selectPanelController.observeSelectSortButton(src);
    }

    function buttonUsesSplitArrows(buttonId) {
        return selectPanelController.buttonUsesSplitArrows(buttonId);
    }

    function imageSetButtonId(src) {
        return selectPanelController.imageSetButtonId(src);
    }

    function elementButtonId(src) {
        return selectPanelController.elementButtonId(src);
    }

    function elementButtonPanel(src) {
        return selectPanelController.elementButtonPanel(src);
    }

    function elementButtonClickEnabled(src) {
        return selectPanelController.elementButtonClickEnabled(src);
    }

    function elementButtonPanelMatches(src) {
        return selectPanelController.elementButtonPanelMatches(src);
    }

    function buttonMouseDelta(src, mouseX, width) {
        return selectPanelController.buttonMouseDelta(src, mouseX, width);
    }

    function buttonFrame(src) {
        return selectPanelController.buttonFrame(src);
    }

    function closeSelectPanel() {
        return selectPanelController.closeSelectPanel();
    }

    function startSelectPanelCloseTimer(panel) {
        return selectPanelController.startSelectPanelCloseTimer(panel);
    }

    function openSelectPanel(panel, heldByStart) {
        return selectPanelController.openSelectPanel(panel, heldByStart);
    }

    function toggleSelectPanel(panel) {
        return selectPanelController.toggleSelectPanel(panel);
    }

    function holdSelectPanel(panel) {
        return selectPanelController.holdSelectPanel(panel);
    }

    function releaseHeldSelectPanel(panel) {
        return selectPanelController.releaseHeldSelectPanel(panel);
    }

    function currentSelectHeldButtonSkinTime() {
        return selectPanelController.currentSelectHeldButtonSkinTime();
    }

    function selectHeldButtonTimerForKey(key) {
        return selectPanelController.selectHeldButtonTimerForKey(key);
    }

    function isSelectHeldButtonTimer(timer) {
        return selectPanelController.isSelectHeldButtonTimer(timer);
    }

    function pressSelectHeldButtonTimer(key) {
        return selectPanelController.pressSelectHeldButtonTimer(key);
    }

    function releaseSelectHeldButtonTimer(key) {
        return selectPanelController.releaseSelectHeldButtonTimer(key);
    }

    function addHeldButtonTimer(result, timer, held) {
        return selectPanelController.addHeldButtonTimer(result, timer, held);
    }

    function addHeldButtonTimers(result) {
        return selectPanelController.addHeldButtonTimers(result);
    }

    function selectHeldButtonTimerFireTime(timer) {
        return selectPanelController.selectHeldButtonTimerFireTime(timer);
    }

    function spriteSkinTime(src, dsts) {
        return selectPanelController.spriteSkinTime(src, dsts);
    }

    function buttonPanelMatches(src) {
        return selectPanelController.buttonPanelMatches(src);
    }

    function handleLr2Button(buttonId, delta, panel, soundPlayer, sourceCount) {
        return selectPanelController.handleLr2Button(buttonId, delta, panel, soundPlayer, sourceCount);
    }

    function triggerSelectPanelButton(buttonId, delta) {
        return selectPanelController.triggerSelectPanelButton(buttonId, delta);
    }

    function triggerPanelButtonForKey(p1ButtonId, p2ButtonId, key, delta) {
        return selectPanelController.triggerPanelButtonForKey(p1ButtonId, p2ButtonId, key, delta);
    }

    function keyUsesPlayer2(key) {
        return selectPanelController.keyUsesPlayer2(key);
    }

    function gameplayOptionSideForKey(key) {
        return selectPanelController.gameplayOptionSideForKey(key);
    }

    function gameplayOptionModifierHeldForKey(key) {
        return selectPanelController.gameplayOptionModifierHeldForKey(key);
    }

    function handleLr2GameplayOptionKey(key) {
        return selectPanelController.handleLr2GameplayOptionKey(key);
    }

    function handleLr2GameplayScratchTick(side, up) {
        return selectPanelController.handleLr2GameplayScratchTick(side, up);
    }

    function handleLr2GameplayArrow(key) {
        return selectPanelController.handleLr2GameplayArrow(key);
    }

    function handleSelectPanelKey(key) {
        return selectPanelController.handleSelectPanelKey(key);
    }

    function handleSelectWheel(wheel) {
        return selectPanelController.handleSelectWheel(wheel);
    }

    function onMouseStateContainsPoint(src, state, mx, my) {
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

    function onMouseElementStateAt(element, mx, my) {
        if (!element || !element.src || !element.src.onMouse || !root.panelMatches(element.src.hoverPanel || 0)) {
            return null;
        }
        if (element.staticState) {
            return root.onMouseStateContainsPoint(element.src, element.staticState, mx, my);
        }
        return root.onMouseSpriteStateAt(element.src, element.dsts, mx, my);
    }

    function onMouseSpriteStateAt(src, dsts, mx, my) {
        if (!src || !src.onMouse || !root.panelMatches(src.hoverPanel || 0)) {
            return null;
        }
        const timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        const state = Lr2Timeline.getCurrentStateFromTimerFire(
            dsts,
            root.renderSkinTime,
            root.skinTimerFireTime(timer),
            root.runtimeActiveOptions);
        return root.onMouseStateContainsPoint(src, state, mx, my);
    }

    function isNowJudgeSprite(src) {
        return root.isGameplayScreen()
            && src
            && (src.timer === 46 || src.timer === 47)
            && (src.w || 0) > 0
            && (src.h || 0) > 0
            && (src.op1 || 0) === 0;
    }

    function nowJudgeComboValue(src) {
        if (!src) {
            return 0;
        }
        return src.timer === 47 ? root.gameplayJudgeCombo2 : root.gameplayJudgeCombo1;
    }

    function nowJudgeState(src, dsts) {
        if (!root.isNowJudgeSprite(src)) {
            return null;
        }
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        let base = Lr2Timeline.getCurrentStateFromTimerFire(
            dsts,
            root.renderSkinTime,
            root.skinTimerFireTime(timer),
            root.dstsUseActiveOptions(dsts) ? root.runtimeActiveOptions : root.emptyActiveOptions);
        let combo = root.nowJudgeComboValue(src);
        if (!base || combo <= 0) {
            return base;
        }

        let shifted = root.copyObject(base);
        let digits = Math.abs(Math.round(combo)).toString().length;
        let comboDigitW = Math.max(1, Math.round((base.h || src.h || 30) * 22 / 30));
        shifted.x -= digits * comboDigitW * 0.5;
        return shifted;
    }

    function spriteStateOverride(src, dsts, skinTime) {
        if (root.isNowJudgeSprite(src)) {
            return root.nowJudgeState(src, dsts);
        }
        if (root.isSelectScrollSlider(src)) {
            return root.selectScrollSliderState(src, dsts, skinTime);
        }
        if (root.isGameplayProgressSlider(src)) {
            return root.gameplayProgressSliderState(src, dsts, skinTime);
        }
        if (root.isGameplayLaneCoverSlider(src)) {
            return root.gameplayLaneCoverSliderState(src, dsts, skinTime);
        }
        if (root.isLr2GenericSlider(src)) {
            return root.lr2GenericSliderState(src, dsts, skinTime);
        }
        return null;
    }

    function elementUsesSpriteStateOverride(src) {
        return root.isNowJudgeSprite(src)
            || root.isSelectScrollSlider(src)
            || root.isGameplayProgressSlider(src)
            || root.isGameplayLaneCoverSlider(src)
            || root.isLr2GenericSlider(src);
    }

    function spriteForceHidden(src, elementIndex) {
        if (src && src.onMouse) {
            return root.selectHoverRevision >= 0
                && root.selectHoverVisibleByIndex[String(elementIndex)] !== true;
        }
        return false;
    }

    function elementUsesSpriteForceHidden(src) {
        return !!(src && src.onMouse);
    }

    function elementUsesButtonFrameOverride(src) {
        return root.effectiveScreenKey === "select"
            && !!src
            && !!src.button;
    }

    function translatedSliderState(src, dsts, position, skinTime) {
        return skinSliderState.translatedState(src, dsts, position, skinTime);
    }

    function selectScrollSliderState(src, dsts, skinTime) {
        return skinSliderState.selectScrollState(src, dsts, skinTime);
    }

    function lr2GenericSliderState(src, dsts, skinTime) {
        return skinSliderState.genericState(src, dsts, skinTime);
    }

    function gameplayProgressSliderState(src, dsts, skinTime) {
        return skinSliderState.gameplayProgressState(src, dsts, skinTime);
    }

    function gameplayLaneCoverSliderState(src, dsts, skinTime) {
        return skinSliderState.gameplayLaneCoverState(src, dsts, skinTime);
    }

    function selectScrollSliderTrackState(src, dsts, skinTime) {
        return skinSliderState.selectScrollTrackState(src, dsts, skinTime);
    }

    function lr2GenericSliderTrackState(src, dsts, skinTime) {
        return skinSliderState.genericTrackState(src, dsts, skinTime);
    }

    function setSelectScrollFromSliderPointer(src, dsts, pointerX, pointerY) {
        skinSliderState.setSelectScrollFromPointer(src, dsts, pointerX, pointerY);
    }

    function setSelectScrollFromSliderTrack(src, track, pointerX, pointerY) {
        skinSliderState.setSelectScrollFromTrack(src, track, pointerX, pointerY);
    }

    function setLr2GenericSliderFromPointer(src, dsts, pointerX, pointerY) {
        skinSliderState.setGenericFromPointer(src, dsts, pointerX, pointerY);
    }

    function setLr2GenericSliderFromTrack(src, track, pointerX, pointerY) {
        skinSliderState.setGenericFromTrack(src, track, pointerX, pointerY);
    }

    readonly property var cachedBarBaseStates: selectBarGeometry.cachedBarBaseStates
    readonly property var cachedBarPositionCache: selectBarGeometry.barPositionCache
    readonly property bool fastBarScrollActive: selectBarGeometry.fastBarScrollActive
    readonly property real fastBarScrollX: selectBarGeometry.fastBarScrollX
    readonly property real fastBarScrollY: selectBarGeometry.fastBarScrollY
    readonly property real selectedFastBarDrawX: selectBarGeometry.selectedFastBarDrawX
    readonly property real selectedFastBarDrawY: selectBarGeometry.selectedFastBarDrawY

    function selectedBarRow() {
        return selectBarGeometry.selectedBarRow();
    }

    function barClickStart() {
        return selectBarGeometry.barClickStart();
    }

    function barClickEnd() {
        return selectBarGeometry.barClickEnd();
    }

    function barRowCanClick(row) {
        return selectBarGeometry.barRowCanClick(row);
    }

    function barRowScrollDelta(row) {
        return selectBarGeometry.barRowScrollDelta(row);
    }

    function handleBarRowClick(row, mouse) {
        return selectBarGeometry.handleBarRowClick(row, mouse);
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
        globalSkinTime: root.globalSkinTime
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
    readonly property var timers: skinTiming.timers

    onRenderSkinTimeChanged: {
        if (root.effectiveScreenKey === "select"
                && root.selectHoverHasPoint
                && root.globalSkinTime < root.selectAnimationLimit) {
            root.scheduleSelectHoverRefresh();
        }
    }

    function selectUsesReplayOptions() {
        return root.skinUsesAnyOption([
            196, 197, 1196, 1197, 1199, 1200,
            1202, 1203, 1205, 1206, 1207, 1208
        ]);
    }

    function selectUsesScoreOptionIds() {
        return root.skinUsesOptionRange(105, 130)
            || root.skinUsesAnyOption([144, 145, 1100, 1102, 1103, 1104, 1128]);
    }

    function selectUsesEntryStatusOptions() {
        return root.skinUsesOptionRange(100, 130)
            || root.skinUsesOptionRange(200, 207)
            || root.skinUsesAnyOption([1100, 1102, 1103, 1104]);
    }

    function selectUsesDifficultyBarOptions() {
        return root.skinUsesOptionRange(70, 79)
            || root.skinUsesOptionRange(500, 565);
    }

    function selectUsesCourseDetailOptions() {
        return root.skinUsesAnyOption([290, 293])
            || root.skinUsesOptionRange(580, 589)
            || root.skinUsesOptionRange(700, 755);
    }

    function selectUsesRankingStatusOptions() {
        return root.skinUsesOptionRange(600, 616);
    }

    function updateSelectAnimationLimits() {
        skinTiming.updateSelectAnimationLimits();
    }

    function restartSkinClock() {
        skinTiming.restartSkinClock();
    }

    function restartSelectInfoTimer() {
        skinTiming.restartSelectInfoTimer();
    }

    function quantizedSkinClock(now) {
        return skinTiming.quantizedSkinClock(now);
    }

    function gameplayTimerFireTime(timer) {
        return skinTiming.gameplayTimerFireTime(timer);
    }

    function resultTimerFireTime(timer) {
        return skinTiming.resultTimerFireTime(timer);
    }

    function selectTimerFireTime(timer) {
        return skinTiming.selectTimerFireTime(timer);
    }

    function skinTimerFireTime(timer) {
        return skinTiming.skinTimerFireTime(timer);
    }

    Lr2SelectContext {
        id: selectContext
        updatesActive: root.screenUpdatesActive && root.effectiveScreenKey === "select"
        enabled: updatesActive
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

    Lr2PlayContext {
        id: playContext
        enabled: root.isGameplayScreen()
        screenRoot: root
        renderSkinTime: root.renderSkinTime
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
            Qt.callLater(root.activateGameplayIfNeeded);
            root.refreshLr2SkinSettingItems();
            root.refreshBaseActiveOptions();
            root.refreshSelectRuntimeActiveOptions();
            Qt.callLater(root.refreshGameplayRuntimeActiveOptions);
            Qt.callLater(root.updateResultOldScores);
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
        Qt.callLater(root.activateGameplayIfNeeded);
        root.refreshLr2SkinSettingItems();
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
    }

    Component.onCompleted: {
        root.componentReady = true;
        root.selectSideEffectsReady = true;
        root.commitLr2RankingRequest();
        root.restartSkinClock();
        root.openSelectIfNeeded();
        Qt.callLater(root.activateGameplayIfNeeded);
        root.updateSelectSideEffects();
        Qt.callLater(root.updateGameplaySavedScores);
        root.refreshLr2SkinSettingItems();
        root.refreshBaseActiveOptions();
        root.refreshSelectRuntimeActiveOptions();
        Qt.callLater(root.refreshGameplayRuntimeActiveOptions);
    }

    function pauseScreenActivity() {
        selectHoverState.stopRefresh();
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

    function stopSelectAudio() {
        selectSideEffects.stopAudio();
        openFolderSound.stop();
        closeFolderSound.stop();
        optionOpenSound.stop();
        optionCloseSound.stop();
        optionChangeSound.stop();
    }

    function playOneShot(player) {
        if (!root.enabled || !player || !player.source) {
            return;
        }
        player.stop();
        player.play();
    }

    function reloadCurrentSelectFolder() {
        if (root.effectiveScreenKey !== "select" || selectContext.historyStack.length === 0) {
            return false;
        }
        let currentFolder = selectContext.historyStack[selectContext.historyStack.length - 1];
        selectContext.open(currentFolder, selectContext.focusedItem);
        return true;
    }

    function selectGoBack() {
        if (root.closeLr2Ranking()) {
            return;
        }
        let before = selectContext.historyStack.length;
        selectContext.goBack();
        if (selectContext.historyStack.length < before) {
            root.playOneShot(closeFolderSound);
        }
    }

    function selectGoForward(item, autoplay, replay, replayScore) {
        let targetItem = item === undefined ? selectContext.activationItem() : item;
        let before = selectContext.historyStack.length;
        selectContext.goForward(targetItem, autoplay, replay, replayScore);
        if (selectContext.historyStack.length > before) {
            root.playOneShot(openFolderSound);
        }
    }

    function playSelectScratch() {
        selectSideEffects.playScratch();
    }

    function resultInputReady() {
        return root.enabled && root.isResultScreen() && root.acceptsInput;
    }

    function closeResultScreen() {
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

    function submitSelectSearch() { selectSearchState.submit(); }

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
        if (!root.selectScrollReady()) return;
        event.accepted = true;
        selectContext.decrementViewIndex(event.isAutoRepeat);
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
        if (!root.selectScrollReady()) return;
        event.accepted = true;
        selectContext.incrementViewIndex(event.isAutoRepeat);
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

    function navigate(number, type, up, key) {
        if (!root.selectScrollReady() || root.lastNavigateKey[root.lastNavigateKey.length - 1] !== key) {
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

    function resetLr2SelectScratchRepeat() {
        root.selectTargetScratchDirection = 0;
        root.selectTargetScratchNextMs = 0;
    }

    function releaseLr2SelectScratchRepeat(up) {
        let sameDirectionStillHeld = up
            ? (Input.col1sUp || Input.col2sUp)
            : (Input.col1sDown || Input.col2sDown);
        if (!sameDirectionStillHeld
                && root.selectTargetScratchDirection === (up ? 1 : -1)) {
            root.resetLr2SelectScratchRepeat();
        }
    }

    function handleLr2SelectScratchTick(side, up, number, type) {
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
                && !root.handleLr2GameplayScratchTick(1, false)) {
            root.navigate(number, type, false, BmsKey.Col1sDown);
        }
    }
    Input.onCol1sUpTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(1, true, number, type)
                && !root.handleLr2GameplayScratchTick(1, true)) {
            root.navigate(number, type, true, BmsKey.Col1sUp);
        }
    }
    Input.onCol2sDownTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(2, false, number, type)
                && !root.handleLr2GameplayScratchTick(2, false)) {
            root.navigate(number, type, false, BmsKey.Col2sDown);
        }
    }
    Input.onCol2sUpTicked: (number, type) => {
        if (!root.handleLr2SelectScratchTick(2, true, number, type)
                && !root.handleLr2GameplayScratchTick(2, true)) {
            root.navigate(number, type, true, BmsKey.Col2sUp);
        }
    }
    Input.onCol1sDownPressed: if (root.selectScrollReady() && root.selectPanel <= 0) root.lastNavigateKey.push(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: if (root.selectScrollReady() && root.selectPanel <= 0) root.lastNavigateKey.push(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: if (root.selectScrollReady() && root.selectPanel <= 0) root.lastNavigateKey.push(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: if (root.selectScrollReady() && root.selectPanel <= 0) root.lastNavigateKey.push(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: {
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sDown);
        root.releaseLr2SelectScratchRepeat(false);
    }
    Input.onCol1sUpReleased: {
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sUp);
        root.releaseLr2SelectScratchRepeat(true);
    }
    Input.onCol2sDownReleased: {
        root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sDown);
        root.releaseLr2SelectScratchRepeat(false);
    }
    Input.onCol2sUpReleased: {
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
