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
    readonly property var emptyActiveOptions: []
    readonly property var zeroTimers: ({ "0": 0 })
    property int gameplayRevision: 0
    property bool gameplayRevisionQueued: false
    property int gameplayTimerRevision: 0
    property bool gameplayTimerRevisionQueued: false
    property var gameplayTimerValues: ({ "0": 0 })
    property var gameplayRuntimeActiveOptions: []
    property bool gameplayRuntimeActiveOptionsRefreshQueued: false
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
    property var resultOldScores1: []
    property var resultOldScores2: []
    property int resultOldScoresRevision: 0
    property int resultOldScoresRequest: 0
    property int resultTimer151SkinTime: -1
    property int resultTimer152SkinTime: -1
    property int resultGraphStartSkinTime: 500
    property int resultGraphEndSkinTime: 2000
    readonly property var gameplayKeyTimers: [
        100, 101, 102, 103, 104, 105, 106, 107,
        110, 111, 112, 113, 114, 115, 116, 117
    ]

    onEnabledChanged: {
        if (root.effectiveScreenKey === "decide" && enabled) {
            Qt.callLater(() => sceneStack.pop());
        }
        if (enabled) {
            Qt.callLater(root.openSelectIfNeeded);
            Qt.callLater(root.activateGameplayIfNeeded);
        } else {
            root.stopSelectAudio();
            root.stopGameplayLifecycle();
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

    function openSelectIfNeeded() {
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
        enabled: root.enabled && !root.selectSearchHasFocus()
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

    StackView.onActivated: {
        Qt.callLater(root.activateGameplayIfNeeded);
    }
    
    readonly property string effectiveScreenKey: root.screenKey || root.inferScreenKey(root.csvPath)
    function playerIsAutoPlayer(player) {
        return !!player && player instanceof AutoPlayer;
    }

    function gameplayAutoplayActive() {
        return root.isGameplayScreen()
            && !!root.chart
            && root.playerIsAutoPlayer(root.chart.player1)
            && (!root.chart.player2 || root.playerIsAutoPlayer(root.chart.player2));
    }

    readonly property var parseActiveOptions: {
        let autoplayOn = root.gameplayAutoplayActive();
        let options = [
            0,  // DEFAULT is always true for #IF.
            20, // no select side panel active.
            30, // BGA size NORMAL.
            autoplayOn ? 33 : 32,
            34, // ghost off.
            38, // scoregraph off.
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
        return options;
    }
    property int selectPanel: 0
    property int selectPanelHeldByStart: 0
    property double selectPanelStartMs: 0
    property int selectPanelElapsed: 0
    property int selectPanelClosing: 0
    property double selectPanelCloseStartMs: 0
    property int selectPanelCloseElapsed: 0
    readonly property int selectPanelHoldTime: 250
    property int selectHeldButtonSkinTime: 0
    property var selectHeldButtonTimerStarts: ({})
    readonly property bool hasSelectHeldButtonTimers: Object.keys(root.selectHeldButtonTimerStarts).length > 0
    property bool anyStartHeld: Input.start1 || Input.start2
    property bool startHoldSuppressed: false

    onSelectPanelChanged: {
        if (root.selectPanel > 0) {
            root.clearSelectSearchFocus();
        }
    }

    onAnyStartHeldChanged: {
        if (root.anyStartHeld) {
            if (root.selectPanel > 0 && root.selectPanelHeldByStart === 0) {
                root.startHoldSuppressed = true;
                root.closeSelectPanel();
                return;
            }
            root.holdSelectPanel(1);
        } else {
            root.startHoldSuppressed = false;
            root.releaseHeldSelectPanel(1);
        }
    }

    readonly property var lr2GaugeLabels: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
    readonly property var lr2GaugeValues: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
    readonly property var lr2RandomLabels: ["OFF", "MIRROR", "RANDOM", "S-RANDOM", "R-RANDOM", "RANDOM+", "S-RANDOM+"]
    readonly property var lr2RandomValues: [
        NoteOrderAlgorithm.Normal,
        NoteOrderAlgorithm.Mirror,
        NoteOrderAlgorithm.Random,
        NoteOrderAlgorithm.SRandom,
        NoteOrderAlgorithm.RRandom,
        NoteOrderAlgorithm.RandomPlus,
        NoteOrderAlgorithm.SRandomPlus
    ]
    readonly property var lr2HiSpeedFixLabels: ["OFF", "MAIN", "START", "MAXBPM", "MINBPM", "AVERAGE"]
    readonly property var lr2HiSpeedFixValues: [
        HiSpeedFix.Off,
        HiSpeedFix.Main,
        HiSpeedFix.Start,
        HiSpeedFix.Max,
        HiSpeedFix.Min,
        HiSpeedFix.Avg
    ]
    readonly property var lr2BattleLabels: ["OFF", "BATTLE", "SP TO DP"]
    readonly property var lr2TargetLabels: ["GRADE", "BEST SCORE", "LAST SCORE"]
    readonly property var lr2TargetValues: [ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore]
    readonly property var lr2HidSudLabels: ["OFF", "HIDDEN", "SUDDEN", "HID+SUD"]
    readonly property var lr2ReplayLabels: ["NEWEST", "BEST SCORE", "BEST CLEAR", "BEST COMBO"]
    readonly property string lr2SearchPlaceholderText: "検索語を入力"
    property int lr2ReplayType: 0
    property var lr2SliderValues: ({
        "8": 0,
        "10": 50, "11": 50, "12": 50, "13": 50, "14": 50, "15": 50, "16": 50,
        "17": 100, "18": 100, "19": 100,
        "20": 50, "21": 50, "22": 50, "23": 50, "24": 50, "25": 50,
        "26": 50
    })
    property bool lr2EqOn: false
    property bool lr2PitchOn: false
    property int lr2PitchType: 0
    property var lr2FxType: [0, 0, 0]
    property var lr2FxOn: [false, false, false]
    property var lr2FxTarget: [0, 0, 0]
    property string lr2ReadmeText: ""
    property var lr2ReadmeLines: []
    property int lr2ReadmeMode: 0 // 0=hidden, 1=open, 2=closing
    property double lr2ReadmeStartMs: 0
    property int lr2ReadmeElapsed: 0
    property real lr2ReadmeOffsetX: 0
    property real lr2ReadmeOffsetY: 0
    property int lr2ReadmeLineSpacing: 18
    property bool lr2ReadmeMouseHeld: false
    property real lr2ReadmeMouseX: 0
    property real lr2ReadmeMouseY: 0
    property real selectMouseX: -100000
    property real selectMouseY: -100000
    property int selectSliderFixedPoint: -1
    property bool selectScratchSoundReady: false
    readonly property bool selectAudioActive: root.enabled && root.effectiveScreenKey === "select"

    onSelectAudioActiveChanged: {
        if (!root.selectAudioActive) {
            root.stopSelectAudio();
        }
    }

    function updateSelectMousePosition(x, y) {
        if (root.effectiveScreenKey !== "select") {
            return;
        }
        root.selectMouseX = x;
        root.selectMouseY = y;
    }

    function hideSelectMouse() {
        root.selectMouseX = -100000;
        root.selectMouseY = -100000;
    }

    function updateSelectMouseFromArea(area, mouse) {
        if (root.effectiveScreenKey !== "select" || !area || !mouse) {
            return;
        }
        if (area.parent === skinContainer) {
            root.updateSelectMousePosition(area.x + mouse.x, area.y + mouse.y);
            return;
        }
        const point = area.mapToItem(skinContainer, mouse.x, mouse.y);
        root.updateSelectMousePosition(point.x, point.y);
    }

    function copyObject(object) {
        let result = {};
        for (let key in object) {
            result[key] = object[key];
        }
        return result;
    }

    function setArrayValue(array, index, value) {
        let copy = array.slice();
        copy[index] = value;
        return copy;
    }

    function sliderInitialValue(type) {
        if (type === 17 || type === 18 || type === 19) {
            return 100;
        }
        if (type === 8) {
            return 0;
        }
        return 50;
    }

    function sliderRawValue(type) {
        let key = String(type);
        let value = root.lr2SliderValues[key];
        return value === undefined ? root.sliderInitialValue(type) : value;
    }

    function setSliderRawValue(type, value) {
        let key = String(type);
        let rounded = Math.max(0, Math.min(100, Math.round(value)));
        if (root.sliderRawValue(type) === rounded && root.lr2SliderValues[key] !== undefined) {
            return;
        }
        let copy = root.copyObject(root.lr2SliderValues);
        copy[key] = rounded;
        root.lr2SliderValues = copy;
    }

    function lr2SliderNumber(num) {
        if (num >= 50 && num <= 56) {
            return root.sliderRawValue(10 + num - 50);
        }
        if (num >= 57 && num <= 59) {
            return root.sliderRawValue(17 + num - 57);
        }
        if (num >= 60 && num <= 65) {
            return root.sliderRawValue(20 + num - 60);
        }
        if (num === 66) {
            return root.sliderRawValue(26);
        }
        return 0;
    }

    function openReadmeText(text) {
        root.lr2ReadmeText = text || "";
        root.lr2ReadmeLines = root.lr2ReadmeText.length > 0
            ? root.lr2ReadmeText.split(/\r\n|\n|\r/)
            : [""];
        root.lr2ReadmeMode = 1;
        root.lr2ReadmeStartMs = Date.now();
        root.lr2ReadmeElapsed = 0;
        root.lr2ReadmeOffsetX = 0;
        root.lr2ReadmeOffsetY = 0;
        root.lr2ReadmeLineSpacing = 18;
        root.lr2ReadmeMouseHeld = false;
        readmeCloseTimer.stop();
        readmeStopwatch.restart();
        root.clearSelectSearchFocus();
    }

    function openReadmePath(path) {
        if (!path) {
            return false;
        }
        const text = Rg.fileQuery.readTextFile(path) || "";
        root.openReadmeText(text.length > 0
            ? text
            : "HELP FILE NOT FOUND\n\n" + path);
        return true;
    }

    function closeReadme() {
        if (root.lr2ReadmeMode === 0) {
            return;
        }
        root.lr2ReadmeMode = 2;
        root.lr2ReadmeStartMs = Date.now();
        root.lr2ReadmeElapsed = 0;
        root.lr2ReadmeMouseHeld = false;
        readmeStopwatch.restart();
        readmeCloseTimer.restart();
    }

    function readmeLinesForSource(src) {
        if (!src || !src.readme || root.lr2ReadmeMode === 0) {
            return [];
        }
        if ((root.lr2ReadmeMode === 1 && src.readmeId === 0)
                || (root.lr2ReadmeMode === 2 && src.readmeId === 1)) {
            return root.lr2ReadmeLines;
        }
        return [];
    }

    function readmeContentHeight() {
        return root.lr2ReadmeLines.length * Math.max(1, root.lr2ReadmeLineSpacing);
    }

    function clampReadmeOffsets() {
        const minY = Math.min(0, root.skinH - root.readmeContentHeight());
        root.lr2ReadmeOffsetX = Math.min(0, root.lr2ReadmeOffsetX);
        root.lr2ReadmeOffsetY = Math.max(minY, Math.min(0, root.lr2ReadmeOffsetY));
    }

    function scrollReadmeBy(dx, dy) {
        if (root.lr2ReadmeMode !== 1) {
            return false;
        }
        root.lr2ReadmeOffsetX += dx;
        root.lr2ReadmeOffsetY += dy;
        root.clampReadmeOffsets();
        return true;
    }

    function mainGeneralVars() {
        return Rg.profileList && Rg.profileList.mainProfile
            ? Rg.profileList.mainProfile.vars.generalVars
            : null;
    }

    function profileForSide(side) {
        if (side === 2 && Rg.profileList.battleActive && Rg.profileList.battleProfiles.player2Profile) {
            return Rg.profileList.battleProfiles.player2Profile;
        }
        if (side === 1 && Rg.profileList.battleActive && Rg.profileList.battleProfiles.player1Profile) {
            return Rg.profileList.battleProfiles.player1Profile;
        }
        return Rg.profileList.mainProfile;
    }

    function generalVarsForSide(side) {
        let profile = root.profileForSide(side);
        return profile ? profile.vars.generalVars : null;
    }

    function indexOfValue(values, value) {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return i;
            }
        }
        return 0;
    }

    function cycleArrayIndex(index, count, delta) {
        return root.wrapValue(index + delta, count);
    }

    function wrappedListValue(values, index) {
        return values[root.wrapValue(index, values.length)];
    }

    readonly property int lr2GaugeIndexP1: {
        let vars = root.generalVarsForSide(1);
        return vars ? root.indexOfValue(root.lr2GaugeValues, vars.gaugeType) : 0;
    }
    readonly property int lr2GaugeIndexP2: {
        let vars = root.generalVarsForSide(2);
        return vars ? root.indexOfValue(root.lr2GaugeValues, vars.gaugeType) : 0;
    }

    function setGaugeIndex(side, index) {
        let vars = root.generalVarsForSide(side);
        if (vars) {
            vars.gaugeType = root.wrappedListValue(root.lr2GaugeValues, index);
        }
    }

    readonly property int lr2RandomIndexP1: {
        let vars = root.generalVarsForSide(1);
        return vars ? root.indexOfValue(root.lr2RandomValues, vars.noteOrderAlgorithm) : 0;
    }
    readonly property int lr2RandomIndexP2: {
        let vars = root.generalVarsForSide(2);
        if (!vars) return 0;
        let value = !Rg.profileList.battleActive
            ? vars.noteOrderAlgorithmP2
            : vars.noteOrderAlgorithm;
        return root.indexOfValue(root.lr2RandomValues, value);
    }

    function setRandomIndex(side, index) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let value = root.wrappedListValue(root.lr2RandomValues, index);
        if (side === 2 && !Rg.profileList.battleActive) {
            vars.noteOrderAlgorithmP2 = value;
        } else {
            vars.noteOrderAlgorithm = value;
        }
    }

    readonly property int lr2HidSudIndexP1: {
        let vars = root.generalVarsForSide(1);
        if (!vars) return 0;
        return (vars.hiddenOn ? 1 : 0) + (vars.laneCoverOn ? 2 : 0);
    }
    readonly property int lr2HidSudIndexP2: {
        let vars = root.generalVarsForSide(2);
        if (!vars) return 0;
        return (vars.hiddenOn ? 1 : 0) + (vars.laneCoverOn ? 2 : 0);
    }

    function setHidSudIndex(side, index) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let normalized = root.wrapValue(index, root.lr2HidSudLabels.length);
        vars.hiddenOn = (normalized & 1) !== 0;
        vars.laneCoverOn = (normalized & 2) !== 0;
    }

    readonly property int lr2HiSpeedFixIndex: {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2HiSpeedFixValues, vars.hiSpeedFix) : 0;
    }

    function setHiSpeedFixIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.hiSpeedFix = root.wrappedListValue(root.lr2HiSpeedFixValues, index);
        }
    }

    readonly property int lr2BattleIndex: {
        let vars = root.mainGeneralVars();
        if (Rg.profileList.battleActive) {
            return 1;
        }
        if (vars && vars.dpOptions === DpOptions.Battle) {
            return 2;
        }
        return 0;
    }

    function ensureBattleProfiles() {
        let list = Rg.profileList;
        let p1 = list.battleProfiles.player1Profile || list.mainProfile;
        let p2 = list.battleProfiles.player2Profile;
        if (!p2 || p2 === p1) {
            p2 = null;
            for (let i = 0; list.profiles && i < list.profiles.length; ++i) {
                let candidate = list.at(i);
                if (candidate && candidate !== p1) {
                    p2 = candidate;
                    break;
                }
            }
        }
        if (!p2) {
            p2 = list.createProfile();
            if (p2 && p2.vars && p2.vars.generalVars) {
                p2.vars.generalVars.name = "Battle 2P";
            }
        }
        if (!p1 || !p2 || p1 === p2) {
            return false;
        }
        list.battleProfiles.player1Profile = p1;
        list.battleProfiles.player2Profile = p2;
        return true;
    }

    function setBattleIndex(index) {
        let vars = root.mainGeneralVars();
        let normalized = root.wrapValue(index, root.lr2BattleLabels.length);
        if (!vars) {
            return;
        }
        if (normalized === 1) {
            vars.dpOptions = DpOptions.Off;
            if (root.ensureBattleProfiles()) {
                Rg.profileList.battleActive = true;
            }
            return;
        }
        Rg.profileList.battleActive = false;
        vars.dpOptions = normalized === 2 ? DpOptions.Battle : DpOptions.Off;
    }

    readonly property int lr2FlipIndex: {
        let vars = root.mainGeneralVars();
        return vars && vars.dpOptions === DpOptions.Flip ? 1 : 0;
    }

    function setFlipIndex(index) {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return;
        }
        if (root.wrapValue(index, 2) === 1) {
            Rg.profileList.battleActive = false;
            vars.dpOptions = DpOptions.Flip;
        } else if (vars.dpOptions === DpOptions.Flip) {
            vars.dpOptions = DpOptions.Off;
        }
    }

    readonly property int lr2LaneCoverIndex: {
        let vars = root.mainGeneralVars();
        return vars && vars.laneCoverOn ? 1 : 0;
    }

    function setLaneCoverIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.laneCoverOn = root.wrapValue(index, 2) === 1;
        }
    }

    readonly property int lr2BgaIndex: {
        let vars = root.mainGeneralVars();
        return vars && !vars.bgaOn ? 1 : 0;
    }

    function setBgaIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.bgaOn = root.wrapValue(index, 2) === 0;
        }
    }

    readonly property int lr2ScoreTargetIndex: {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2TargetValues, vars.scoreTarget) : 0;
    }

    function setScoreTargetIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.scoreTarget = root.wrappedListValue(root.lr2TargetValues, index);
        }
    }

    readonly property int lr2TargetPercent: {
        let vars = root.mainGeneralVars();
        return vars ? Math.round((vars.targetScoreFraction || 0) * 100) : 80;
    }

    function setTargetPercent(percent) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.targetScoreFraction = Math.max(0, Math.min(1, percent / 100));
        }
    }

    readonly property int lr2HiSpeedP1: {
        let vars = root.generalVarsForSide(1);
        return !vars || vars.noteScreenTimeMillis <= 0
            ? 100
            : Math.max(1, Math.min(999, Math.round(100000 / vars.noteScreenTimeMillis)));
    }
    readonly property int lr2HiSpeedP2: {
        let vars = root.generalVarsForSide(2);
        return !vars || vars.noteScreenTimeMillis <= 0
            ? 100
            : Math.max(1, Math.min(999, Math.round(100000 / vars.noteScreenTimeMillis)));
    }

    function setHiSpeedNumber(side, value) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let clamped = Math.max(1, Math.min(999, value));
        vars.noteScreenTimeMillis = 100000 / clamped;
    }

    function adjustOffset(delta) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.offset = Math.max(-99, Math.min(99, (vars.offset || 0) + delta));
        }
    }

    function isLoggedIn() {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        return !!profile
            && profile.loginState === Profile.LoggedIn
            && !!profile.onlineUserData;
    }

    function currentLr2RankingChart() {
        if (root.effectiveScreenKey !== "select") {
            return null;
        }
        if (selectContext.cachedSelectedChartData) {
            return selectContext.cachedSelectedChartData;
        }
        if (selectContext.rankingMode && selectContext.rankingBaseItem) {
            return selectContext.rankingBaseItem;
        }

        let item = selectContext.current;
        return selectContext.isChart(item) || selectContext.isEntry(item) ? item : null;
    }

    readonly property var lr2RankingChart: root.currentLr2RankingChart()
    readonly property string lr2RankingMd5: root.lr2RankingChart && root.lr2RankingChart.md5
        ? String(root.lr2RankingChart.md5)
        : ""
    property string lr2RankingRequestMd5: ""
    property bool lr2RankingOpenWhenReady: false
    property bool lr2InternetRankingOpenWhenReady: false
    property int lr2RankingTransitionPhase: 0 // 175 before list swap, 176 after list swap.
    property string lr2RankingTransitionAction: ""
    property double lr2RankingTransitionStartMs: 0
    property int lr2RankingTransitionElapsed: 0
    readonly property int lr2RankingTransitionDuration: 120

    function commitLr2RankingRequest() {
        root.lr2RankingRequestMd5 = root.lr2RankingMd5;
    }

    function lr2RankingProviderEnum() {
        let vars = root.mainGeneralVars();
        return vars ? vars.lr2RankingProvider : OnlineRankingModel.RhythmGame;
    }

    function lr2RankingMatchesCurrentChart() {
        let targetMd5 = root.lr2RankingMd5.length > 0 ? root.lr2RankingMd5.toLowerCase() : "";
        let loadedMd5 = lr2OnlineRanking.md5 ? String(lr2OnlineRanking.md5).toLowerCase() : "";
        return targetMd5.length > 0 && targetMd5 === loadedMd5;
    }

    function lr2LocalRankingEntry() {
        let chart = root.lr2RankingChart;
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        if (!chart || !profile) {
            return null;
        }

        let scoreList = selectContext.entryScores(chart);
        let bestPointsScore = selectContext.bestScoreByPoints(scoreList);
        let bestClearScore = selectContext.scoreWithBestClear(scoreList);
        if (!bestPointsScore || !bestPointsScore.result || !bestClearScore || !bestClearScore.result) {
            return null;
        }

        let stats = selectContext.statsForScore(bestPointsScore);
        return {
            __lr2Local: true,
            userId: profile.onlineUserData ? profile.onlineUserData.userId : 0,
            userName: profile.vars.generalVars.name,
            bestClearType: bestClearScore.result.clearType || "NOPLAY",
            bestPoints: bestPointsScore.result.points || 0,
            maxPoints: bestPointsScore.result.maxPoints || 0,
            bestCombo: bestPointsScore.result.maxCombo || 0,
            bestComboBreaks: stats ? stats.badPoor : 0,
            scoreCount: scoreList.length
        };
    }

    function lr2RankingEntries() {
        if (!root.lr2RankingMatchesCurrentChart()) {
            return [];
        }

        let source = lr2OnlineRanking.rankingEntries || [];
        let entries = [];
        for (let i = 0; i < source.length; ++i) {
            entries.push(source[i]);
        }

        if (root.lr2RankingProviderEnum() !== OnlineRankingModel.LR2IR) {
            return entries;
        }

        let localEntry = root.lr2LocalRankingEntry();
        if (!localEntry) {
            return entries;
        }

        let result = [];
        let inserted = false;
        for (let entry of entries) {
            if (!inserted && localEntry.bestPoints > (entry.bestPoints || 0)) {
                result.push(localEntry);
                inserted = true;
            }
            result.push(entry);
        }
        if (!inserted) {
            result.push(localEntry);
        }
        return result;
    }

    function lr2RankingClearCounts(entries) {
        let counts = {};
        for (let entry of entries || []) {
            let clearType = entry.bestClearType || "NOPLAY";
            counts[clearType] = (counts[clearType] || 0) + 1;
        }
        return counts;
    }

    function lr2RankingPlayerCount(entries) {
        if (!root.lr2RankingMatchesCurrentChart()) {
            return 0;
        }
        let count = Number(lr2OnlineRanking.playerCount || 0);
        if (entries && entries.length > 0) {
            return Math.max(count, entries.length);
        }
        if (count <= 0) {
            let source = lr2OnlineRanking.rankingEntries || [];
            count = source.length || 0;
        }
        if (root.lr2RankingProviderEnum() === OnlineRankingModel.LR2IR && root.lr2LocalRankingEntry()) {
            count += 1;
        }
        return count;
    }

    function lr2RankingTotalPlayCount(entries) {
        let modelCount = lr2OnlineRanking.scoreCount || 0;
        let total = 0;
        for (let entry of entries || []) {
            total += entry.scoreCount || 0;
        }
        return Math.max(modelCount, total);
    }

    function lr2RankingProfileUserId() {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        if (!profile) {
            return 0;
        }
        if (root.lr2RankingProviderEnum() === OnlineRankingModel.Tachi) {
            return profile.tachiData ? Number(profile.tachiData.userId || 0) : 0;
        }
        return profile.onlineUserData ? Number(profile.onlineUserData.userId || 0) : 0;
    }

    function lr2RankingPlayerRank(entries) {
        let userId = root.lr2RankingProfileUserId();
        for (let i = 0; i < (entries || []).length; ++i) {
            let entry = entries[i];
            if (entry.__lr2Local || (userId > 0 && Number(entry.userId || 0) === userId)) {
                return i + 1;
            }
        }
        return 0;
    }

    function lr2RankingSnapshot() {
        let entries = root.lr2RankingEntries();
        return {
            entries: entries,
            clearCounts: root.lr2RankingClearCounts(entries),
            playerCount: root.lr2RankingPlayerCount(entries),
            totalPlayCount: root.lr2RankingTotalPlayCount(entries),
            playerRank: root.lr2RankingPlayerRank(entries)
        };
    }

    function applyRankingStatsToSelectContext() {
        let chart = root.lr2RankingChart;
        if (!chart || !chart.md5 || !root.lr2RankingMatchesCurrentChart() || lr2OnlineRanking.loading) {
            return;
        }
        let snapshot = root.lr2RankingSnapshot();
        selectContext.setRankingStats(
            chart.md5,
            snapshot.clearCounts,
            snapshot.playerCount,
            snapshot.totalPlayCount,
            snapshot.playerRank);
    }

    function handleRankingModelChanged(tryOpenRanking, tryOpenInternetRanking) {
        root.applyRankingStatsToSelectContext();
        if (tryOpenRanking && root.lr2RankingOpenWhenReady && !lr2OnlineRanking.loading) {
            Qt.callLater(root.finishOpenLr2Ranking);
        }
        if (tryOpenInternetRanking && root.lr2InternetRankingOpenWhenReady && !lr2OnlineRanking.loading) {
            Qt.callLater(() => {
                if (!root.finishOpenLr2InternetRanking()) {
                    root.lr2InternetRankingOpenWhenReady = false;
                }
            });
        }
    }

    function lr2RankingStatusOption() {
        let chart = root.lr2RankingChart;
        if (!chart || !chart.md5) {
            return 600;
        }
        if (!root.lr2RankingMatchesCurrentChart()) {
            return lr2OnlineRanking.loading ? 601 : 600;
        }
        if (lr2OnlineRanking.loading) {
            return 601;
        }
        return root.lr2RankingPlayerCount() > 0 ? 602 : 603;
    }

    function startLr2RankingTransition(action) {
        if (root.lr2RankingTransitionPhase !== 0) {
            return false;
        }
        root.lr2RankingTransitionAction = action;
        root.lr2RankingTransitionPhase = 175;
        root.lr2RankingTransitionStartMs = Date.now();
        root.lr2RankingTransitionElapsed = 0;
        return true;
    }

    function clearLr2RankingTransition() {
        root.lr2RankingTransitionPhase = 0;
        root.lr2RankingTransitionAction = "";
        root.lr2RankingTransitionElapsed = 0;
    }

    function enterLr2RankingPostSwapTimer() {
        root.lr2RankingTransitionPhase = 176;
        root.lr2RankingTransitionStartMs = Date.now();
        root.lr2RankingTransitionElapsed = 0;
    }

    function performLr2RankingOpen() {
        if (!root.lr2RankingMatchesCurrentChart() || lr2OnlineRanking.loading) {
            return false;
        }
        let snapshot = root.lr2RankingSnapshot();
        if (snapshot.entries.length === 0) {
            let chart = root.lr2RankingChart;
            if (chart && chart.md5) {
                selectContext.setRankingStats(chart.md5, {}, 0, 0, 0);
            }
            return false;
        }
        let opened = selectContext.showRanking(
            snapshot.entries,
            snapshot.clearCounts,
            snapshot.playerCount,
            snapshot.totalPlayCount,
            snapshot.playerRank);
        if (opened) {
            root.playOneShot(optionOpenSound);
        }
        return opened;
    }

    function performLr2RankingClose() {
        if (!selectContext.rankingMode) {
            return false;
        }
        let closed = selectContext.hideRanking();
        if (closed) {
            root.playOneShot(optionCloseSound);
        }
        return closed;
    }

    function advanceLr2RankingTransition() {
        if (root.lr2RankingTransitionPhase === 175) {
            let ok = root.lr2RankingTransitionAction === "close"
                ? root.performLr2RankingClose()
                : root.performLr2RankingOpen();
            if (!ok) {
                root.clearLr2RankingTransition();
                return;
            }
            root.enterLr2RankingPostSwapTimer();
            return;
        }
        root.clearLr2RankingTransition();
    }

    function requestLr2RankingFetch(chart) {
        let md5 = chart && chart.md5 ? String(chart.md5) : "";
        if (md5.length === 0) {
            return false;
        }

        root.lr2RankingOpenWhenReady = true;
        if (!lr2OnlineRanking.loading) {
            lr2OnlineRanking.refresh();
        }
        return false;
    }

    function lr2TachiKeymode(chart) {
        switch (chart ? (chart.keymode || 0) : 0) {
        case 5:
        case 7:
            return "7K";
        case 10:
        case 14:
            return "14K";
        default:
            return "";
        }
    }

    function lr2InternetRankingUrl(chart) {
        let md5 = chart && chart.md5 ? String(chart.md5) : "";
        if (md5.length === 0) {
            return "";
        }

        switch (root.lr2RankingProviderEnum()) {
        case OnlineRankingModel.RhythmGame: {
            let vars = root.mainGeneralVars();
            let baseUrl = vars && vars.websiteBaseUrl
                ? String(vars.websiteBaseUrl)
                : "https://rhythmgame.eu";
            while (baseUrl.length > 0 && baseUrl.charAt(baseUrl.length - 1) === "/") {
                baseUrl = baseUrl.substring(0, baseUrl.length - 1);
            }
            return baseUrl + "/charts/" + md5;
        }
        case OnlineRankingModel.Tachi: {
            if (!root.lr2RankingMatchesCurrentChart() || !lr2OnlineRanking.chartId) {
                return "";
            }
            let keymode = root.lr2TachiKeymode(chart);
            return keymode.length > 0
                ? "https://boku.tachi.ac/games/bms/" + keymode + "/charts/" + lr2OnlineRanking.chartId
                : "";
        }
        case OnlineRankingModel.LR2IR:
        default:
            return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5="
                + md5 + "#status";
        }
    }

    function finishOpenLr2InternetRanking() {
        let chart = root.lr2RankingChart;
        let url = root.lr2InternetRankingUrl(chart);
        if (url.length === 0) {
            return false;
        }
        root.lr2InternetRankingOpenWhenReady = false;
        let opened = Qt.openUrlExternally(url);
        if (opened === false) {
            console.warn("[LR2] Failed to open internet ranking URL: " + url);
        }
        return opened !== false;
    }

    function openLr2InternetRanking() {
        root.updateDisplayedSelectChart();
        let chart = root.lr2RankingChart;
        if (!chart || !chart.md5) {
            return false;
        }

        if (root.lr2RankingProviderEnum() === OnlineRankingModel.Tachi
                && (!root.lr2RankingMatchesCurrentChart()
                    || !lr2OnlineRanking.chartId
                    || lr2OnlineRanking.loading)) {
            root.lr2InternetRankingOpenWhenReady = true;
            if (!lr2OnlineRanking.loading) {
                lr2OnlineRanking.refresh();
            }
            return true;
        }

        return root.finishOpenLr2InternetRanking();
    }

    function finishOpenLr2Ranking() {
        if (root.lr2RankingTransitionPhase !== 0) {
            return true;
        }
        if (!root.lr2RankingMatchesCurrentChart() || lr2OnlineRanking.loading) {
            return false;
        }
        let snapshot = root.lr2RankingSnapshot();
        if (snapshot.entries.length === 0) {
            root.lr2RankingOpenWhenReady = false;
            let chart = root.lr2RankingChart;
            if (chart && chart.md5) {
                selectContext.setRankingStats(chart.md5, {}, 0, 0, 0);
            }
            return false;
        }
        root.lr2RankingOpenWhenReady = false;
        return root.startLr2RankingTransition("open");
    }

    function openLr2Ranking() {
        if (!root.selectNavigationReady() || root.selectPanel === 1) {
            return false;
        }
        if (root.lr2RankingTransitionPhase !== 0) {
            return true;
        }

        root.updateDisplayedSelectChart();
        let chart = root.lr2RankingChart;
        if (!chart || !chart.md5) {
            return false;
        }
        if (!root.lr2RankingMatchesCurrentChart()
            || lr2OnlineRanking.loading
            || root.lr2RankingPlayerCount() === 0) {
            return root.requestLr2RankingFetch(chart);
        }
        return root.finishOpenLr2Ranking();
    }

    function closeLr2Ranking() {
        root.lr2RankingOpenWhenReady = false;
        if (root.lr2RankingTransitionPhase !== 0) {
            return true;
        }
        if (!selectContext.rankingMode) {
            return false;
        }
        return root.startLr2RankingTransition("close");
    }

    function isLr2RankingKey(key) {
        return key === BmsKey.Col15 || key === BmsKey.Col25;
    }

    function clearStatusOption() {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return 62;
        }
        if (vars.gaugeMode === GaugeMode.Best) {
            return 62;
        }
        let gauge = String(vars.gaugeType || "").toUpperCase();
        if (gauge === "AEASY" || gauge === "EASY") {
            return 63;
        }
        if (gauge === "NORMAL") {
            return 64;
        }
        if (gauge === "HARD" || gauge === "EXHARD") {
            return 65;
        }
        if (gauge === "FC" || gauge === "PERFECT" || gauge === "MAX") {
            return 66;
        }
        return 62;
    }

    function clearStatusIsBest() {
        let vars = root.mainGeneralVars();
        return !!vars && vars.gaugeMode === GaugeMode.Best;
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        z: -1000000
    }

    function inferScreenKey(path) {
        let normalized = (path || "").replace(/\\/g, "/").toLowerCase();
        if (normalized.indexOf("/select/") !== -1 || normalized.endsWith("select.lr2skin") || normalized.endsWith("select.csv")) {
            return "select";
        }
        if (normalized.indexOf("/decide/") !== -1 || normalized.endsWith("decide.lr2skin") || normalized.endsWith("decide.csv")) {
            return "decide";
        }
        if (normalized.indexOf("/courseresult/") !== -1 || normalized.endsWith("courseresult.lr2skin") || normalized.endsWith("courseresult.csv")) {
            return "courseResult";
        }
        if (normalized.indexOf("/result/") !== -1 || normalized.endsWith("result.lr2skin") || normalized.endsWith("result.csv")) {
            return "result";
        }
        return "";
    }

    function isSelectScrollSlider(src) {
        return root.effectiveScreenKey === "select"
            && !!src
            && !!src.slider
            && src.sliderType === 1
            && src.sliderRange > 0
            && src.sliderDisabled === 0;
    }

    function isLr2GenericSlider(src) {
        return root.effectiveScreenKey === "select"
            && !!src
            && !!src.slider
            && !root.isSelectScrollSlider(src)
            && src.sliderRange > 0
            && src.sliderDisabled === 0;
    }

    function isGameplayProgressSlider(src) {
        return root.isGameplayScreen()
            && !!src
            && !!src.slider
            && src.sliderType === 6
            && src.sliderRange > 0;
    }

    function sliderTrackState(src, dsts) {
        if (!src || !src.slider) {
            return null;
        }
        let base = Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
        if (!base) {
            return null;
        }

        let track = {
            x: base.x,
            y: base.y,
            w: base.w,
            h: base.h
        };
        let range = Math.max(1, src.sliderRange || 0);
        switch (src.sliderDirection) {
        case 0:
            track.y -= range;
            track.h += range;
            break;
        case 1:
            track.w += range;
            break;
        case 2:
            track.h += range;
            break;
        case 3:
            track.x -= range;
            track.w += range;
            break;
        default:
            return null;
        }
        return track;
    }

    function sliderPositionFromPointer(src, track, pointerX, pointerY) {
        let range = Math.max(1, src.sliderRange || 0);
        let moveX = (track.w - range) / 2;
        let moveY = (track.h - range) / 2;
        let position = 0;
        switch (src.sliderDirection) {
        case 0: {
            let start = track.y + moveY;
            let end = track.y + track.h - moveY;
            position = end !== start ? (end - pointerY) / (end - start) : 0;
            break;
        }
        case 1: {
            let start = track.x + moveX;
            let end = track.x + track.w - moveX;
            position = end !== start ? (pointerX - start) / (end - start) : 0;
            break;
        }
        case 2: {
            let start = track.y + moveY;
            let end = track.y + track.h - moveY;
            position = end !== start ? (pointerY - start) / (end - start) : 0;
            break;
        }
        case 3: {
            let start = track.x + moveX;
            let end = track.x + track.w - moveX;
            position = end !== start ? (end - pointerX) / (end - start) : 0;
            break;
        }
        default:
            return 0;
        }
        return Math.max(0, Math.min(1, position));
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

    function isSelectSearchText(src) {
        return root.effectiveScreenKey === "select"
            && root.acceptsInput
            && src
            && src.st === 30
            && src.edit !== 0
            && root.panelMatches(src.panel || 0);
    }

    function selectSearchTextState(src, dsts) {
        if (!root.isSelectSearchText(src)) {
            return null;
        }
        return Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
    }

    function textPrefix(text, position) {
        const source = text || "";
        const clamped = Math.max(0, Math.min(position || 0, source.length));
        return source.substring(0, clamped);
    }

    property var selectSearchInputItem: null

    function selectSearchHasFocus() {
        return root.selectSearchInputItem && root.selectSearchInputItem.activeFocus;
    }

    function selectInputReady() {
        return root.enabled
            && root.effectiveScreenKey === "select"
            && root.acceptsInput
            && !root.selectSearchHasFocus();
    }

    function selectScrollReady() {
        return root.selectInputReady() && root.lr2ReadmeMode === 0;
    }

    function selectNavigationReady() {
        return root.selectInputReady()
            && root.lr2ReadmeMode === 0;
    }

    function focusSelectSearch() {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput || root.selectPanel > 0) {
            return;
        }
        if (!root.selectSearchInputItem) {
            return;
        }
        root.selectSearchInputItem.syncFromContext();
        root.selectSearchInputItem.cursorPosition = root.selectSearchInputItem.text.length;
        root.selectSearchInputItem.forceActiveFocus();
    }

    function clearSelectSearchFocus() {
        if (root.selectSearchHasFocus()) {
            root.forceActiveFocus();
        }
    }

    function resetSelectSearch() {
        if (selectContext.searchText.length > 0) {
            selectContext.searchText = "";
            selectContext.touch();
        }
        if (root.selectSearchInputItem && root.selectSearchInputItem.text.length > 0) {
            root.selectSearchInputItem.text = "";
        }
        root.clearSelectSearchFocus();
    }

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
            || (!!src && ((src.cycle || 0) > 0
                || (src.resultChartType || 0) > 0));
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
            || root.sourceCollectionUsesTimers(skinModel.lineSources);
    }

    function addOption(options, option) {
        if (option !== undefined && option !== null && options.indexOf(option) === -1) {
            options.push(option);
        }
    }

    function gaugeColorOption(side) {
        let vars = root.generalVarsForSide(side);
        let gauge = String(vars ? vars.gaugeType : "").toUpperCase();
        let red = gauge === "HARD" || gauge === "EXHARD"
            || gauge === "FC" || gauge === "PERFECT" || gauge === "MAX";
        return side === 2 ? (red ? 45 : 44) : (red ? 43 : 42);
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

    function dateTimeNumber(num) {
        let now = new Date();
        switch (num) {
        case 21:
            return now.getFullYear();
        case 22:
            return now.getMonth() + 1;
        case 23:
            return now.getDate();
        case 24:
            return now.getHours();
        case 25:
            return now.getMinutes();
        case 26:
            return now.getSeconds();
        default:
            return 0;
        }
    }

    function appendCommonRuntimeOptions(options, scoreGraphOn) {
        let vars = root.mainGeneralVars();
        root.addOption(options, 30); // BGA size NORMAL; LR2 EXTEND is not exposed here yet.
        if (root.isGameplayScreen()) {
            root.addOption(options, root.gameplayAutoplayActive() ? 33 : 32);
        } else {
            root.addOption(options, 32); // autoplay off unless a launch button explicitly requests it.
        }
        if (root.effectiveScreenKey === "decide") {
            root.addOption(options, 33); // LR2 decide reports both autoplay states true.
        }
        root.addOption(options, 34); // ghost off; ghost play is not exposed from select.
        root.addOption(options, scoreGraphOn ? 39 : 38);
        root.addOption(options, vars && vars.bgaOn === false ? 40 : 41);
        root.addOption(options, root.gaugeColorOption(1));
        root.addOption(options, root.gaugeColorOption(2));
        root.addOption(options, 46); // difficulty filter enabled.
        root.addOption(options, root.isLoggedIn() ? 51 : 50);
        root.addOption(options, 52); // extra mode off.
        root.addOption(options, 54); // 1P autoscratch/assist off.
        root.addOption(options, 56); // 2P autoscratch/assist off.
        root.addOption(options, 61);
        root.addOption(options, root.clearStatusOption());
        if (root.isGameplayScreen()) {
            // LR2 play skins use 80 while the chart is still loading, then
            // switch to 81 when timer 40 (READY / load complete) fires.
            root.addOption(options, root.gameplayReadySkinTime >= 0 ? 81 : 80);
        } else {
            root.addOption(options, 80); // select/decide are already loaded when the skin is shown.
        }
        root.addOption(options, 82); // no replay playback in select/decide UI.
        root.addOption(options, 572); // course editor/making mode is not supported here.
        root.addOption(options, 622); // ghost battle is not supported from select.
        root.addOption(options, 624); // rival compare is not supported from select.
    }

    function appendPanelOptions(options) {
        if (root.selectPanel > 0) {
            root.addOption(options, 20 + root.selectPanel);
        } else {
            root.addOption(options, 20);
        }
    }

    function appendChartOptions(options, chartData) {
        let allowStageFileOption = root.effectiveScreenKey !== "decide";
        if (!chartData) {
            if (allowStageFileOption) {
                addOption(options, 190);
            }
            addOption(options, 192);
            addOption(options, 194);
            addOption(options, 170);
            addOption(options, 172);
            addOption(options, 174);
            addOption(options, 176);
            addOption(options, 178);
            addOption(options, 196);
            addOption(options, 150);
            return;
        }
        if (allowStageFileOption) {
            addOption(options, chartData.stageFile ? 191 : 190);
        }
        addOption(options, chartData.banner ? 193 : 192);
        addOption(options, chartData.backBmp ? 195 : 194);
        addOption(options, selectContext.hasBga(chartData) ? 171 : 170);
        addOption(options, selectContext.hasLongNote(chartData) ? 173 : 172);
        addOption(options, selectContext.hasAttachedText(chartData) ? 175 : 174);
        addOption(options, (chartData.maxBpm || 0) !== (chartData.minBpm || 0) ? 177 : 176);
        addOption(options, chartData.isRandom ? 179 : 178);
        addOption(options, selectContext.judgeOption(chartData));
        addOption(options, selectContext.highLevelOption(chartData));
        addOption(options, selectContext.hasReplay(chartData) ? 197 : 196);

        let difficulty = selectContext.entryDifficulty(chartData);
        if (difficulty >= 1 && difficulty <= 5) {
            addOption(options, 150 + difficulty);
        } else {
            addOption(options, 150);
        }

        let keymode = chartData.keymode || 0;
        root.appendKeymodeOption(options, keymode, 160);
        root.appendKeymodeOption(options, root.keymodeAfterOptions(keymode), 165);
    }

    function appendKeymodeOption(options, keymode, baseOption) {
        switch (keymode) {
        case 7:
            root.addOption(options, baseOption);
            break;
        case 5:
            root.addOption(options, baseOption + 1);
            break;
        case 14:
            root.addOption(options, baseOption + 2);
            break;
        case 10:
            root.addOption(options, baseOption + 3);
            break;
        }
    }

    function keymodeAfterOptions(keymode) {
        if (!root.spToDpActive()) {
            return keymode;
        }
        if (keymode === 7) {
            return 14;
        }
        if (keymode === 5) {
            return 10;
        }
        return keymode;
    }

    function chartKeymodeForStatus(item, selectedChart) {
        if (selectedChart && selectedChart.keymode) {
            return selectedChart.keymode;
        }
        if ((selectContext.isChart(item) || selectContext.isEntry(item)) && item.keymode) {
            return item.keymode;
        }
        return 0;
    }

    function appendEntryStatusOptions(options, item, selectedChart) {
        let folderLike = selectContext.isFolderLikeForLamp(item);
        if (!selectContext.isChart(item)
                && !selectContext.isEntry(item)
                && !selectContext.isRankingEntry(item)
                && !folderLike) {
            return;
        }
        if (!folderLike && root.chartKeymodeForStatus(item, selectedChart) <= 0) {
            return;
        }
        let lamp = selectContext.entryLamp(item);
        if (lamp >= 0 && lamp <= 5) {
            root.addOption(options, 100 + lamp);
        }
        if (folderLike) {
            return;
        }

        let rank = selectContext.entryRank(item);
        if (lamp > 0 && rank >= 1) {
            root.addOption(options, 118 - Math.min(rank, 8));
        }
    }

    function appendCourseOptions(options, item) {
        if (!selectContext.isCourse(item)) {
            return;
        }
        root.addOption(options, 290);
        root.addOption(options, 293); // rank certification / class
        if (!item.loadCharts) {
            return;
        }

        let stages = item.loadCharts();
        for (let i = 0; i < Math.min(10, stages.length); ++i) {
            root.addOption(options, 580 + i);
        }
        for (let stage = 0; stage < Math.min(5, stages.length); ++stage) {
            let difficulty = selectContext.entryDifficulty(stages[stage]);
            root.addOption(options, 700 + stage * 10 + Math.max(0, Math.min(5, difficulty)));
        }
    }

    function appendDifficultyBarOptions(options) {
        for (let diff = 1; diff <= 5; ++diff) {
            if (selectContext.hasDifficulty(diff)) {
                root.addOption(options, 504 + diff);
                root.addOption(options, selectContext.difficultyLevelBarOption(diff));
            } else {
                root.addOption(options, 499 + diff);
            }

            let diffCount = selectContext.difficultyCount(diff);
            if (diffCount === 1) {
                root.addOption(options, 509 + diff);
            } else if (diffCount > 1) {
                root.addOption(options, 514 + diff);
            }
            root.addOption(options, 510 + diff * 10 + selectContext.difficultyLamp(diff));
        }
    }

    function appendSelectItemTypeOptions(options, item) {
        if (selectContext.isChart(item) || selectContext.isEntry(item) || selectContext.isRankingEntry(item)) {
            root.addOption(options, 2);
            root.addOption(options, 5);
        } else if (selectContext.isCourse(item)) {
            root.addOption(options, 3);
            root.addOption(options, 5);
            root.addOption(options, 290);
        } else {
            root.addOption(options, 1);
        }
    }

    function appendSelectedChartModeOptions(options, chartData) {
        let keymode = chartData ? (chartData.keymode || 0) : 0;
        let doubleMode = keymode === 10 || keymode === 14
            || ((keymode === 5 || keymode === 7) && root.spToDpActive());
        let battleMode = root.battleModeActive();
        if (doubleMode) {
            root.addOption(options, 10);
        }
        if (battleMode) {
            root.addOption(options, 11);
        }
        if (doubleMode || battleMode) {
            root.addOption(options, 12);
        }
        if (battleMode) {
            root.addOption(options, 13);
        }
    }

    function isGameplayScreen() {
        switch (root.effectiveScreenKey) {
        case "k5":
        case "k7":
        case "k10":
        case "k14":
        case "k5battle":
        case "k7battle":
        case "k10battle":
        case "k14battle":
            return true;
        default:
            return false;
        }
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
        gameplayStartTimer.stop();
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
            gameplayStartTimer.restart();
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
        gameplayStartTimer.stop();

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
            gameplayStartTimer.stop();
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

    function isResultScreen() {
        return root.effectiveScreenKey === "result"
            || root.effectiveScreenKey === "courseResult";
    }

    function resultScore(side) {
        return root.scores && root.scores.length >= side ? root.scores[side - 1] : null;
    }

    function resultData(side) {
        let score = root.resultScore(side);
        return score && score.result ? score.result : null;
    }

    function resultProfile(side) {
        return root.profiles && root.profiles.length >= side
            ? root.profiles[side - 1]
            : (Rg.profileList ? Rg.profileList.mainProfile : null);
    }

    function resultChartData() {
        if (root.effectiveScreenKey === "courseResult") {
            return null;
        }
        if (root.chartData) {
            return root.chartData;
        }
        return root.chartDatas && root.chartDatas.length > 0 ? root.chartDatas[0] : null;
    }

    function displayChartData() {
        if (root.isResultScreen()) {
            return root.resultChartData();
        }
        if (root.effectiveScreenKey === "select") {
            return selectContext.selectedChartData();
        }
        if (root.isGameplayScreen()) {
            return root.gameplayChartData();
        }
        return root.chart && root.chart.chartData ? root.chart.chartData : null;
    }

    function resultClearOption() {
        let result = root.resultData(1);
        let clearType = result ? String(result.clearType || "FAILED") : "FAILED";
        return clearType !== "FAILED" && clearType !== "NOPLAY" ? 90 : 91;
    }

    function resultTotalNotes(result) {
        if (!result) {
            return 0;
        }
        let maxPoints = Math.floor(result.maxPoints || 0);
        return maxPoints > 0 ? Math.floor(maxPoints / 2) : Math.max(0, result.maxHits || 0);
    }

    function resultJudgementCount(result, judgement) {
        let counts = result && result.judgementCounts ? result.judgementCounts : [];
        return judgement >= 0 && judgement < counts.length ? (counts[judgement] || 0) : 0;
    }

    function resultPoorCount(result) {
        return root.resultJudgementCount(result, Judgement.Poor)
            + root.resultJudgementCount(result, Judgement.EmptyPoor);
    }

    function resultBadPoor(result) {
        return root.resultJudgementCount(result, Judgement.Bad) + root.resultPoorCount(result);
    }

    function resultExScore(result) {
        return result ? Math.floor(result.points || 0) : 0;
    }

    function resultLr2Score(result) {
        let totalNotes = root.resultTotalNotes(result);
        if (totalNotes <= 0) {
            return 0;
        }
        let pgreat = root.resultJudgementCount(result, Judgement.Perfect);
        let great = root.resultJudgementCount(result, Judgement.Great);
        let good = root.resultJudgementCount(result, Judgement.Good);
        return Math.floor((good + (great + pgreat * 2) * 2) * 50000 / totalNotes);
    }

    function resultScorePrint(result) {
        let value = root.resultLr2Score(result);
        let chartData = root.resultChartData();
        let keymode = chartData ? chartData.keymode : (result ? result.keymode : 0);
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function resultRateInteger(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 100 / denominator) : 0;
    }

    function resultRateDecimal(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 10000 / denominator) % 100 : 0;
    }

    function resultScoreRateInteger(points, result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 100 / denominator) : 0;
    }

    function resultScoreRateDecimal(points, result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 10000 / denominator) % 100 : 0;
    }

    function resultRawRank(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        if (!result || denominator <= 0 || root.resultExScore(result) <= 0) {
            return -1;
        }
        return Math.floor(root.resultExScore(result) * 9 / denominator);
    }

    function resultRankDelta(result) {
        let totalNotes = root.resultTotalNotes(result);
        let perfectScore = totalNotes * 2;
        let exScore = root.resultExScore(result);
        if (totalNotes <= 0 || exScore === perfectScore) {
            return 0;
        }
        let rank = Math.floor(exScore * 9 / perfectScore);
        rank = Math.max(1, Math.min(8, rank));
        return exScore - Math.floor(perfectScore * (rank + 1) / 9);
    }

    function resultRankOptionForResult(result, baseOption) {
        let rank = root.resultRawRank(result);
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

    function resultOldScores(side) {
        root.resultOldScoresRevision;
        return side === 2 ? root.resultOldScores2 : root.resultOldScores1;
    }

    function resultBestScoreByPoints(scores) {
        let best = null;
        let bestRate = -1;
        for (let score of scores || []) {
            let result = score && score.result ? score.result : null;
            let maxPoints = result ? (result.maxPoints || 0) : 0;
            if (maxPoints <= 0) {
                continue;
            }
            let rate = (result.points || 0) / maxPoints;
            if (rate > bestRate) {
                best = score;
                bestRate = rate;
            }
        }
        return best;
    }

    function resultOldBestScore(side) {
        return root.resultBestScoreByPoints(root.resultOldScores(side));
    }

    function resultLastOldScore(side) {
        root.resultOldScoresRevision;
        let scores = root.resultOldScores(side) || [];
        return scores.length > 0 ? scores[0] : null;
    }

    function resultTargetSavedScore(side) {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        switch (vars ? vars.scoreTarget : ScoreTarget.BestScore) {
        case ScoreTarget.LastScore:
            return root.resultLastOldScore(side);
        case ScoreTarget.BestScore:
            return root.resultOldBestScore(side);
        default:
            return null;
        }
    }

    function resultTargetFraction(side) {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        return vars ? (vars.targetScoreFraction || 0) : 0;
    }

    function resultTargetPoints(side) {
        let targetScore = root.resultTargetSavedScore(side);
        if (targetScore && targetScore.result) {
            return root.resultExScore(targetScore.result);
        }
        let current = root.resultData(side);
        return current ? Math.floor((current.maxPoints || 0) * root.resultTargetFraction(side)) : 0;
    }

    function resultHighScorePoints(side) {
        return root.resultExScore(root.resultOldBestResult(side));
    }

    function resultTargetMaxPoints(side) {
        let current = root.resultData(side);
        return current ? Math.max(1, current.maxPoints || 1) : 1;
    }

    function resultOldBestResult(side) {
        let score = root.resultOldBestScore(side);
        return score && score.result ? score.result : null;
    }

    function resultUpdatedBestResult(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        if (!current) {
            return old;
        }
        if (!old || (current.maxPoints || 0) <= 0 || (old.maxPoints || 0) <= 0) {
            return current;
        }
        return current.points / current.maxPoints >= old.points / old.maxPoints ? current : old;
    }

    function resultScoreImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultExScore(current) > root.resultExScore(old));
    }

    function resultComboImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || (current.maxCombo || 0) > (old.maxCombo || 0));
    }

    function resultBadPoorImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultBadPoor(current) < root.resultBadPoor(old));
    }

    function updateResultOldScores() {
        if (!root.isResultScreen()) {
            return;
        }
        let request = ++root.resultOldScoresRequest;
        root.resultOldScores1 = [];
        root.resultOldScores2 = [];
        root.resultOldScoresRevision++;

        for (let side = 1; side <= 2; ++side) {
            let score = root.resultScore(side);
            let profile = root.resultProfile(side);
            let scoreDb = profile ? profile.scoreDb : null;
            if (!score || !score.result || !scoreDb) {
                continue;
            }
            let currentGuid = score.result.guid || "";
            if (root.course && root.course.identifier) {
                let courseId = root.course.identifier;
                scoreDb.getScoresForCourseId([courseId]).then(result => {
                    if (request !== root.resultOldScoresRequest) {
                        return;
                    }
                    let list = result && result.scores ? (result.scores[courseId] || []) : [];
                    let filtered = list.filter(oldScore => oldScore && oldScore.result && oldScore.result.guid !== currentGuid);
                    if (side === 2) {
                        root.resultOldScores2 = filtered;
                    } else {
                        root.resultOldScores1 = filtered;
                    }
                    root.resultOldScoresRevision++;
                });
            } else {
                let chartData = root.resultChartData();
                let md5 = chartData && chartData.md5 ? String(chartData.md5).toUpperCase() : "";
                if (md5.length === 0) {
                    continue;
                }
                scoreDb.getScoresForMd5([md5]).then(result => {
                    if (request !== root.resultOldScoresRequest) {
                        return;
                    }
                    let list = result && result.scores
                        ? (result.scores[md5] || result.scores[md5.toLowerCase()] || [])
                        : [];
                    let filtered = list.filter(oldScore => oldScore && oldScore.result && oldScore.result.guid !== currentGuid);
                    if (side === 2) {
                        root.resultOldScores2 = filtered;
                    } else {
                        root.resultOldScores1 = filtered;
                    }
                    root.resultOldScoresRevision++;
                });
            }
        }
    }

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
        if (!score || !score.gauges || score.gauges.length === 0) {
            return 0;
        }
        let gauges = score.gauges;
        for (let gauge of gauges) {
            if (gauge.gauge > gauge.threshold) {
                return gauge.gauge;
            }
        }
        return gauges[gauges.length - 1].gauge || 0;
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

    function gameplayGaugeOption(side) {
        let vars = root.generalVarsForSide(side);
        let gauge = String(vars ? vars.gaugeType : "").toUpperCase();
        if (gauge === "HARD" || gauge === "EXHARD" || gauge === "EXDAN" || gauge === "EXHARDDAN") {
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

    function appendGameplaySideOptions(options, side) {
        let score = root.gameplayScore(side);
        root.addOption(options, root.gameplayGaugeOption(side));
        root.addOption(options, root.gameplayLaneOption(score));
        root.addOption(options, root.gameplayLaneCoverOption(side));

        let currentRank = root.gameplayRankOption(score, side === 2 ? 210 : 200, true);
        if (currentRank >= 0) {
            root.addOption(options, currentRank);
        }
        if (side === 1) {
            let totalRank = root.gameplayRankOption(score, 220, false);
            if (totalRank >= 0) {
                root.addOption(options, totalRank);
            }
            root.addOption(options, root.gameplayExactRankOption(score, 300));
        } else {
            root.addOption(options, root.gameplayExactRankOption(score, 310));
        }
        root.addGameplayGaugeRangeOption(options, score, side === 2 ? 250 : 230);
        let judgementOption = root.gameplayJudgementOption(side, side === 2 ? 261 : 241);
        if (judgementOption >= 0) {
            root.addOption(options, judgementOption);
        }
        root.addOption(options, root.gameplayPoorBgaOption(side, side === 2 ? 267 : 247));
    }

    function appendGameplayRuntimeOptions(options) {
        root.gameplayRevision;
        let chartData = root.gameplayChartData();
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendChartOptions(options, chartData);
        root.appendGameplaySideOptions(options, 1);
        if (root.gameplayPlayer(2)) {
            root.appendGameplaySideOptions(options, 2);
        }
        if (root.gameplaySudChanging(1)) {
            root.addOption(options, 270);
        }
        if (root.gameplaySudChanging(2)) {
            root.addOption(options, 271);
        }

        root.addOption(options, 142); // autoscratch off.
        if (root.battleModeActive()) {
            root.addOption(options, 144);
        }
        if (root.spToDpActive()) {
            root.addOption(options, 145);
        }
        if (root.chart && root.chart.chartDatas && root.chart.currentChartIndex !== undefined) {
            let stage = Math.max(0, root.chart.currentChartIndex || 0);
            let count = root.chart.chartDatas.length || 0;
            root.addOption(options, 290);
            root.addOption(options, 293);
            if (count > 0 && stage === count - 1) {
                root.addOption(options, 289);
            } else if (count > 0) {
                root.addOption(options, 280 + Math.min(stage, 8));
            }
        }
    }

    function appendResultRuntimeOptions(options) {
        root.resultOldScoresRevision;
        let chartData = root.resultChartData();
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendChartOptions(options, chartData);
        root.addOption(options, root.resultClearOption());
        if (options.indexOf(351) === -1) {
            root.addOption(options, 350);
        }

        let current1 = root.resultData(1);
        let current2 = root.resultData(2);
        root.addOption(options, root.resultRankOptionForResult(current1, 300));
        if (current2) {
            root.addOption(options, root.resultRankOptionForResult(current2, 310));
        }
        root.addOption(options, root.resultRankOptionForResult(root.resultOldBestResult(1), 320));
        root.addOption(options, root.resultRankOptionForResult(root.resultUpdatedBestResult(1), 340));

        if (root.resultScoreImproved(1)) {
            root.addOption(options, 330);
            if (root.resultRawRank(current1) > root.resultRawRank(root.resultOldBestResult(1))) {
                root.addOption(options, 335);
            }
        }
        if (root.resultComboImproved(1)) {
            root.addOption(options, 331);
        }
        if (root.resultBadPoorImproved(1)) {
            root.addOption(options, 332);
        }

        if (current1 && current2) {
            let diff = root.resultExScore(current1) - root.resultExScore(current2);
            root.addOption(options, diff > 0 ? 352 : (diff < 0 ? 353 : 354));
        }
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
        if (!hit || !hit.noteRemoved) {
            return;
        }
        let lane = root.gameplayLr2LaneForHit(side, hit);
        if (lane < 0) {
            return;
        }

        let hitTimer = 50 + lane;
        let longNoteTimer = 70 + lane;
        let hitNote = root.gameplayNoteForHit(side, hit);

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

    function gameplayJudgeComboForHit(side, judgement) {
        return judgement >= Judgement.Good && judgement <= Judgement.Perfect
            ? root.gameplayCombo(side, false)
            : 0;
    }

    function updateGameplayHitTimers(side, hit) {
        let score = root.gameplayScore(side);
        if (!score) {
            return;
        }
        root.updateGameplayHitEffectTimers(side, hit);
        let judgement = root.gameplayJudgementFromHit(hit);

        let currentGauge = Math.floor(root.gameplayGaugeValue(score));
        let previousGaugeName = side === 2 ? "gameplayPreviousGauge2" : "gameplayPreviousGauge1";
        let previousGauge = root[previousGaugeName];
        if (previousGauge >= 0 && currentGauge > previousGauge) {
            if (side === 2) {
                root.gameplayGaugeUpSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(43, root.gameplayGaugeUpSkinTime2);
            } else {
                root.gameplayGaugeUpSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(42, root.gameplayGaugeUpSkinTime1);
            }
        } else if (previousGauge >= 0 && currentGauge < previousGauge) {
            if (side === 2) {
                root.gameplayGaugeDownSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(45, root.gameplayGaugeDownSkinTime2);
            } else {
                root.gameplayGaugeDownSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(44, root.gameplayGaugeDownSkinTime1);
            }
        }
        root[previousGaugeName] = currentGauge;

        if (judgement >= 0 && judgement <= Judgement.Perfect) {
            let displayCombo = root.gameplayJudgeComboForHit(side, judgement);
            if (side === 2) {
                root.gameplayJudgeSkinTime2 = root.renderSkinTime;
                root.setGameplayTimerValue(47, root.gameplayJudgeSkinTime2);
                root.gameplayLastJudgement2 = judgement;
                root.gameplayJudgeCombo2 = displayCombo;
                root.gameplayJudgeRevision2++;
            } else {
                root.gameplayJudgeSkinTime1 = root.renderSkinTime;
                root.setGameplayTimerValue(46, root.gameplayJudgeSkinTime1);
                root.gameplayLastJudgement1 = judgement;
                root.gameplayJudgeCombo1 = displayCombo;
                root.gameplayJudgeRevision1++;
            }
        }
        if (judgement >= 0 && judgement <= Judgement.Bad) {
            if (side === 2) {
                root.gameplayLastMissSkinTime2 = root.renderSkinTime;
            } else {
                root.gameplayLastMissSkinTime1 = root.renderSkinTime;
            }
            gameplayPoorBgaOptionTimer.restart();
        }

        let totalNotes = root.gameplayTotalNotes(score);
        let currentChartCombo = score ? (score.combo || 0) : 0;
        let previousComboName = side === 2 ? "gameplayPreviousCombo2" : "gameplayPreviousCombo1";
        let previousChartCombo = root[previousComboName] || 0;
        if (hit && hit.noteRemoved
                && totalNotes > 0
                && currentChartCombo >= totalNotes
                && previousChartCombo < totalNotes) {
            if (side === 2) {
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
        root.addGameplayTimer(result, 48, root.gameplayFullComboSkinTime1);
        root.addGameplayTimer(result, 49, root.gameplayFullComboSkinTime2);
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

    function gameplayColumnStateForKeyTimer(timer) {
        let lane = root.gameplayLr2LaneForKeyTimer(timer);
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

    function syncGameplayKeyTimersFromColumns() {
        if (!root.isGameplayScreen() || !root.chart) {
            return;
        }

        for (let timer of root.gameplayKeyTimers) {
            root.syncGameplayKeyTimerFromColumnState(timer, root.gameplayColumnStateForKeyTimer(timer));
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

    function appendCurrentSelectOptions(options, item, selectedChart) {
        root.appendSelectItemTypeOptions(options, item);
        root.appendSelectedChartModeOptions(options, selectedChart);
        root.appendEntryStatusOptions(options, item, selectedChart);
        root.appendCourseOptions(options, item);

        if (selectedChart) {
            root.appendDifficultyBarOptions(options);
        }

        root.appendChartOptions(options, selectedChart);
        for (let optionId of selectContext.scoreOptionIds(item)) {
            root.addOption(options, optionId);
        }
    }

    function appendDecideOptions(options) {
        let chartData = root.chart && root.chart.chartData
            ? root.chart.chartData
            : selectContext.selectedChartData();
        root.appendSelectItemTypeOptions(options, chartData);
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendEntryStatusOptions(options, chartData, chartData);
        root.appendChartOptions(options, chartData);
        for (let optionId of selectContext.scoreOptionIds(chartData)) {
            root.addOption(options, optionId);
        }
    }

    // Bar delegates get per-row state from the select context; keep their option set stable.
    readonly property var barActiveOptions: root.buildBarActiveOptions()
    readonly property var baseActiveOptions: root.buildBaseActiveOptions(root.barActiveOptions)
    readonly property var runtimeActiveOptions: root.isGameplayScreen()
        ? root.gameplayRuntimeActiveOptions
        : root.buildRuntimeActiveOptions(root.baseActiveOptions)
    readonly property var barTimers: ({ "0": 0 })

    function appendParserActiveOptions(result) {
        root.addOption(result, 0);
        let staticOptions = skinModel.effectiveActiveOptions && skinModel.effectiveActiveOptions.length
            ? skinModel.effectiveActiveOptions
            : root.parseActiveOptions;
        for (let option of staticOptions) {
            if (root.isGameplayScreen()
                    && (option === 32 || option === 33 || option === 80 || option === 81)) {
                continue;
            }
            root.addOption(result, option);
        }
    }

    function appendStaticSelectOptions(result) {
        root.appendParserActiveOptions(result);

        root.addOption(result, 20);  // no side panel active
        root.addOption(result, 46);  // difficulty filter enabled
        root.addOption(result, 52);  // non-extra mode
        root.addOption(result, 572); // not course-select mode
        root.addOption(result, 620); // internet ranking panel closed
        root.addOption(result, 622); // no ghost battle
        root.addOption(result, 624); // no rival score
    }

    function buildBarActiveOptions() {
        let result = [];
        root.appendStaticSelectOptions(result);
        return result;
    }

    function buildBaseActiveOptions(barOptions) {
        let result = root.effectiveScreenKey === "select"
            ? (barOptions || root.barActiveOptions).slice()
            : [];
        if (root.effectiveScreenKey !== "select") {
            root.appendParserActiveOptions(result);
        }

        if (root.effectiveScreenKey !== "select") {
            return result;
        }

        root.addOption(result, root.lr2RankingStatusOption());
        let rankingCount = root.lr2RankingPlayerCount();
        if (rankingCount === 0) {
            root.addOption(result, 610);
        }
        for (let threshold = 1; threshold <= 6; ++threshold) {
            if (rankingCount > threshold - 1) {
                root.addOption(result, 610 + threshold);
            }
        }
        root.addOption(result, selectContext.rankingMode ? 621 : 620);
        root.addOption(result, 622); // not ghost battle
        root.addOption(result, 624); // not rival compare
        root.appendPanelOptions(result);

        return result;
    }

    function buildRuntimeActiveOptions(baseOptions) {
        let result = baseOptions.slice();
        if (root.effectiveScreenKey === "select") {
            root.appendCommonRuntimeOptions(result);
            let item = selectContext.current;
            let selectedChart = selectContext.selectedChartData();
            root.appendCurrentSelectOptions(result, item, selectedChart);
        } else if (root.effectiveScreenKey === "decide") {
            root.appendCommonRuntimeOptions(result);
            root.appendDecideOptions(result);
        } else if (root.isGameplayScreen()) {
            root.appendCommonRuntimeOptions(result, true);
            root.appendGameplayRuntimeOptions(result);
        } else if (root.isResultScreen()) {
            root.appendCommonRuntimeOptions(result, true);
            root.appendResultRuntimeOptions(result);
        } else {
            root.appendChartOptions(result, root.chart && root.chart.chartData ? root.chart.chartData : null);
        }

        return result;
    }

    onBaseActiveOptionsChanged: root.scheduleGameplayRuntimeActiveOptionsRefresh()

    function refreshGameplayRuntimeActiveOptions() {
        if (!root.isGameplayScreen()) {
            return;
        }
        root.gameplayRuntimeActiveOptions = root.buildRuntimeActiveOptions(root.baseActiveOptions);
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

    readonly property int selectRevision: selectContext.selectionRevision + selectContext.scoreRevision
    property var deferredSelectChart: null
    property string activePreviewSource: ""
    property int pendingPreviewRevision: -1
    property int pendingPreviewRequest: 0
    property string pendingPreviewSource: ""
    property bool selectSideEffectsReady: false
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? root.deferredSelectChart
        : (root.chart || root.resultChartData())
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
        let item = root.effectiveScreenKey === "select" ? selectContext.current : null;
        if (!item || !selectContext.isCourse(item) || !item.loadCharts) {
            return [];
        }
        return item.loadCharts();
    }

    onSelectRevisionChanged: {
        root.handleCommittedSelectState();
    }
    Connections {
        target: selectContext
        function onSelectionRevisionChanged() {
            root.restartSelectInfoTimer();
            root.playSelectScratch();
        }
    }
    onEffectiveScreenKeyChanged: {
        if (root.effectiveScreenKey !== "select") {
            root.selectPanel = 0;
            root.selectPanelHeldByStart = 0;
            root.selectPanelElapsed = 0;
            root.selectPanelClosing = 0;
            root.selectPanelCloseElapsed = 0;
            root.lr2ReadmeMode = 0;
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
            root.updateGameplayHitTimers(1, hit);
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
            root.updateGameplayHitTimers(2, hit);
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
        root.commitLr2RankingRequest();
        root.applyRankingStatsToSelectContext();
        root.updateSelectSideEffects();
    }

    function handleScreenContextChanged() {
        root.commitLr2RankingRequest();
        root.restartSelectInfoTimer();
        root.updateSelectSideEffects();
    }

    function handleExternalChartChanged() {
        root.updateDisplayedSelectChart();
        root.commitLr2RankingRequest();
    }

    function updateDisplayedSelectChart() {
        if (root.effectiveScreenKey === "select") {
            root.deferredSelectChart = selectContext.selectedChartWrapper();
        } else {
            root.deferredSelectChart = root.chart;
        }
    }

    // Previews and ranking fetches are committed side effects; visual selection stays reactive.
    function updateSelectSideEffects() {
        if (!root.selectSideEffectsReady) {
            return;
        }

        if (selectContext.scrollFixedPointDragging) {
            root.updateDisplayedSelectChart();
            return;
        }

        if (!root.selectAudioActive) {
            root.stopSelectAudio();
            root.updateDisplayedSelectChart();
            return;
        }

        if (!root.deferredSelectChart || skinModel.reloadBanner) {
            root.updateDisplayedSelectChart();
        } else {
            selectChartSettleTimer.restart();
        }

        let nextPreviewSource = selectContext.selectedPreviewSource() || "";
        root.pendingPreviewRevision = root.selectRevision;
        if (nextPreviewSource === root.pendingPreviewSource) {
            if (nextPreviewSource !== ""
                    && root.activePreviewSource !== nextPreviewSource
                    && !previewDelayTimer.running) {
                previewDelayTimer.restart();
            }
            return;
        }

        previewDelayTimer.stop();
        root.pendingPreviewRequest += 1;
        root.pendingPreviewSource = nextPreviewSource;
        root.activePreviewSource = "";
        playMusic.stop();
        if (nextPreviewSource !== "") {
            previewDelayTimer.restart();
        }
    }
    readonly property bool acceptsInput: root.effectiveScreenKey !== "select"
        || root.globalSkinTime >= skinModel.startInput
    onAcceptsInputChanged: {
        if (root.effectiveScreenKey === "select" && root.acceptsInput) {
            root.selectNoScrollStartSkinTime = root.renderSkinTime;
        }
        if (root.acceptsInput && root.anyStartHeld && !root.startHoldSuppressed) {
            root.holdSelectPanel(1);
        }
    }

    function optionText(labels, index) {
        return index >= 0 && index < labels.length ? labels[index] : "";
    }

    function clearLabelForLamp(lamp) {
        switch (lamp) {
        case 1:
            return "FAILED";
        case 2:
            return "EASY";
        case 3:
            return "NORMAL";
        case 4:
            return "HARD";
        case 5:
            return "FULLCOMBO";
        default:
            return "NO PLAY";
        }
    }

    function courseStage(index) {
        return index >= 0 && index < root.selectedCourseStages.length
            ? root.selectedCourseStages[index]
            : null;
    }

    function chartTitle(chart) {
        if (typeof chart === "string") {
            return chart;
        }
        return chart ? (chart.title || "") : "";
    }

    function chartSubtitle(chart) {
        if (typeof chart === "string") {
            return "";
        }
        return chart ? (chart.subtitle || "") : "";
    }

    function lr2SelectOptionText(st) {
        switch (st) {
        case 63:
            return root.optionText(root.lr2RandomLabels, root.lr2RandomIndexP1);
        case 64:
            return root.optionText(root.lr2RandomLabels, root.lr2RandomIndexP2);
        case 65:
            return root.optionText(root.lr2GaugeLabels, root.lr2GaugeIndexP1);
        case 66:
            return root.optionText(root.lr2GaugeLabels, root.lr2GaugeIndexP2);
        case 67:
            return "OFF";
        case 68:
            return "OFF";
        case 69:
            return root.optionText(root.lr2BattleLabels, root.lr2BattleIndex);
        case 70:
            return root.optionText(["OFF", "FLIP"], root.lr2FlipIndex);
        case 71:
            return "OFF";
        case 72:
            return "OFF";
        case 73:
            return root.optionText(["OFF", "ON"], root.lr2LaneCoverIndex);
        case 74:
            return root.optionText(root.lr2HiSpeedFixLabels, root.lr2HiSpeedFixIndex);
        case 75:
            return "NORMAL";
        case 76:
            return root.optionText(["ON", "OFF"], root.lr2BgaIndex);
        case 77:
            return "32BIT";
        case 78:
            return "OFF";
        case 79:
            return globalRoot.isFullScreen() ? "FULLSCREEN" : "WINDOW";
        case 80:
            return "OFF";
        case 81:
            return root.optionText(root.lr2ReplayLabels, root.lr2ReplayType);
        case 82:
            return "MISSION COMPLETED";
        case 83:
            return "";
        case 84:
            return root.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP1);
        case 85:
            return root.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP2);
        case 120:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 121:
        case 122:
        case 123:
        case 124:
            return "";
        case 130:
            return root.clearLabelForLamp(selectContext.entryLamp(selectContext.current));
        case 131:
        case 132:
        case 133:
        case 134:
            return "";
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
            return root.chartTitle(root.courseStage(st - 150));
        case 160:
        case 161:
        case 162:
        case 163:
        case 164:
            return root.chartSubtitle(root.courseStage(st - 160));
        case 170:
            return root.effectiveScreenKey === "select" ? selectContext.entryDisplayName(selectContext.current, true) : "";
        case 171:
            return "TITLE";
        case 172:
            return "ARTIST";
        case 173:
            return "GENRE";
        case 174:
            return "TAG";
        case 183:
            return "";
        case 190:
            return "OFF";
        case 191:
        case 192:
        case 193:
        case 194:
        case 195:
            return "NO LIMIT";
        case 196:
            return "RANDOM";
        case 198:
        case 199:
            return "GROOVE";
        default:
            return "";
        }
    }

    function resolveText(st) {
        let revision = root.effectiveScreenKey === "select"
            ? selectContext.selectionRevision + selectContext.scoreRevision + selectContext.listRevision
            : (root.isResultScreen() ? root.resultOldScoresRevision : root.gameplayRevision);
        let chartData = root.displayChartData();
        let currentEntry = root.effectiveScreenKey === "select" ? selectContext.current : null;
        switch (st) {
        case 1:
            return root.optionText(root.lr2TargetLabels, root.lr2ScoreTargetIndex);
        case 2:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 10:
            if (chartData) {
                return (chartData.title || "").replace(/\r\n|\n|\r/g, " ");
            }
            if (root.effectiveScreenKey === "select") {
                return selectContext.entryMainTitle(currentEntry);
            }
            return root.course ? (root.course.name || "") : (root.chart && root.chart.course ? root.chart.course.name : "");
        case 11:
            return chartData ? (chartData.subtitle || "") : "";
        case 12:
            return chartData ? ((chartData.title || "") + (chartData.subtitle ? " " + chartData.subtitle : "")) : selectContext.entryDisplayName(currentEntry, true);
        case 13:
            return chartData ? (chartData.genre || "") : "Course";
        case 14:
            return chartData ? (chartData.artist || "") : "";
        case 15:
            return chartData ? (chartData.subartist || "") : "";
        case 17:
            return chartData ? String(chartData.playLevel || "") : "";
        case 18:
            return chartData ? String(selectContext.entryDifficulty(chartData) || "") : "";
        case 20:
            if (root.effectiveScreenKey === "select") {
                return selectContext.entryMainTitle(currentEntry);
            }
            return root.isResultScreen() ? "" : "";
        case 21:
            return root.effectiveScreenKey === "select" ? selectContext.entrySubtitle(currentEntry) : "";
        case 23:
            return root.effectiveScreenKey === "select" ? selectContext.entryGenre(currentEntry) : "";
        case 24:
            return root.effectiveScreenKey === "select" ? selectContext.entryArtist(currentEntry) : "";
        case 25:
            return root.effectiveScreenKey === "select" ? selectContext.entrySubartist(currentEntry) : "";
        case 26:
            return "";
        case 27:
            return chartData ? String(chartData.playLevel || "") : "";
        case 28:
            return chartData ? String(selectContext.entryDifficulty(chartData) || "") : "";
        case 29:
            return chartData ? String(chartData.rank || "") : "";
        case 30:
            if (root.effectiveScreenKey !== "select") {
                return "";
            }
            if (selectContext.searchText.length > 0) {
                return selectContext.searchText;
            }
            if (root.selectSearchHasFocus()) {
                return "";
            }
            let folderName = selectContext.currentFolderDisplayName();
            return folderName.length > 0 ? folderName : root.lr2SearchPlaceholderText;
        case 60:
            return root.effectiveScreenKey === "select" ? selectContext.keyFilterLabel() : "ALL";
        case 61:
            return root.effectiveScreenKey === "select" ? selectContext.sortLabel() : "DIRECTORY";
        case 62:
            return root.effectiveScreenKey === "select" ? selectContext.difficultyFilterLabel() : "ALL";
        default:
            return root.effectiveScreenKey === "select" ? root.lr2SelectOptionText(st) : "";
        }
    }

    function resolveGameplayNumber(num) {
        root.gameplayRevision;
        let chartData = root.gameplayChartData();
        let p1 = root.gameplayPlayer(1);
        let s1 = root.gameplayScore(1);
        let s2 = root.gameplayScore(2);

        switch (num) {
        case 10:
            return root.lr2HiSpeedP1;
        case 11:
            return root.lr2HiSpeedP2;
        case 12: {
            let vars = root.mainGeneralVars();
            return vars ? Math.round(vars.offset || 0) : 0;
        }
        case 13:
            return root.lr2TargetPercent;
        case 14:
            return root.laneCoverNumber(1);
        case 15:
            return root.laneCoverNumber(2);
        case 100:
            return root.gameplayDisplayedScorePrint(1);
        case 101:
            return root.gameplayExScore(s1);
        case 102:
            return root.gameplayRateInteger(s1, true);
        case 103:
            return root.gameplayRateDecimal(s1, true);
        case 104:
            return root.gameplayCombo(1, false);
        case 105:
            return root.gameplayCombo(1, true);
        case 106:
            return root.gameplayTotalNotes(s1);
        case 107:
            return Math.floor(root.gameplayGaugeValue(s1) / 2) * 2;
        case 108:
            return root.gameplayExScore(s1) - root.gameplayExScore(s2);
        case 109:
            return root.gameplayRankDelta(s1);
        case 110:
            return root.gameplayJudgementCount(s1, 5);
        case 111:
            return root.gameplayJudgementCount(s1, 4);
        case 112:
            return root.gameplayJudgementCount(s1, 3);
        case 113:
            return root.gameplayJudgementCount(s1, 2);
        case 114:
            return root.gameplayPoorCount(s1);
        case 115:
            return root.gameplayRateInteger(s1, false);
        case 116:
            return root.gameplayRateDecimal(s1, false);
        case 120:
            return root.gameplayDisplayedScorePrint(2);
        case 121:
            return root.gameplayExScore(s2);
        case 122:
            return root.gameplayRateInteger(s2, true);
        case 123:
            return root.gameplayRateDecimal(s2, true);
        case 124:
            return root.gameplayCombo(2, false);
        case 125:
            return root.gameplayCombo(2, true);
        case 126:
            return root.gameplayTotalNotes(s2);
        case 127:
            return Math.floor(root.gameplayGaugeValue(s2) / 2) * 2;
        case 128:
            return root.gameplayExScore(s2) - root.gameplayExScore(s1);
        case 129:
            return root.gameplayRankDelta(s2);
        case 130:
            return root.gameplayJudgementCount(s2, 5);
        case 131:
            return root.gameplayJudgementCount(s2, 4);
        case 132:
            return root.gameplayJudgementCount(s2, 3);
        case 133:
            return root.gameplayJudgementCount(s2, 2);
        case 134:
            return root.gameplayPoorCount(s2);
        case 135:
            return root.gameplayRateInteger(s2, false);
        case 136:
            return root.gameplayRateDecimal(s2, false);
        case 150:
            return root.gameplayHighScorePoints();
        case 151:
            return root.gameplayTargetScorePoints();
        case 152:
            return root.gameplayExScore(s1) - root.gameplayHighScorePoints();
        case 153:
            return root.gameplayExScore(s1) - root.gameplayTargetScorePoints();
        case 154:
            return root.gameplayRankDelta(s1);
        case 155:
            return root.gameplayScoreRateInteger(root.gameplayHighScorePoints(), s1);
        case 156:
            return root.gameplayScoreRateDecimal(root.gameplayHighScorePoints(), s1);
        case 157:
            return root.gameplayScoreRateInteger(root.gameplayTargetScorePoints(), s1);
        case 158:
            return root.gameplayScoreRateDecimal(root.gameplayTargetScorePoints(), s1);
        case 160:
            return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
        case 161:
            return Math.floor(root.gameplayTimeSeconds(1, false) / 60);
        case 162:
            return root.gameplayTimeSeconds(1, false) % 60;
        case 163:
            return Math.floor(root.gameplayTimeSeconds(1, true) / 60);
        case 164:
            return root.gameplayTimeSeconds(1, true) % 60;
        case 165:
            return root.chart && root.chart.status !== undefined ? 100 : 0;
        case 42:
        case 96:
            return chartData ? (chartData.playLevel || 0) : 0;
        case 90:
        case 290:
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        case 91:
        case 291:
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        default:
            return 0;
        }
    }

    function resultCompareResult() {
        return root.resultData(2) || root.resultOldBestResult(1);
    }

    function resolveResultSideNumber(num, result) {
        switch (num) {
        case 0:
            return root.resultScorePrint(result);
        case 1:
            return root.resultExScore(result);
        case 2:
            return root.resultRateInteger(result);
        case 3:
            return root.resultRateDecimal(result);
        case 5:
            return result ? (result.maxCombo || 0) : 0;
        case 6:
            return root.resultTotalNotes(result);
        case 10:
            return root.resultJudgementCount(result, Judgement.Perfect);
        case 11:
            return root.resultJudgementCount(result, Judgement.Great);
        case 12:
            return root.resultJudgementCount(result, Judgement.Good);
        case 13:
            return root.resultJudgementCount(result, Judgement.Bad);
        case 14:
            return root.resultPoorCount(result);
        default:
            return 0;
        }
    }

    function resolveResultTargetSideNumber(num, side) {
        let targetScore = root.resultTargetSavedScore(side);
        let targetResult = targetScore && targetScore.result ? targetScore.result : null;
        if (targetResult) {
            return root.resolveResultSideNumber(num, targetResult);
        }

        let points = root.resultTargetPoints(side);
        let maxPoints = root.resultTargetMaxPoints(side);
        let totalNotes = Math.floor(maxPoints / 2);
        switch (num) {
        case 0:
            return Math.floor(points * 100000 / maxPoints);
        case 1:
            return points;
        case 2:
            return Math.floor(points * 100 / maxPoints);
        case 3:
            return Math.floor(points * 10000 / maxPoints) % 100;
        case 6:
            return totalNotes;
        default:
            return 0;
        }
    }

    function resolveResultNumber(num) {
        root.resultOldScoresRevision;
        let current = root.resultData(1);
        let old = root.resultOldBestResult(1);

        if (num >= 100 && num <= 116) {
            return root.resolveResultSideNumber(num - 100, current);
        }
        if (num >= 120 && num <= 136) {
            return root.resolveResultTargetSideNumber(num - 120, 1);
        }

        switch (num) {
        case 42:
        case 96: {
            let chartData = root.resultChartData();
            return chartData ? (chartData.playLevel || 0) : 0;
        }
        case 90:
        case 290: {
            let chartData = root.resultChartData();
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        }
        case 91:
        case 291: {
            let chartData = root.resultChartData();
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        }
        case 150:
            return root.resultHighScorePoints(1);
        case 151:
            return root.resultTargetPoints(1);
        case 152:
            return root.resultExScore(current) - root.resultHighScorePoints(1);
        case 153:
            return root.resultExScore(current) - root.resultTargetPoints(1);
        case 154:
            return root.resultRankDelta(current);
        case 155:
            return root.resultScoreRateInteger(root.resultHighScorePoints(1), current);
        case 156:
            return root.resultScoreRateDecimal(root.resultHighScorePoints(1), current);
        case 157:
            return root.resultScoreRateInteger(root.resultTargetPoints(1), current);
        case 158:
            return root.resultScoreRateDecimal(root.resultTargetPoints(1), current);
        case 170:
            return root.resultExScore(old);
        case 171:
            return root.resultExScore(current);
        case 172:
            return root.resultExScore(current) - root.resultExScore(old);
        case 173:
            return old ? (old.maxCombo || 0) : 0;
        case 174:
            return current ? (current.maxCombo || 0) : 0;
        case 175:
            return (current ? (current.maxCombo || 0) : 0) - (old ? (old.maxCombo || 0) : 0);
        case 176:
            return root.resultBadPoor(old);
        case 177:
            return root.resultBadPoor(current);
        case 178:
            return root.resultBadPoor(current) - root.resultBadPoor(old);
        case 179:
        case 180:
        case 181:
        case 182:
            return 0;
        case 183:
            return root.resultRateInteger(old);
        case 184:
            return root.resultRateDecimal(old);
        default:
            return 0;
        }
    }

    function resolveNumber(num) {
        if (root.isResultScreen()) {
            return root.resolveResultNumber(num);
        }
        if (root.isGameplayScreen()) {
            return root.resolveGameplayNumber(num);
        }
        if (root.effectiveScreenKey === "select" || root.effectiveScreenKey === "decide") {
            switch (num) {
            case 10:
                return root.lr2HiSpeedP1;
            case 11:
                return root.lr2HiSpeedP2;
            case 12: {
                let vars = root.mainGeneralVars();
                return vars ? Math.round(vars.offset || 0) : 0;
            }
            case 13:
                return root.lr2TargetPercent;
            case 14:
                return root.laneCoverNumber(1);
            case 15:
                return root.laneCoverNumber(2);
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
                return root.dateTimeNumber(num);
            }
            if ((num >= 50 && num <= 66) || num === 8) {
                return root.lr2SliderNumber(num);
            }
            if (num >= 250 && num <= 259) {
                let stage = root.courseStage(num - 250);
                return selectContext.entryPlayLevel(stage);
            }
            if (num === 220) {
                return -1;
            }
            if ((num >= 300 && num <= 382)
                    || num === 271 || num === 272
                    || num === 274 || num === 275 || num === 276
                    || num === 292 || num === 293) {
                return 0;
            }
            let selectValue = selectContext.numberValue(num);
            if (root.effectiveScreenKey === "select" || selectValue !== 0) {
                return selectValue;
            }
            let chartData = root.chart && root.chart.chartData ? root.chart.chartData : null;
            switch (num) {
            case 42:
            case 96:
                return chartData ? (chartData.playLevel || 0) : 0;
            case 90:
            case 290:
                return chartData && (chartData.maxBpm || chartData.mainBpm)
                    ? Math.round(chartData.maxBpm || chartData.mainBpm)
                    : -1;
            case 91:
            case 291:
                return chartData && (chartData.minBpm || chartData.mainBpm)
                    ? Math.round(chartData.minBpm || chartData.mainBpm)
                    : -1;
            default:
                return 0;
            }
        }
        return 0;
    }

    function numberValue(src) {
        if (src && src.nowCombo) {
            return (src.side || (src.timer === 47 ? 2 : 1)) === 2
                ? root.gameplayJudgeCombo2
                : root.gameplayJudgeCombo1;
        }
        return root.resolveNumber(src ? src.num : 0);
    }

    function numberForceHidden(src) {
        if (!src || !src.nowCombo || !root.isGameplayScreen()) {
            return false;
        }
        let side = src.side || (src.timer === 47 ? 2 : 1);
        let judgement = side === 2 ? root.gameplayLastJudgement2 : root.gameplayLastJudgement1;
        let combo = side === 2 ? root.gameplayJudgeCombo2 : root.gameplayJudgeCombo1;
        return combo <= 0
            || (src.judgementIndex >= 0 && judgement !== src.judgementIndex);
    }

    function numberAnimationRevision(src) {
        if (!src || !src.nowCombo || !root.isGameplayScreen()) {
            return 0;
        }
        return (src.side || (src.timer === 47 ? 2 : 1)) === 2
            ? root.gameplayJudgeRevision2
            : root.gameplayJudgeRevision1;
    }

    function resolveBarGraph(type) {
        if (root.effectiveScreenKey === "select") {
            return selectContext.barGraphValue(type);
        }
        if (root.isGameplayScreen()) {
            return playContext.barGraphValue(type);
        }
        return 0;
    }

    function barGraphHasSourceAnimation(src) {
        return src
            && (src.cycle || 0) > 0
            && Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1) > 1;
    }

    function barGraphSourceSkinTime(src) {
        return root.effectiveScreenKey === "select" && root.barGraphHasSourceAnimation(src)
            ? root.selectSourceSkinTime
            : root.renderSkinTime;
    }

    function wrapValue(value, count) {
        return ((value % count) + count) % count;
    }

    function buttonUsesSplitArrows(buttonId) {
        switch (buttonId) {
        case 10:
        case 11:
        case 12:
        case 20:
        case 21:
        case 22:
        case 26:
        case 27:
        case 28:
        case 33:
        case 40:
        case 41:
        case 42:
        case 43:
        case 46:
        case 50:
        case 51:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 72:
        case 74:
        case 76:
        case 77:
        case 83:
            return true;
        default:
            return false;
        }
    }

    function buttonMouseDelta(src, mouseX, width) {
        if (src && src.buttonPlusOnly === 1) {
            return 1;
        }
        if (src && src.buttonPlusOnly === 2) {
            return -1;
        }
        if (src && root.buttonUsesSplitArrows(src.buttonId)) {
            return mouseX < width / 2 ? -1 : 1;
        }
        return 1;
    }

    function buttonFrame(src) {
        if (root.effectiveScreenKey !== "select" || !src || !src.button) {
            return -1;
        }
        switch (src.buttonId) {
        case 10:
            return selectContext.difficultyFilter;
        case 11:
            return Math.min(selectContext.keyFilter, 6);
        case 12:
            return selectContext.sortMode;
        case 40:
            return root.lr2GaugeIndexP1;
        case 41:
            return root.lr2GaugeIndexP2;
        case 42:
            return root.lr2RandomIndexP1;
        case 43:
            return root.lr2RandomIndexP2;
        case 44:
            return 0;
        case 45:
            return 0;
        case 20:
        case 21:
        case 22:
            return root.lr2FxType[src.buttonId - 20] || 0;
        case 23:
        case 24:
        case 25:
            return root.lr2FxOn[src.buttonId - 23] ? 1 : 0;
        case 26:
        case 27:
        case 28:
            return root.lr2FxTarget[src.buttonId - 26] || 0;
        case 29:
            return root.lr2EqOn ? 1 : 0;
        case 32:
            return root.lr2PitchOn ? 1 : 0;
        case 33:
            return root.lr2PitchType;
        case 46:
            return root.lr2LaneCoverIndex;
        case 50:
            return root.lr2HidSudIndexP1;
        case 51:
            return root.lr2HidSudIndexP2;
        case 54:
            return root.lr2FlipIndex;
        case 55:
            return root.lr2HiSpeedFixIndex;
        case 56:
            return root.lr2BattleIndex;
        case 70:
            return 0;
        case 71:
            return 0;
        case 72:
            return root.lr2BgaIndex;
        case 73:
            return 0;
        case 75:
            return 0;
        case 80:
            return 0;
        case 81:
            return 0;
        case 82:
            return 0;
        case 83:
            return 0;
        case 210:
            return 0;
        default:
            if ((src.buttonId >= 13 && src.buttonId <= 14)
                    || src.buttonId === 18
                    || (src.buttonId >= 230 && src.buttonId <= 268)) {
                return 0;
            }
            return -1;
        }
    }

    function closeSelectPanel() {
        if (root.selectPanel > 0) {
            root.playOneShot(optionCloseSound);
            root.startSelectPanelCloseTimer(root.selectPanel);
        }
        root.selectPanel = 0;
        root.selectPanelHeldByStart = 0;
        root.selectPanelElapsed = 0;
        root.selectHeldButtonTimerStarts = ({});
    }

    function startSelectPanelCloseTimer(panel) {
        if (panel <= 0) {
            return;
        }
        root.selectPanelClosing = panel;
        root.selectPanelCloseStartMs = Date.now();
        root.selectPanelCloseElapsed = 0;
    }

    function openSelectPanel(panel, heldByStart) {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel !== panel) {
            root.playOneShot(optionOpenSound);
            if (root.selectPanel > 0) {
                root.startSelectPanelCloseTimer(root.selectPanel);
            }
            root.selectPanel = panel;
            root.selectPanelStartMs = Date.now();
            root.selectPanelElapsed = 0;
        }
        root.selectPanelHeldByStart = heldByStart ? panel : 0;
    }

    function toggleSelectPanel(panel) {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel === panel) {
            root.closeSelectPanel();
            return;
        }
        root.openSelectPanel(panel, false);
    }

    function holdSelectPanel(panel) {
        if (!root.selectInputReady()) {
            return;
        }
        if (root.selectPanel === panel && root.selectPanelHeldByStart !== panel) {
            return;
        }
        root.openSelectPanel(panel, true);
    }

    function releaseHeldSelectPanel(panel) {
        if (root.selectPanelHeldByStart === panel) {
            root.closeSelectPanel();
        }
    }

    function currentSelectHeldButtonSkinTime() {
        if (root.effectiveScreenKey !== "select") {
            return root.renderSkinTime;
        }
        return Math.max(root.renderSkinTime, Date.now() - root.sceneStartMs);
    }

    function selectHeldButtonTimerForKey(key) {
        switch (key) {
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

    function isSelectHeldButtonTimer(timer) {
        return (timer >= 101 && timer <= 107) || (timer >= 111 && timer <= 117);
    }

    function pressSelectHeldButtonTimer(key) {
        if (root.effectiveScreenKey !== "select" || root.selectPanel !== 1) {
            return;
        }
        let timer = root.selectHeldButtonTimerForKey(key);
        if (!timer) {
            return;
        }
        let starts = root.selectHeldButtonTimerStarts;
        if (starts[timer] !== undefined) {
            return;
        }
        let copy = {};
        for (let keyName in starts) {
            copy[keyName] = starts[keyName];
        }
        root.selectHeldButtonSkinTime = root.currentSelectHeldButtonSkinTime();
        copy[timer] = root.selectHeldButtonSkinTime;
        root.selectHeldButtonTimerStarts = copy;
    }

    function releaseSelectHeldButtonTimer(key) {
        let timer = root.selectHeldButtonTimerForKey(key);
        if (!timer || root.selectHeldButtonTimerStarts[timer] === undefined) {
            return;
        }
        let copy = {};
        for (let keyName in root.selectHeldButtonTimerStarts) {
            if (Number(keyName) !== timer) {
                copy[keyName] = root.selectHeldButtonTimerStarts[keyName];
            }
        }
        root.selectHeldButtonTimerStarts = copy;
    }

    function addHeldButtonTimer(result, timer, held) {
        if (!held) {
            return;
        }
        let start = root.selectHeldButtonTimerStarts[timer];
        result[timer] = start === undefined ? root.currentSelectHeldButtonSkinTime() : start;
    }

    function addHeldButtonTimers(result) {
        if (root.effectiveScreenKey !== "select" || root.selectPanel !== 1) {
            return;
        }
        root.addHeldButtonTimer(result, 101, Input.col11);
        root.addHeldButtonTimer(result, 102, Input.col12);
        root.addHeldButtonTimer(result, 103, Input.col13);
        root.addHeldButtonTimer(result, 104, Input.col14);
        root.addHeldButtonTimer(result, 105, Input.col15);
        root.addHeldButtonTimer(result, 106, Input.col16);
        root.addHeldButtonTimer(result, 107, Input.col17);
        root.addHeldButtonTimer(result, 111, Input.col21);
        root.addHeldButtonTimer(result, 112, Input.col22);
        root.addHeldButtonTimer(result, 113, Input.col23);
        root.addHeldButtonTimer(result, 114, Input.col24);
        root.addHeldButtonTimer(result, 115, Input.col25);
        root.addHeldButtonTimer(result, 116, Input.col26);
        root.addHeldButtonTimer(result, 117, Input.col27);
    }

    function spriteSkinTime(dsts) {
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        return root.isSelectHeldButtonTimer(timer)
            ? (root.hasSelectHeldButtonTimers ? root.selectHeldButtonSkinTime : root.currentSelectHeldButtonSkinTime())
            : root.renderSkinTime;
    }

    function buttonPanelMatches(src) {
        if (!src || !src.button) {
            return false;
        }
        return root.panelMatches(src.buttonPanel || 0);
    }

    function handleLr2Button(buttonId, delta, panel) {
        if (!root.selectInputReady()) {
            return;
        }
        root.resetSelectSearch();
        if (buttonId >= 1 && buttonId <= 9) {
            root.toggleSelectPanel(buttonId);
            return;
        }
        if (buttonId === 17) {
            let path = selectContext.attachedTextFile(selectContext.selectedChartData());
            if (path) {
                root.openReadmePath(path);
            }
            return;
        }
        if (buttonId === 200 && root.lr2ReadmeMode > 0) {
            root.closeReadme();
            return;
        }
        if (buttonId === 200 && panel > 0) {
            root.closeSelectPanel();
            return;
        }
        if (buttonId >= 201 && buttonId <= 206 && panel > 0) {
            let helpFiles = skinModel.helpFiles || [];
            let helpIndex = buttonId - 200;
            if (helpIndex >= 0 && helpIndex < helpFiles.length) {
                root.openReadmePath(helpFiles[helpIndex]);
            }
            return;
        }
        if (buttonId === 210) {
            root.openLr2InternetRanking();
            return;
        }
        let optionChanged = false;
        switch (buttonId) {
        case 10:
            selectContext.difficultyFilter = root.wrapValue(selectContext.difficultyFilter + delta, 6);
            selectContext.sortOrFilterChanged();
            optionChanged = true;
            break;
        case 11:
            selectContext.keyFilter = root.wrapValue(selectContext.keyFilter + delta, 7);
            selectContext.sortOrFilterChanged();
            optionChanged = true;
            break;
        case 12:
            selectContext.sortMode = root.wrapValue(selectContext.sortMode + delta, 5);
            selectContext.sortOrFilterChanged();
            optionChanged = true;
            break;
        case 15:
            root.selectGoForward();
            break;
        case 16:
            root.selectGoForward(undefined, true);
            break;
        case 19: {
            let replayScore = selectContext.replayScoreForType(selectContext.current, root.lr2ReplayType);
            if (replayScore) {
                root.selectGoForward(selectContext.current, false, true, replayScore);
            }
            break;
        }
        case 57:
            root.setHiSpeedNumber(1, root.lr2HiSpeedP1 + delta * 10);
            optionChanged = true;
            break;
        case 58:
            root.setHiSpeedNumber(2, root.lr2HiSpeedP2 + delta * 10);
            optionChanged = true;
            break;
        case 40:
            root.setGaugeIndex(1, root.lr2GaugeIndexP1 + delta);
            optionChanged = true;
            break;
        case 41:
            root.setGaugeIndex(2, root.lr2GaugeIndexP2 + delta);
            optionChanged = true;
            break;
        case 42:
            root.setRandomIndex(1, root.lr2RandomIndexP1 + delta);
            optionChanged = true;
            break;
        case 43:
            root.setRandomIndex(2, root.lr2RandomIndexP2 + delta);
            optionChanged = true;
            break;
        case 44:
        case 45:
            // Assist options are intentionally unsupported; keep LR2 menus at OFF.
            break;
        case 18:
            optionChanged = true;
            break;
        case 20:
        case 21:
        case 22: {
            let slot = buttonId - 20;
            root.lr2FxType = root.setArrayValue(root.lr2FxType, slot, root.wrapValue((root.lr2FxType[slot] || 0) + delta, 8));
            optionChanged = true;
            break;
        }
        case 23:
        case 24:
        case 25: {
            let slot = buttonId - 23;
            root.lr2FxOn = root.setArrayValue(root.lr2FxOn, slot, !root.lr2FxOn[slot]);
            optionChanged = true;
            break;
        }
        case 26:
        case 27:
        case 28: {
            let slot = buttonId - 26;
            root.lr2FxTarget = root.setArrayValue(root.lr2FxTarget, slot, root.wrapValue((root.lr2FxTarget[slot] || 0) + delta, 3));
            optionChanged = true;
            break;
        }
        case 29:
            root.lr2EqOn = !root.lr2EqOn;
            optionChanged = true;
            break;
        case 32:
            root.lr2PitchOn = !root.lr2PitchOn;
            optionChanged = true;
            break;
        case 33:
            root.lr2PitchType = root.wrapValue(root.lr2PitchType + delta, 3);
            optionChanged = true;
            break;
        case 46:
            root.setLaneCoverIndex(root.lr2LaneCoverIndex + delta);
            optionChanged = true;
            break;
        case 50:
            root.setHidSudIndex(1, root.lr2HidSudIndexP1 + delta);
            optionChanged = true;
            break;
        case 51:
            root.setHidSudIndex(2, root.lr2HidSudIndexP2 + delta);
            optionChanged = true;
            break;
        case 54:
            root.setFlipIndex(root.lr2FlipIndex + delta);
            optionChanged = true;
            break;
        case 55:
            root.setHiSpeedFixIndex(root.lr2HiSpeedFixIndex + delta);
            optionChanged = true;
            break;
        case 56:
            root.setBattleIndex(root.lr2BattleIndex + delta);
            optionChanged = true;
            break;
        case 70:
        case 71:
            // Score graph and ghost options are not backed yet.
            break;
        case 72:
            root.setBgaIndex(root.lr2BgaIndex + delta);
            optionChanged = true;
            break;
        case 73:
        case 75:
            // BGA size and judge auto-adjust are not selectable from LR2 skins yet.
            break;
        case 74:
            root.adjustOffset(delta);
            optionChanged = true;
            break;
        case 76:
            root.setTargetPercent(root.lr2TargetPercent + delta);
            optionChanged = true;
            break;
        case 77:
            root.setScoreTargetIndex(root.lr2ScoreTargetIndex + delta);
            optionChanged = true;
            break;
        case 80:
            globalRoot.toggleFullScreen();
            break;
        case 81:
        case 82:
            // Color depth and vsync settings are not mutable from the LR2 skin.
            break;
        case 83:
            root.lr2ReplayType = root.wrapValue(root.lr2ReplayType + delta, root.lr2ReplayLabels.length);
            optionChanged = true;
            break;
        default:
            if (buttonId >= 91 && buttonId <= 96) {
                selectContext.clickDifficulty(buttonId - 91);
                optionChanged = true;
                break;
            }
            if ((buttonId >= 13 && buttonId <= 14)
                    || (buttonId >= 230 && buttonId <= 268)) {
                // These LR2 controls cover skin/key config, IR, and tag-editor
                // actions that do not have safe backing state here.
                break;
            }
            console.info("LR2 select button " + buttonId + " is not implemented yet");
            break;
        }
        if (optionChanged) {
            root.playOneShot(optionChangeSound);
        }
    }

    function triggerSelectPanelButton(buttonId, delta) {
        root.handleLr2Button(buttonId, delta === undefined ? 1 : delta, root.selectPanel);
        return true;
    }

    function triggerPanelButtonForKey(p1ButtonId, p2ButtonId, key, delta) {
        let buttonId = p1ButtonId;
        if (p2ButtonId !== undefined) {
            switch (key) {
            case BmsKey.Col21:
            case BmsKey.Col22:
            case BmsKey.Col23:
            case BmsKey.Col24:
            case BmsKey.Col25:
            case BmsKey.Col26:
            case BmsKey.Col27:
            case BmsKey.Col2sUp:
            case BmsKey.Col2sDown:
            case BmsKey.Select2:
                buttonId = p2ButtonId;
                break;
            default:
                break;
            }
        }
        return root.triggerSelectPanelButton(buttonId, delta === undefined ? 1 : delta);
    }

    function handleSelectPanelKey(key) {
        if (!root.selectInputReady() || root.selectPanel <= 0) {
            return false;
        }

        if (root.selectPanel === 1) {
            switch (key) {
            case BmsKey.Col11:
            case BmsKey.Col21:
                return root.triggerPanelButtonForKey(11, 11, key);
            case BmsKey.Col12:
            case BmsKey.Col22:
                return root.triggerPanelButtonForKey(42, 43, key);
            case BmsKey.Col13:
            case BmsKey.Col23:
                return root.triggerPanelButtonForKey(56, 56, key);
            case BmsKey.Col14:
            case BmsKey.Col24:
                return root.triggerPanelButtonForKey(40, 41, key);
            case BmsKey.Col15:
            case BmsKey.Col25:
                return root.triggerPanelButtonForKey(54, 54, key);
            case BmsKey.Col16:
            case BmsKey.Col26:
                return root.triggerPanelButtonForKey(44, 45, key);
            case BmsKey.Col17:
            case BmsKey.Col27:
                return root.triggerPanelButtonForKey(50, 51, key);
            case BmsKey.Select1:
            case BmsKey.Select2:
                return root.triggerPanelButtonForKey(77, 77, key);
            default:
                return false;
            }
        }

        if (root.selectPanel === 2) {
            switch (key) {
            case BmsKey.Col11:
            case BmsKey.Col21:
                return root.triggerSelectPanelButton(71);
            case BmsKey.Col12:
            case BmsKey.Col22:
                return root.triggerSelectPanelButton(70);
            case BmsKey.Col13:
            case BmsKey.Col23:
                return root.triggerSelectPanelButton(76);
            case BmsKey.Col14:
            case BmsKey.Col24:
                return root.triggerSelectPanelButton(72);
            case BmsKey.Col15:
            case BmsKey.Col25:
                return root.triggerSelectPanelButton(73);
            case BmsKey.Col16:
            case BmsKey.Col26:
                return root.triggerSelectPanelButton(74);
            case BmsKey.Col17:
            case BmsKey.Col27:
                return root.triggerSelectPanelButton(75);
            case BmsKey.Select1:
            case BmsKey.Select2:
                return root.triggerSelectPanelButton(83);
            default:
                return false;
            }
        }

        return false;
    }

    property real selectWheelRemainder: 0

    function handleSelectWheel(wheel) {
        if (root.lr2ReadmeMode === 1) {
            let readmeDelta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
            if (readmeDelta !== 0) {
                root.scrollReadmeBy(0, readmeDelta / 120.0 * Math.max(1, root.lr2ReadmeLineSpacing));
            }
            wheel.accepted = true;
            return;
        }
        if (!root.selectScrollReady()) {
            return;
        }
        let delta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
        if (delta === 0) {
            return;
        }
        root.selectWheelRemainder += delta / 120.0;
        let steps = root.selectWheelRemainder > 0
            ? Math.floor(root.selectWheelRemainder)
            : Math.ceil(root.selectWheelRemainder);
        if (steps !== 0) {
            root.selectWheelRemainder -= steps;
            selectContext.queueWheelSteps(steps);
        }
        wheel.accepted = true;
    }

    function onMouseSpriteState(src, dsts) {
        if (!src || !src.onMouse || !root.panelMatches(src.hoverPanel || 0)) {
            return null;
        }
        let state = Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
        if (!state) {
            return null;
        }
        // LR2 tests mouse hitboxes in integer 640x480 skin pixels. Without
        // snapping the scaled QML position back to that grid, adjacent
        // ONMOUSE regions can leave a tiny dead strip between arrow halves.
        let mx = Math.floor(root.selectMouseX / skinScale);
        let my = Math.floor(root.selectMouseY / skinScale);
        let hoverX = state.x + (src.hoverX || 0);
        let hoverY = state.y + (src.hoverY || 0);
        let hoverW = src.hoverW || state.w;
        let hoverH = src.hoverH || state.h;
        return mx >= hoverX && mx <= hoverX + hoverW && my >= hoverY && my <= hoverY + hoverH
            ? state
            : null;
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
        let base = Lr2Timeline.getCurrentState(
            dsts,
            root.renderSkinTime,
            root.elementUsesTimers(src, dsts) ? root.timers : root.zeroTimers,
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

    function spriteStateOverride(src, dsts) {
        if (root.isNowJudgeSprite(src)) {
            return root.nowJudgeState(src, dsts);
        }
        if (root.isSelectScrollSlider(src)) {
            return root.selectScrollSliderState(src, dsts);
        }
        if (root.isGameplayProgressSlider(src)) {
            return root.gameplayProgressSliderState(src, dsts);
        }
        if (root.isLr2GenericSlider(src)) {
            return root.lr2GenericSliderState(src, dsts);
        }
        return null;
    }

    function spriteForceHidden(src, dsts) {
        if (src && src.onMouse) {
            return root.onMouseSpriteState(src, dsts) === null;
        }
        return false;
    }

    function translatedSliderState(src, dsts, position) {
        let base = Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
        if (!base) {
            return null;
        }

        let sliderOffset = position * Math.max(1, src.sliderRange || 0);
        let offsetX = 0;
        let offsetY = 0;
        switch (src.sliderDirection) {
        case 0:
            offsetY = -sliderOffset;
            break;
        case 1:
            offsetX = sliderOffset;
            break;
        case 2:
            offsetY = sliderOffset;
            break;
        case 3:
            offsetX = -sliderOffset;
            break;
        default:
            return null;
        }

        return {
            x: base.x + offsetX,
            y: base.y + offsetY,
            w: base.w,
            h: base.h,
            a: base.a,
            r: base.r,
            g: base.g,
            b: base.b,
            angle: base.angle || 0,
            center: base.center || 0,
            blend: base.blend || 0,
            filter: base.filter || 0,
            op1: base.op1 || 0,
            op2: base.op2 || 0,
            op3: base.op3 || 0,
            op4: base.op4 || 0
        };
    }

    function selectScrollSliderState(src, dsts) {
        if (!root.isSelectScrollSlider(src)) {
            return null;
        }

        let logicalCount = Math.max(1, selectContext.logicalCount);
        let maxFixed = Math.max(1, logicalCount * 1000 - 1);
        let fixedValue = Math.max(0, Math.min(maxFixed, selectContext.normalizedVisualIndex * 1000));
        let position = logicalCount > 1
            ? fixedValue / maxFixed
            : 0;
        return root.translatedSliderState(src, dsts, position);
    }

    function lr2GenericSliderState(src, dsts) {
        if (!root.isLr2GenericSlider(src)) {
            return null;
        }
        return root.translatedSliderState(src, dsts, root.sliderRawValue(src.sliderType) / 100);
    }

    function gameplayProgressSliderState(src, dsts) {
        if (!root.isGameplayProgressSlider(src)) {
            return null;
        }
        root.globalSkinTime;
        let player = root.gameplayPlayer(1);
        let elapsed = player ? Math.max(0, player.elapsed || 0) : 0;
        let length = player ? Math.max(0, player.chartLength || 0) : 0;
        let position = length > 0 ? Math.max(0, Math.min(1, elapsed / length)) : 0;
        return root.translatedSliderState(src, dsts, position);
    }

    function selectScrollSliderTrackState(src, dsts) {
        if (!root.isSelectScrollSlider(src)) {
            return null;
        }
        return root.sliderTrackState(src, dsts);
    }

    function lr2GenericSliderTrackState(src, dsts) {
        if (!root.isLr2GenericSlider(src)) {
            return null;
        }
        return root.sliderTrackState(src, dsts);
    }

    function setSelectScrollFromSliderPointer(src, dsts, pointerX, pointerY) {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0) {
            return;
        }
        let track = root.selectScrollSliderTrackState(src, dsts);
        root.setSelectScrollFromSliderTrack(src, track, pointerX, pointerY);
    }

    function setSelectScrollFromSliderTrack(src, track, pointerX, pointerY) {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0) {
            return;
        }
        if (!track) {
            return;
        }

        let position = root.sliderPositionFromPointer(src, track, pointerX, pointerY);
        let maxFixed = Math.max(0, selectContext.logicalCount * 1000 - 1);
        let fixedPoint = Math.max(0, Math.min(maxFixed, Math.round(position * maxFixed)));
        if (fixedPoint === root.selectSliderFixedPoint) {
            return;
        }
        root.selectSliderFixedPoint = fixedPoint;
        selectContext.dragScrollFixedPoint(fixedPoint);
    }

    function setLr2GenericSliderFromPointer(src, dsts, pointerX, pointerY) {
        let track = root.lr2GenericSliderTrackState(src, dsts);
        root.setLr2GenericSliderFromTrack(src, track, pointerX, pointerY);
    }

    function setLr2GenericSliderFromTrack(src, track, pointerX, pointerY) {
        if (!track) {
            return;
        }
        root.setSliderRawValue(src.sliderType, root.sliderPositionFromPointer(src, track, pointerX, pointerY) * 100);
    }

    function computeBarBaseState(row, selectedRow) {
        let rows = skinModel.barRows;
        if (!rows || row < 0 || row >= rows.length) {
            return null;
        }
        let data = rows[row];
        let useOn = row === selectedRow && data.onDsts && data.onDsts.length > 0;
        let dstList = useOn ? data.onDsts : data.offDsts;
        if (!dstList || dstList.length === 0) {
            dstList = data.onDsts || [];
        }
        return Lr2Timeline.getCurrentState(dstList, root.barSkinTime, root.barTimers, root.barActiveOptions);
    }

    function interpolateBarState(fromState, toState, progress) {
        if (!fromState || !toState) {
            return fromState;
        }
        let inv = 1.0 - progress;
        return {
            x: fromState.x * inv + toState.x * progress,
            y: fromState.y * inv + toState.y * progress,
            w: fromState.w * inv + toState.w * progress,
            h: fromState.h * inv + toState.h * progress,
            a: fromState.a * inv + toState.a * progress,
            r: fromState.r * inv + toState.r * progress,
            g: fromState.g * inv + toState.g * progress,
            b: fromState.b * inv + toState.b * progress,
            blend: fromState.blend,
            filter: fromState.filter,
            angle: fromState.angle * inv + toState.angle * progress,
            center: fromState.center,
            sortId: (fromState.sortId || 0) * inv + (toState.sortId || 0) * progress
        };
    }

    readonly property var cachedBarBaseStates: {
        let rows = skinModel.barRows || [];
        let selected = root.selectedBarRow();
        let result = [];
        for (let row = 0; row < rows.length; ++row) {
            result.push(root.computeBarBaseState(row, selected));
        }
        return result;
    }

    function barBaseState(row) {
        return root.cachedBarBaseStates && row >= 0 && row < root.cachedBarBaseStates.length
            ? root.cachedBarBaseStates[row]
            : null;
    }

    function barDrawState(row) {
        let fromState = root.barBaseState(row);
        let offset = selectContext.scrollOffset;
        if (!fromState || offset <= 0.001) {
            return fromState;
        }

        let toState = row > 0 ? root.barBaseState(row - 1) : null;
        return fromState && toState
            ? root.interpolateBarState(fromState, toState, offset)
            : fromState;
    }

    function selectedBarRow() {
        return skinModel.barCenter + selectContext.selectedOffset;
    }

    function barClickStart() {
        return Math.max(0, skinModel.barAvailableStart - 3);
    }

    function barClickEnd() {
        let rows = skinModel.barRows ? skinModel.barRows.length : 0;
        return Math.min(rows - 1, skinModel.barAvailableEnd + 3);
    }

    function barRowCanClick(row) {
        if (skinModel.barAvailableEnd < skinModel.barAvailableStart) {
            return false;
        }
        return row >= root.barClickStart() && row <= root.barClickEnd();
    }

    function barRowScrollDelta(row) {
        let first = skinModel.barAvailableStart;
        let last = skinModel.barAvailableEnd;
        if (row < first) {
            return -2 * (first - row);
        }
        if (row > last) {
            return 2 * (row - last);
        }
        return 0;
    }

    function handleBarRowClick(row, mouse) {
        root.resetSelectSearch();
        if (mouse.button === Qt.RightButton) {
            root.selectGoBack();
            return;
        }

        let delta = root.barRowScrollDelta(row);
        if (delta !== 0) {
            if (!selectContext.visualMoveActive) {
                selectContext.scrollBy(delta, selectContext.lr2ClickDuration);
            }
            return;
        }

        if (row === root.selectedBarRow()) {
            root.selectGoForward();
            return;
        }

        selectContext.selectVisibleRow(row, skinModel.barCenter);
    }

    // Monotonic scene clock. Reset after the parsed model is ready so skin
    // loading does not eat the first second or two of LR2's decide animation.
    property double sceneStartMs: Date.now()
    property int globalSkinTime: 0
    property double selectInfoStartMs: Date.now()
    property int selectInfoElapsed: 0
    property int selectScrollStartSkinTime: 0
    property int selectNoScrollStartSkinTime: 0
    property int selectDatabaseLoadedSkinTime: 0
    property int selectSourceSkinTime: 0
    property int selectAnimationLimit: 3200
    property int barAnimationLimit: 2200
    readonly property int selectInfoAnimationLimit: 1000
    readonly property int renderSkinTime: root.effectiveScreenKey === "select"
        ? Math.min(root.globalSkinTime, root.selectAnimationLimit)
        : root.globalSkinTime
    readonly property int barSkinTime: root.effectiveScreenKey === "select"
        ? Math.min(root.globalSkinTime, root.barAnimationLimit)
        : root.renderSkinTime
    readonly property bool shouldAutoAdvance: root.effectiveScreenKey === "decide"
        && !!root.chart
        && skinModel.sceneTime > 0

    function updateSelectAnimationLimits() {
        let startInput = skinModel.startInput || 0;
        root.selectAnimationLimit = Math.max(3200, startInput);
        root.barAnimationLimit = Math.max(2200, startInput);
    }

    function restartSkinClock() {
        root.updateSelectAnimationLimits();
        root.sceneStartMs = Date.now();
        root.globalSkinTime = 0;
        root.selectSourceSkinTime = 0;
        root.selectScrollStartSkinTime = 0;
        root.selectNoScrollStartSkinTime = 0;
        root.selectDatabaseLoadedSkinTime = 0;
        root.selectHeldButtonSkinTime = 0;
        root.selectHeldButtonTimerStarts = ({});
        root.selectPanelClosing = 0;
        root.selectPanelCloseElapsed = 0;
        root.selectScratchSoundReady = false;
        root.restartSelectInfoTimer();
        if (root.shouldAutoAdvance) {
            sceneEndTimer.restart();
        } else {
            sceneEndTimer.stop();
        }
    }

    function restartSelectInfoTimer() {
        root.selectInfoStartMs = Date.now();
        root.selectInfoElapsed = 0;
        if (root.effectiveScreenKey === "select") {
            selectInfoStopwatch.restart();
        }
    }

    Timer {
        id: skinStopwatch
        interval: 16
        running: root.effectiveScreenKey !== "select" || root.globalSkinTime < root.selectAnimationLimit
        repeat: true
        onTriggered: {
            root.globalSkinTime = Date.now() - root.sceneStartMs;
            if (root.isGameplayScreen()
                    && root.chart
                    && (root.gameplayReadySkinTime < 0 || root.gameplayStartSkinTime < 0)) {
                root.updateGameplayStatusTimers();
                root.startGameplayWhenReady();
            }
        }
    }

    Timer {
        id: selectSourceSkinStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select"
        repeat: true
        onTriggered: {
            let now = Date.now();
            root.selectSourceSkinTime = now - root.sceneStartMs;
            selectContext.updateVisualIndex(now);
        }
    }

    Timer {
        id: selectPanelStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select" && root.selectPanel > 0 && root.selectPanelElapsed < root.selectPanelHoldTime
        repeat: true
        onTriggered: {
            root.selectPanelElapsed = Math.min(root.selectPanelHoldTime, Date.now() - root.selectPanelStartMs);
        }
    }

    Timer {
        id: selectPanelCloseStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select" && root.selectPanelClosing > 0
        repeat: true
        onTriggered: {
            root.selectPanelCloseElapsed = Math.min(root.selectPanelHoldTime, Date.now() - root.selectPanelCloseStartMs);
            if (root.selectPanelCloseElapsed >= root.selectPanelHoldTime) {
                root.selectPanelClosing = 0;
            }
        }
    }

    Timer {
        id: selectHeldButtonStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select" && root.hasSelectHeldButtonTimers
        repeat: true
        onTriggered: {
            root.selectHeldButtonSkinTime = root.currentSelectHeldButtonSkinTime();
        }
    }

    Timer {
        id: selectInfoStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select" && root.selectInfoElapsed < root.selectInfoAnimationLimit
        repeat: true
        onTriggered: {
            root.selectInfoElapsed = Math.min(root.selectInfoAnimationLimit, Date.now() - root.selectInfoStartMs);
        }
    }

    Timer {
        id: readmeStopwatch
        interval: 16
        running: root.effectiveScreenKey === "select" && root.lr2ReadmeMode > 0 && root.lr2ReadmeElapsed < 500
        repeat: true
        onTriggered: {
            root.lr2ReadmeElapsed = Math.min(500, Date.now() - root.lr2ReadmeStartMs);
        }
    }

    Timer {
        id: readmeCloseTimer
        interval: 500
        repeat: false
        onTriggered: {
            root.lr2ReadmeMode = 0;
            root.lr2ReadmeText = "";
            root.lr2ReadmeLines = [];
            root.lr2ReadmeOffsetX = 0;
            root.lr2ReadmeOffsetY = 0;
            root.lr2ReadmeMouseHeld = false;
        }
    }

    Timer {
        id: readmeEdgeScrollTimer
        interval: 16
        running: root.effectiveScreenKey === "select"
            && root.lr2ReadmeMode === 1
            && root.lr2ReadmeMouseHeld
        repeat: true
        onTriggered: {
            const amount = interval * 600.0 / 1000.0;
            let dx = 0;
            let dy = 0;
            if (root.lr2ReadmeMouseX < 200) {
                dx += amount;
            } else if (root.lr2ReadmeMouseX > 440) {
                dx -= amount;
            }
            if (root.lr2ReadmeMouseY < 150) {
                dy += amount;
            } else if (root.lr2ReadmeMouseY > 330) {
                dy -= amount;
            }
            if (dx !== 0 || dy !== 0) {
                root.scrollReadmeBy(dx, dy);
            }
        }
    }

    Timer {
        id: lr2RankingTransitionTimer
        interval: 16
        repeat: true
        running: root.effectiveScreenKey === "select"
            && root.lr2RankingTransitionPhase !== 0
        onTriggered: {
            root.lr2RankingTransitionElapsed = Math.min(
                root.lr2RankingTransitionDuration,
                Date.now() - root.lr2RankingTransitionStartMs);
            if (root.lr2RankingTransitionElapsed >= root.lr2RankingTransitionDuration) {
                root.advanceLr2RankingTransition();
            }
        }
    }

    Timer {
        id: sceneEndTimer
        interval: Math.max(1, skinModel.sceneTime)
        repeat: false
        onTriggered: {
            if (root.shouldAutoAdvance) {
                globalRoot.openGameplay(root.chart);
            }
        }
    }

    Timer {
        id: gameplayStartTimer
        interval: Math.max(1, skinModel.playStart || 2000)
        repeat: false
        onTriggered: {
            if (root.gameplayStartArmed
                    && root.enabled
                    && root.isGameplayScreen()
                    && root.chart
                    && root.chartStatusIs(root.chart.status, ChartRunner.Ready)) {
                root.gameplayStartArmed = false;
                root.chart.start();
            }
        }
    }

    Timer {
        id: gameplayPoorBgaOptionTimer
        interval: 1000
        repeat: false
        onTriggered: {
            root.queueGameplayRevision();
            root.scheduleGameplayRuntimeActiveOptionsRefresh();
        }
    }

    // Timer fire times (ms since scene start). LR2 select panels use timers
    // 21..26 for side-drawer opening and 31..36 for closing, so synthesize
    // those without unfreezing the whole select skin clock.
    readonly property var timers: {
        if (root.isGameplayScreen()) {
            root.gameplayTimerRevision;
            return root.gameplayTimerValues;
        }
        if (root.isResultScreen()) {
            let resultTimers = { "0": 0 };
            if (root.acceptsInput) {
                resultTimers[1] = Math.min(root.renderSkinTime, skinModel.startInput || 0);
            }
            if (root.renderSkinTime >= root.resultGraphStartSkinTime) {
                resultTimers[150] = root.resultGraphStartSkinTime;
            }
            if (root.resultTimer151SkinTime >= 0) {
                resultTimers[151] = root.resultTimer151SkinTime;
            } else if (root.renderSkinTime >= root.resultGraphEndSkinTime) {
                resultTimers[151] = root.resultGraphEndSkinTime;
            }
            if (root.resultTimer152SkinTime >= 0) {
                resultTimers[152] = root.resultTimer152SkinTime;
            }
            return resultTimers;
        }
        if (root.effectiveScreenKey !== "select") {
            return root.zeroTimers;
        }

        let result = { "0": 0 };
        result[171] = root.selectDatabaseLoadedSkinTime;
        if (root.acceptsInput) {
            result[1] = Math.min(root.renderSkinTime, skinModel.startInput || 0);
        }
        // LR2 restarts the selected-song information timers on item
        // changes. The select skin clock is intentionally capped after the
        // intro, so synthesize those timers from their own small stopwatch.
        result[11] = root.renderSkinTime - root.selectInfoElapsed;
        if (selectContext.visualMoveActive || selectContext.scrollFixedPointDragging) {
            result[10] = root.selectScrollStartSkinTime;
            if (selectContext.scrollDirection === selectContext.lr2ScrollUp) {
                result[12] = root.selectScrollStartSkinTime;
            } else if (selectContext.scrollDirection === selectContext.lr2ScrollDown) {
                result[13] = root.selectScrollStartSkinTime;
            }
        } else if (root.acceptsInput) {
            result[14] = root.selectNoScrollStartSkinTime;
        }
        if (root.selectPanel > 0) {
            result[20 + root.selectPanel] = root.renderSkinTime - root.selectPanelElapsed;
        }
        if (root.selectPanelClosing > 0) {
            result[30 + root.selectPanelClosing] = root.renderSkinTime - root.selectPanelCloseElapsed;
        }
        if (root.lr2ReadmeMode === 1) {
            result[15] = root.renderSkinTime - root.lr2ReadmeElapsed;
        } else if (root.lr2ReadmeMode === 2) {
            result[16] = root.renderSkinTime - root.lr2ReadmeElapsed;
        }
        if (root.lr2RankingTransitionPhase !== 0) {
            result[root.lr2RankingTransitionPhase] = root.renderSkinTime - root.lr2RankingTransitionElapsed;
        }
        root.addHeldButtonTimers(result);
        return result;
    }

    Lr2SelectContext {
        id: selectContext
        enabled: root.effectiveScreenKey === "select"

        Component.onCompleted: {
            root.selectContextRef = selectContext;
        }

        onBarMoveStartMsChanged: {
            if (root.effectiveScreenKey === "select" && barMoveStartMs > 0) {
                root.selectScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onVisualMoveActiveChanged: {
            if (root.effectiveScreenKey === "select" && !visualMoveActive && !scrollFixedPointDragging) {
                root.selectNoScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onScrollFixedPointDraggingChanged: {
            if (root.effectiveScreenKey === "select" && !scrollFixedPointDragging && !visualMoveActive) {
                root.selectNoScrollStartSkinTime = root.renderSkinTime;
            }
        }

        onOpenedFolder: {
            if (root.effectiveScreenKey === "select") {
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

    OnlineRankingModel {
        id: lr2OnlineRanking
        md5: root.lr2RankingRequestMd5
        limit: 999
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
        provider: root.lr2RankingProviderEnum()
        webApiUrl: root.mainGeneralVars() ? root.mainGeneralVars().webApiUrl : ""

        onMd5Changed: {
            root.handleRankingModelChanged(false, false);
        }
    }
    Connections {
        target: lr2OnlineRanking

        function onLoadingChanged() {
            root.handleRankingModelChanged(true, true);
        }

        function onRankingEntriesChanged() {
            root.handleRankingModelChanged(true, false);
        }

        function onPlayerCountChanged() {
            root.handleRankingModelChanged(false, false);
        }

        function onScoreCountChanged() {
            root.handleRankingModelChanged(false, false);
        }

        function onClearCountsChanged() {
            root.handleRankingModelChanged(true, false);
        }

        function onChartIdChanged() {
            if (root.lr2InternetRankingOpenWhenReady && !lr2OnlineRanking.loading) {
                Qt.callLater(root.finishOpenLr2InternetRanking);
            }
        }
    }

    Lr2SkinModel {
        id: skinModel
        csvPath: root.csvPath
        settingValues: root.skinSettings || {}
        activeOptions: root.parseActiveOptions

        onSkinLoaded: {
            Qt.callLater(root.restartSkinClock);
            Qt.callLater(root.openSelectIfNeeded);
            Qt.callLater(root.activateGameplayIfNeeded);
            Qt.callLater(root.refreshGameplayRuntimeActiveOptions);
            Qt.callLater(root.updateResultOldScores);
        }
    }

    readonly property var skinModelRef: skinModel

    onCsvPathChanged: Qt.callLater(root.openSelectIfNeeded)
    onScreenKeyChanged: {
        Qt.callLater(root.openSelectIfNeeded);
        Qt.callLater(root.activateGameplayIfNeeded);
    }

    Component.onCompleted: {
        root.selectSideEffectsReady = true;
        root.commitLr2RankingRequest();
        Qt.callLater(root.restartSkinClock);
        Qt.callLater(root.openSelectIfNeeded);
        Qt.callLater(root.activateGameplayIfNeeded);
        Qt.callLater(root.updateSelectSideEffects);
        Qt.callLater(root.updateGameplaySavedScores);
        Qt.callLater(root.refreshGameplayRuntimeActiveOptions);
    }

    function stopSelectAudio() {
        selectChartSettleTimer.stop();
        previewDelayTimer.stop();
        selectBgmDelayTimer.stop();
        root.activePreviewSource = "";
        root.pendingPreviewRevision = -1;
        root.pendingPreviewSource = "";
        root.pendingPreviewRequest += 1;
        playMusic.stop();
        selectBgm.stop();
        openFolderSound.stop();
        closeFolderSound.stop();
        scratchSound.stop();
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
        if (!root.enabled
            || root.effectiveScreenKey !== "select"
            || !root.selectScratchSoundReady) {
            return;
        }
        root.playOneShot(scratchSound);
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

    function submitSelectSearch() {
        let query = selectContext.searchText.trim();
        if (query.length > 0) {
            selectContext.search(query);
        }
    }

    Keys.onUpPressed: (event) => {
        if (root.lr2ReadmeMode === 1) {
            event.accepted = true;
            root.scrollReadmeBy(0, Math.max(1, root.lr2ReadmeLineSpacing));
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
        if (!root.selectScrollReady()) return;
        event.accepted = true;
        selectContext.incrementViewIndex(event.isAutoRepeat);
    }
    Keys.onLeftPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoBack();
    }
    Keys.onRightPressed: (event) => {
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

    Input.onCol1sDownTicked: (number, type) => root.navigate(number, type, false, BmsKey.Col1sDown)
    Input.onCol1sUpTicked: (number, type) => root.navigate(number, type, true, BmsKey.Col1sUp)
    Input.onCol2sDownTicked: (number, type) => root.navigate(number, type, false, BmsKey.Col2sDown)
    Input.onCol2sUpTicked: (number, type) => root.navigate(number, type, true, BmsKey.Col2sUp)
    Input.onCol1sDownPressed: if (root.selectScrollReady()) root.lastNavigateKey.push(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: if (root.selectScrollReady()) root.lastNavigateKey.push(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: if (root.selectScrollReady()) root.lastNavigateKey.push(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: if (root.selectScrollReady()) root.lastNavigateKey.push(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sDown)
    Input.onCol1sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sUp)
    Input.onCol2sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sDown)
    Input.onCol2sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sUp)
    Input.onCol11Pressed: if (root.selectNavigationReady()) root.selectGoForward()
    Input.onCol17Pressed: if (root.selectNavigationReady()) root.selectGoForward()
    Input.onCol21Pressed: if (root.selectNavigationReady()) root.selectGoForward()
    Input.onCol27Pressed: if (root.selectNavigationReady()) root.selectGoForward()
    Input.onButtonPressed: (key) => {
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

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.selectAudioActive ? root.activePreviewSource : ""
        onSourceChanged: {
            stop();
        }
    }

    AudioPlayer {
        id: selectBgm
        looping: true
        fadeInMillis: 1000
        source: root.mainGeneralVars() ? root.mainGeneralVars().bgmPath + "select" : ""
        property bool canPlay: root.selectAudioActive
            && (!playMusic.playing || playMusic.source === "")
        onCanPlayChanged: {
            if (!canPlay) {
                stop();
            }
        }
    }

    Timer {
        id: selectBgmDelayTimer
        interval: 500
        running: selectBgm.canPlay
        repeat: false
        onTriggered: selectBgm.play()
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
        id: scratchSound
        source: root.mainGeneralVars() ? root.mainGeneralVars().soundsetPath + "scratch" : ""
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
                gameplayStartTimer.restart();
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

    Timer {
        id: selectChartSettleTimer
        interval: 120
        repeat: false
        onTriggered: root.updateDisplayedSelectChart()
    }

    Timer {
        id: previewDelayTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (!root.selectAudioActive
                || root.pendingPreviewRevision !== root.selectRevision) {
                return;
            }

            let source = root.pendingPreviewSource;
            let request = root.pendingPreviewRequest;
            root.activePreviewSource = source || "";
            if (source) {
                Qt.callLater(() => {
                    if (root.selectAudioActive
                        && root.activePreviewSource === source
                        && root.pendingPreviewRevision === root.selectRevision
                        && root.pendingPreviewRequest === request) {
                        playMusic.play();
                    }
                });
            }
        }
    }

    readonly property real skinW: 640
    readonly property real skinH: 480
    readonly property real skinScale: 1.0

    // Outer container: visual size after scaling, centered in the screen
    Item {
        id: skinViewport
        width: skinW * skinScale
        height: skinH * skinScale
        anchors.centerIn: parent

        // Inner canvas: exact viewport size - DST coords in 640x480 space
        // are scaled up to this larger size uniformly
        Item {
            id: skinContainer
            width: skinW * skinScale
            height: skinH * skinScale

            MouseArea {
                id: selectBlankMouseArea
                anchors.fill: parent
                enabled: root.selectNavigationReady()
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                z: -100000
                onPressed: (mouse) => root.updateSelectMouseFromArea(selectBlankMouseArea, mouse)
                onPositionChanged: (mouse) => {
                    if (pressed) {
                        root.updateSelectMouseFromArea(selectBlankMouseArea, mouse);
                    }
                }
                onClicked: (mouse) => {
                    root.updateSelectMouseFromArea(selectBlankMouseArea, mouse);
                    root.resetSelectSearch();
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select"
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                z: 900000
                onPositionChanged: (mouse) => {
                    root.updateSelectMousePosition(mouse.x, mouse.y);
                }
                onExited: root.hideSelectMouse()
            }

            Repeater {
                id: skinRepeater
                model: skinModel

                delegate: Loader {
                    id: elemLoader
                    x: 0; y: 0
                    width: skinW * skinScale
                    height: skinH * skinScale
                    z: root.elementZ(model.type, index, model.src, model.dsts)
                    readonly property bool usesActiveOptions: root.dstsUseActiveOptions(model.dsts)
                    readonly property bool usesTimers: root.elementUsesTimers(model.src, model.dsts)
                    readonly property bool usesSkinTime: root.elementUsesSkinTime(model.src, model.dsts)
                    readonly property var elementActiveOptions: usesActiveOptions
                        ? root.runtimeActiveOptions
                        : root.emptyActiveOptions
                    readonly property var elementTimers: usesTimers
                        ? root.timers
                        : root.zeroTimers
                    readonly property int elementSkinTime: usesSkinTime ? root.renderSkinTime : 0

                    sourceComponent: {
                        if (model.type === 0) {
                            return model.src && model.src.mouseCursor ? undefined : imageComponent;
                        } else if (model.type === 1) {
                            return numberComponent;
                        } else if (model.type === 2) {
                            return model.src && model.src.readme ? readmeTextComponent : textComponent;
                        } else if (model.type === 3) {
                            return barImageComponent;
                        } else if (model.type === 4) {
                            return barTextComponent;
                        } else if (model.type === 5) {
                            return barNumberComponent;
                        } else if (model.type === 6) {
                            return barGraphComponent;
                        } else if (model.type === 7) {
                            return bgaComponent;
                        } else if (model.type === 8) {
                            return playNotesComponent;
                        } else if (model.type === 9) {
                            return grooveGaugeComponent;
                        } else if (model.type === 10) {
                            return resultChartComponent;
                        }
                        return undefined;
                    }

                    Component {
                        id: imageComponent
                        Item {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property int spriteSkinClock: elemLoader.usesSkinTime
                                ? root.spriteSkinTime(model.dsts)
                                : 0

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: parent.spriteSkinClock
                                activeOptions: elemLoader.elementActiveOptions
                                timers: elemLoader.elementTimers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                mediaActive: root.enabled
                                transColor: skinModel.transColor
                                frameOverride: root.buttonFrame(model.src)
                                stateOverride: root.spriteStateOverride(model.src, model.dsts)
                                forceHidden: root.spriteForceHidden(model.src, model.dsts)
                                scratchAngle1: playContext.scratchAngle1
                                scratchAngle2: playContext.scratchAngle2
                            }

                            readonly property var buttonState: model.src && model.src.button
                                ? Lr2Timeline.getCurrentState(
                                    model.dsts,
                                    root.renderSkinTime,
                                    elemLoader.elementTimers,
                                    elemLoader.elementActiveOptions)
                                : null

                            MouseArea {
                                id: lr2ButtonMouseArea
                                enabled: root.effectiveScreenKey === "select"
                                    && root.acceptsInput
                                    && model.src
                                    && model.src.button
                                    && model.src.buttonClick !== 0
                                    && root.buttonPanelMatches(model.src)
                                    && !!parent.buttonState
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                hoverEnabled: true
                                x: parent.buttonState ? Math.min(parent.buttonState.x, parent.buttonState.x + parent.buttonState.w) * skinScale : 0
                                y: parent.buttonState ? Math.min(parent.buttonState.y, parent.buttonState.y + parent.buttonState.h) * skinScale : 0
                                width: parent.buttonState ? Math.abs(parent.buttonState.w) * skinScale : 0
                                height: parent.buttonState ? Math.abs(parent.buttonState.h) * skinScale : 0
                                onPressed: (mouse) => root.updateSelectMouseFromArea(lr2ButtonMouseArea, mouse)
                                onPositionChanged: (mouse) => {
                                    if (pressed) {
                                        root.updateSelectMouseFromArea(lr2ButtonMouseArea, mouse);
                                    }
                                }
                                onClicked: (mouse) => {
                                    root.updateSelectMouseFromArea(lr2ButtonMouseArea, mouse);
                                    let delta = root.buttonMouseDelta(model.src, mouse.x, width);
                                    root.handleLr2Button(model.src.buttonId, delta, model.src.buttonPanel);
                                }
                            }
                        }
                    }

                    Component {
                        id: bgaComponent
                        Lr2BgaRenderer {
                            dsts: model.dsts
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            chart: root.chart
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.isGameplayScreen()
                            poorVisible: root.gameplayPoorBgaVisible()
                        }
                    }

                    Component {
                        id: playNotesComponent
                        Lr2PlayNoteField {
                            anchors.fill: parent
                            screenRoot: root
                            skinModel: root.skinModelRef
                            skinScale: skinScale
                            renderSkinTime: root.renderSkinTime
                            runtimeActiveOptions: root.noteFieldUsesActiveOptions()
                                ? root.runtimeActiveOptions
                                : root.emptyActiveOptions
                            timers: root.noteFieldUsesTimers()
                                ? root.timers
                                : root.zeroTimers
                            transColor: root.skinModelRef ? root.skinModelRef.transColor : "black"
                            enabled: root.enabled && root.isGameplayScreen()
                        }
                    }

                    Component {
                        id: grooveGaugeComponent
                        Lr2GrooveGaugeRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            screenRoot: root
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.isGameplayScreen()
                            transColor: root.skinModelRef ? root.skinModelRef.transColor : "black"
                        }
                    }

                    Component {
                        id: resultChartComponent
                        Lr2ResultChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            screenRoot: root
                        }
                    }

                    Component {
                        id: numberComponent
                        Lr2NumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            value: root.numberValue(model.src)
                            forceHidden: root.numberForceHidden(model.src)
                            animationRevision: root.numberAnimationRevision(model.src)
                        }
                    }

                    Component {
                        id: textComponent
                        Item {
                            id: textDelegateRoot

                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property string resolvedText: root.resolveText(model.src ? model.src.st : -1)
                            readonly property var searchTextState: root.selectSearchTextState(model.src, model.dsts)
                            readonly property bool isSearchText: root.isSelectSearchText(model.src)
                            readonly property string searchFontPath: model.src ? model.src.fontPath : ""
                            readonly property int searchAlignment: model.src ? model.src.align : 0
                            readonly property bool searchTextEditing: isSearchText
                                && searchInputLoader.item
                                && searchInputLoader.item.activeFocus
                            readonly property string searchEditingText: searchInputLoader.item
                                ? searchInputLoader.item.text
                                : selectContext.searchText
                            readonly property int searchCursorPosition: searchInputLoader.item
                                ? searchInputLoader.item.cursorPosition
                                : 0
                            readonly property int searchSelectionStart: searchInputLoader.item
                                ? Math.min(searchInputLoader.item.selectionStart, searchInputLoader.item.selectionEnd)
                                : 0
                            readonly property int searchSelectionEnd: searchInputLoader.item
                                ? Math.max(searchInputLoader.item.selectionStart, searchInputLoader.item.selectionEnd)
                                : 0
                            readonly property bool searchHasSelection: searchTextEditing
                                && searchSelectionStart !== searchSelectionEnd
                            readonly property bool searchCursorVisible: searchTextEditing
                                && !searchHasSelection
                                && searchCursorOn
                            property bool searchCursorOn: true

                            function restartSearchCursorBlink() {
                                searchCursorOn = true;
                                searchCursorBlinkTimer.restart();
                            }

                            Timer {
                                id: searchCursorBlinkTimer
                                interval: 500
                                repeat: true
                                running: parent.searchTextEditing
                                onTriggered: parent.searchCursorOn = !parent.searchCursorOn
                            }

                            onSearchTextEditingChanged: restartSearchCursorBlink()
                            onSearchCursorPositionChanged: restartSearchCursorBlink()
                            onSearchSelectionStartChanged: restartSearchCursorBlink()
                            onSearchSelectionEndChanged: restartSearchCursorBlink()

                            Lr2BitmapFontText {
                                id: searchFullMeasure
                                x: -10000
                                y: -10000
                                width: searchInputLoader.item ? searchInputLoader.item.width : 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: searchInputLoader.parent.searchEditingText
                            }

                            Lr2BitmapFontText {
                                id: searchCursorMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchCursorPosition)
                            }

                            Lr2BitmapFontText {
                                id: searchSelectionStartMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchSelectionStart)
                            }

                            Lr2BitmapFontText {
                                id: searchSelectionEndMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchSelectionEnd)
                            }

                            readonly property real searchTextScaleY: searchFullMeasure.naturalHeight > 0 && searchInputLoader.item
                                ? searchInputLoader.item.height / searchFullMeasure.naturalHeight
                                : 1
                            readonly property real searchTextFitScaleX: searchFullMeasure.naturalWidth > 0 && searchInputLoader.item
                                && searchFullMeasure.naturalWidth > searchInputLoader.item.width
                                ? searchInputLoader.item.width / searchFullMeasure.naturalWidth
                                : 1
                            readonly property real searchTextScaleX: searchTextScaleY * searchTextFitScaleX
                            readonly property real searchDrawnWidth: searchFullMeasure.naturalWidth * searchTextScaleX
                            readonly property real searchTextOriginX: searchInputLoader.item
                                ? searchInputLoader.item.x + (
                                    searchAlignment === 1
                                        ? -searchDrawnWidth / 2
                                        : (searchAlignment === 2 ? -searchDrawnWidth : 0))
                                : 0
                            readonly property real searchTextOriginY: searchInputLoader.item
                                ? searchInputLoader.item.y
                                : 0
                            property int searchDragAnchor: 0

                            function searchCursorPositionAt(parentX) {
                                const text = searchEditingText || "";
                                if (text.length <= 0 || searchTextScaleX <= 0) {
                                    return 0;
                                }

                                const sourceX = Math.max(
                                    0,
                                    (parentX - searchTextOriginX) / searchTextScaleX);
                                let previousWidth = 0;
                                for (let i = 1; i <= text.length; ++i) {
                                    const item = searchPrefixMeasureRepeater.itemAt(i);
                                    const width = item ? item.naturalWidth : previousWidth;
                                    if (sourceX < previousWidth + (width - previousWidth) / 2) {
                                        return i - 1;
                                    }
                                    previousWidth = width;
                                }
                                return text.length;
                            }

                            function moveSearchCursorTo(parentX, selecting) {
                                if (!searchInputLoader.item) {
                                    return;
                                }
                                const position = searchCursorPositionAt(parentX);
                                searchInputLoader.item.syncFromContext();
                                searchInputLoader.item.forceActiveFocus();
                                if (selecting) {
                                    searchInputLoader.item.select(searchDragAnchor, position);
                                } else {
                                    searchDragAnchor = position;
                                    searchInputLoader.item.cursorPosition = position;
                                    searchInputLoader.item.deselect();
                                }
                                restartSearchCursorBlink();
                            }

                            Repeater {
                                id: searchPrefixMeasureRepeater
                                model: searchInputLoader.parent.isSearchText
                                    ? searchInputLoader.parent.searchEditingText.length + 1
                                    : 0

                                Lr2BitmapFontText {
                                    x: -10000
                                    y: -10000
                                    width: 1
                                    height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                    opacity: 0
                                    fontPath: textDelegateRoot.searchFontPath
                                    text: root.textPrefix(
                                        textDelegateRoot.searchEditingText,
                                        index)
                                }
                            }

                            Rectangle {
                                z: 1
                                visible: parent.searchHasSelection && searchInputLoader.item
                                x: parent.searchTextOriginX
                                    + searchSelectionStartMeasure.naturalWidth * parent.searchTextScaleX
                                y: parent.searchTextOriginY
                                width: Math.max(
                                    skinScale,
                                    (searchSelectionEndMeasure.naturalWidth
                                        - searchSelectionStartMeasure.naturalWidth)
                                        * parent.searchTextScaleX)
                                height: searchInputLoader.item ? searchInputLoader.item.height : 0
                                color: Qt.rgba(0.45, 0.72, 1.0, 0.45)
                            }

                            Lr2TextRenderer {
                                z: 2
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: elemLoader.elementSkinTime
                                activeOptions: elemLoader.elementActiveOptions
                                timers: elemLoader.elementTimers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                resolvedText: parent.resolvedText
                            }

                            Loader {
                                id: searchInputLoader
                                z: 4
                                active: parent.isSearchText
                                sourceComponent: TextInput {
                                    id: searchInput

                                    property bool syncing: false
                                    readonly property var textState: searchInputLoader.parent.searchTextState

                                    function syncFromContext() {
                                        if (text === selectContext.searchText) {
                                            return;
                                        }
                                        syncing = true;
                                        text = selectContext.searchText;
                                        syncing = false;
                                    }

                                    x: textState ? Math.min(textState.x, textState.x + textState.w) * skinScale : 0
                                    y: textState ? Math.min(textState.y, textState.y + textState.h) * skinScale : 0
                                    width: textState ? Math.abs(textState.w) * skinScale : 0
                                    height: textState ? Math.abs(textState.h) * skinScale : 0
                                    visible: !!textState
                                    enabled: !!textState
                                    opacity: textState ? textState.a / 255.0 : 0
                                    clip: true
                                    activeFocusOnTab: false
                                    inputMethodHints: Qt.ImhNoPredictiveText

                                    color: "transparent"
                                    cursorVisible: false
                                    selectionColor: "transparent"
                                    selectedTextColor: "transparent"

                                    Component.onCompleted: {
                                        root.selectSearchInputItem = searchInput;
                                        syncFromContext();
                                    }

                                    Component.onDestruction: {
                                        if (root.selectSearchInputItem === searchInput) {
                                            root.selectSearchInputItem = null;
                                        }
                                    }

                                    onActiveFocusChanged: {
                                        if (activeFocus) {
                                            syncFromContext();
                                            cursorPosition = text.length;
                                        }
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onCursorPositionChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onSelectionStartChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onSelectionEndChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onTextEdited: {
                                        if (!syncing && selectContext.searchText !== text) {
                                            selectContext.searchText = text;
                                            selectContext.touch();
                                        }
                                    }

                                    Keys.onReturnPressed: (event) => {
                                        event.accepted = true;
                                        root.submitSelectSearch();
                                        root.clearSelectSearchFocus();
                                    }

                                    Keys.onEnterPressed: (event) => {
                                        event.accepted = true;
                                        root.submitSelectSearch();
                                        root.clearSelectSearchFocus();
                                    }

                                    Keys.onEscapePressed: (event) => {
                                        event.accepted = true;
                                        root.resetSelectSearch();
                                    }

                                    Connections {
                                        target: selectContext
                                        function onSearchTextChanged() {
                                            const oldPosition = searchInput.cursorPosition;
                                            searchInput.syncFromContext();
                                            searchInput.cursorPosition = Math.min(
                                                oldPosition,
                                                searchInput.text.length);
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                z: 5
                                visible: parent.searchCursorVisible && searchInputLoader.item
                                x: parent.searchTextOriginX
                                    + searchCursorMeasure.naturalWidth * parent.searchTextScaleX
                                y: parent.searchTextOriginY
                                width: Math.max(1, skinScale)
                                height: searchInputLoader.item ? searchInputLoader.item.height : 0
                                color: "white"
                            }

                            MouseArea {
                                id: searchEditMouseArea
                                z: 6
                                enabled: !!parent.searchTextState
                                acceptedButtons: Qt.LeftButton
                                preventStealing: true
                                x: parent.searchTextState ? Math.min(parent.searchTextState.x, parent.searchTextState.x + parent.searchTextState.w) * skinScale : 0
                                y: parent.searchTextState ? Math.min(parent.searchTextState.y, parent.searchTextState.y + parent.searchTextState.h) * skinScale : 0
                                width: parent.searchTextState ? Math.abs(parent.searchTextState.w) * skinScale : 0
                                height: parent.searchTextState ? Math.abs(parent.searchTextState.h) * skinScale : 0
                                onPressed: (mouse) => {
                                    root.updateSelectMouseFromArea(searchEditMouseArea, mouse);
                                    mouse.accepted = true;
                                    parent.moveSearchCursorTo(x + mouse.x, false);
                                }
                                onPositionChanged: (mouse) => {
                                    if (pressed) {
                                        root.updateSelectMouseFromArea(searchEditMouseArea, mouse);
                                        mouse.accepted = true;
                                        parent.moveSearchCursorTo(x + mouse.x, true);
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: readmeTextComponent
                        Item {
                            id: readmeTextDelegateRoot
                            width: skinW * skinScale
                            height: skinH * skinScale
                            clip: true
                            readonly property var readmeSrc: model.src
                            readonly property var readmeDsts: model.dsts

                            Component.onCompleted: {
                                if (readmeSrc && readmeSrc.readme && readmeSrc.readmeId === 0) {
                                    root.lr2ReadmeLineSpacing = Math.max(1, readmeSrc.readmeLineSpacing || 18);
                                    root.clampReadmeOffsets();
                                }
                            }

                            Repeater {
                                model: root.readmeLinesForSource(readmeTextDelegateRoot.readmeSrc)

                                Lr2TextRenderer {
                                    anchors.fill: parent
                                    dsts: readmeTextDelegateRoot.readmeDsts
                                    srcData: readmeTextDelegateRoot.readmeSrc
                                    skinTime: root.renderSkinTime
                                    activeOptions: elemLoader.elementActiveOptions
                                    timers: elemLoader.elementTimers
                                    chart: root.renderChart
                                    scaleOverride: skinScale
                                    offsetX: root.lr2ReadmeOffsetX
                                    offsetY: root.lr2ReadmeOffsetY
                                        + index * (readmeTextDelegateRoot.readmeSrc
                                            ? readmeTextDelegateRoot.readmeSrc.readmeLineSpacing
                                            : root.lr2ReadmeLineSpacing)
                                    resolvedText: modelData
                                }
                            }
                        }
                    }

                    Component {
                        id: barImageComponent
                        Lr2BarSpriteRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barScrollOffset: selectContext.scrollOffset
                            barCenter: skinModel.barCenter
                            transColor: skinModel.transColor
                        }
                    }

                    Component {
                        id: barTextComponent
                        Lr2BarTextRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barScrollOffset: selectContext.scrollOffset
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barNumberComponent
                        Lr2BarNumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barScrollOffset: selectContext.scrollOffset
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barGraphComponent
                        Lr2BarGraphRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            sourceSkinTime: root.barGraphSourceSkinTime(model.src)
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            value: root.resolveBarGraph(model.src ? model.src.graphType : 0)
                            animateValue: root.effectiveScreenKey === "select"
                                && model.src
                                && model.src.graphType >= 5
                                && model.src.graphType <= 9
                        }
                    }
                }
            }

            Text {
                visible: root.effectiveScreenKey === "select" && root.clearStatusIsBest()
                text: "BEST"
                x: 80 * skinScale
                y: 459 * skinScale
                z: 100200
                color: "white"
                font.family: "Arial"
                font.pixelSize: 6 * skinScale
                font.bold: true
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: barRowsMouseArea
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select"
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                propagateComposedEvents: true
                z: 100000
                property int pressedRow: -1

                function rowAt(mouse) {
                    if (!root.selectScrollReady()) {
                        return -1;
                    }
                    let states = root.cachedBarBaseStates || [];
                    let px = mouse.x / skinScale;
                    let py = mouse.y / skinScale;
                    for (let row = root.barClickEnd(); row >= root.barClickStart(); --row) {
                        let state = row >= 0 && row < states.length ? states[row] : null;
                        if (!state) {
                            continue;
                        }
                        let left = Math.min(state.x, state.x + state.w);
                        let right = Math.max(state.x, state.x + state.w);
                        let top = Math.min(state.y, state.y + state.h);
                        let bottom = Math.max(state.y, state.y + state.h);
                        if (px >= left && px <= right && py >= top && py <= bottom) {
                            return row;
                        }
                    }
                    return -1;
                }

                onPressed: (mouse) => {
                    root.updateSelectMouseFromArea(barRowsMouseArea, mouse);
                    pressedRow = rowAt(mouse);
                    if (pressedRow < 0) {
                        mouse.accepted = false;
                    }
                }
                onPositionChanged: (mouse) => {
                    if (pressedRow >= 0 && pressed) {
                        root.updateSelectMouseFromArea(barRowsMouseArea, mouse);
                    }
                }
                onClicked: (mouse) => {
                    let row = pressedRow >= 0 ? pressedRow : rowAt(mouse);
                    pressedRow = -1;
                    if (row < 0) {
                        mouse.accepted = false;
                        return;
                    }
                    root.updateSelectMouseFromArea(barRowsMouseArea, mouse);
                    root.handleBarRowClick(row, mouse);
                }
                onDoubleClicked: (mouse) => {
                    let row = pressedRow >= 0 ? pressedRow : rowAt(mouse);
                    pressedRow = -1;
                    if (row < 0 || mouse.button !== Qt.LeftButton) {
                        mouse.accepted = false;
                        return;
                    }
                    root.updateSelectMouseFromArea(barRowsMouseArea, mouse);
                    selectContext.selectVisibleRow(row, skinModel.barCenter);
                    root.selectGoForward(selectContext.current);
                }
                onCanceled: pressedRow = -1
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }

            Repeater {
                model: skinModel

                MouseArea {
                    id: sliderMouseArea
                    readonly property bool selectScroll: root.isSelectScrollSlider(model.src)
                    readonly property bool genericSlider: root.isLr2GenericSlider(model.src)
                    readonly property var trackState: selectScroll
                        ? root.selectScrollSliderTrackState(model.src, model.dsts)
                        : root.lr2GenericSliderTrackState(model.src, model.dsts)
                    enabled: root.selectScrollReady() && !!trackState
                    acceptedButtons: Qt.LeftButton
                    preventStealing: true
                    x: trackState ? Math.min(trackState.x, trackState.x + trackState.w) * skinScale : 0
                    y: trackState ? Math.min(trackState.y, trackState.y + trackState.h) * skinScale : 0
                    width: trackState ? Math.abs(trackState.w) * skinScale : 0
                    height: trackState ? Math.abs(trackState.h) * skinScale : 0
                    z: 100300 + index
                    function updateSlider(mouse) {
                        let pointerX = (x + mouse.x) / skinScale;
                        let pointerY = (y + mouse.y) / skinScale;
                        if (selectScroll) {
                            root.setSelectScrollFromSliderTrack(model.src,
                                                                trackState,
                                                                pointerX,
                                                                pointerY);
                        } else if (genericSlider) {
                            root.setLr2GenericSliderFromTrack(model.src,
                                                              trackState,
                                                              pointerX,
                                                              pointerY);
                        }
                    }
                    onPressed: (mouse) => {
                        root.updateSelectMouseFromArea(sliderMouseArea, mouse);
                        root.selectSliderFixedPoint = -1;
                        if (selectScroll) {
                            root.selectScrollStartSkinTime = root.renderSkinTime;
                            selectContext.beginScrollFixedPointDrag();
                        }
                        updateSlider(mouse);
                        mouse.accepted = true;
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            root.updateSelectMouseFromArea(sliderMouseArea, mouse);
                            updateSlider(mouse);
                        }
                    }
                    onReleased: {
                        if (selectScroll) {
                            selectContext.finishScrollFixedPoint(100);
                            root.selectSliderFixedPoint = -1;
                        }
                    }
                    onCanceled: {
                        if (selectScroll) {
                            selectContext.finishScrollFixedPoint(100);
                            root.selectSliderFixedPoint = -1;
                        }
                    }
                    onWheel: (wheel) => root.handleSelectWheel(wheel)
                }
            }

            Lr2NativeCursor {
                id: nativeCursor
                anchors.fill: parent

                readonly property var cursorSrcData: skinModel.mouseCursor && skinModel.mouseCursor.src
                    ? skinModel.mouseCursor.src
                    : null
                readonly property var cursorDsts: skinModel.mouseCursor && skinModel.mouseCursor.dsts
                    ? skinModel.mouseCursor.dsts
                    : []
                readonly property var cursorState: cursorSrcData
                    ? Lr2Timeline.getCurrentState(cursorDsts,
                                                  root.renderSkinTime,
                                                  root.timers,
                                                  root.runtimeActiveOptions)
                    : null
                readonly property bool wholeTextureSource: cursorSrcData
                    && (cursorSrcData.x < 0 || cursorSrcData.y < 0
                        || cursorSrcData.w < 0 || cursorSrcData.h < 0)
                readonly property bool croppedTextureSource: cursorSrcData
                    && cursorSrcData.w > 0 && cursorSrcData.h > 0
                readonly property string resolvedSource: {
                    if (!cursorSrcData || !cursorSrcData.source) {
                        return "";
                    }
                    let absPath = cursorSrcData.source.replace(/\\/g, "/");
                    if (/^[A-Za-z]:\//.test(absPath)) {
                        return "file:///" + absPath;
                    }
                    if (absPath.startsWith("/")) {
                        return "file://" + absPath;
                    }
                    return absPath;
                }
                readonly property int frameIndex: {
                    if (!cursorSrcData) {
                        return 0;
                    }
                    let timerIdx = cursorSrcData.timer || 0;
                    let fire = root.timers && root.timers[timerIdx] !== undefined
                        ? root.timers[timerIdx]
                        : -1;
                    return Lr2Timeline.getAnimationFrame(cursorSrcData,
                                                         root.renderSkinTime,
                                                         fire);
                }
                readonly property rect clipRect: {
                    if (!cursorSrcData || wholeTextureSource || !croppedTextureSource) {
                        return Qt.rect(0, 0, 0, 0);
                    }

                    let sx = Math.max(0, cursorSrcData.x || 0);
                    let sy = Math.max(0, cursorSrcData.y || 0);
                    let sw = cursorSrcData.w;
                    let sh = cursorSrcData.h;
                    let divX = Math.max(1, cursorSrcData.div_x || 1);
                    let divY = Math.max(1, cursorSrcData.div_y || 1);
                    let cellW = sw / divX;
                    let cellH = sh / divY;
                    let col = frameIndex % divX;
                    let row = Math.floor(frameIndex / divX) % divY;

                    return Qt.rect(sx + col * cellW, sy + row * cellH, cellW, cellH);
                }

                active: root.effectiveScreenKey === "select"
                    && root.selectMouseX > -9999
                    && root.selectMouseY > -9999
                    && resolvedSource !== ""
                source: resolvedSource
                sourceRect: clipRect
                targetSize: cursorState
                    ? Qt.size(cursorState.w * skinScale, cursorState.h * skinScale)
                    : Qt.size(0, 0)
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select"
                acceptedButtons: Qt.NoButton
                z: 100200
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }

            MouseArea {
                id: readmeMouseArea
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select" && root.lr2ReadmeMode === 1
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                z: 100250

                function updateReadmeMouse(mouse) {
                    root.updateSelectMouseFromArea(readmeMouseArea, mouse);
                    const point = readmeMouseArea.mapToItem(skinContainer, mouse.x, mouse.y);
                    root.lr2ReadmeMouseX = point.x / skinScale;
                    root.lr2ReadmeMouseY = point.y / skinScale;
                }

                onPressed: (mouse) => {
                    updateReadmeMouse(mouse);
                    if (mouse.button === Qt.RightButton) {
                        root.closeReadme();
                    } else {
                        root.lr2ReadmeMouseHeld = true;
                    }
                    mouse.accepted = true;
                }
                onPositionChanged: (mouse) => {
                    updateReadmeMouse(mouse);
                    mouse.accepted = true;
                }
                onReleased: (mouse) => {
                    updateReadmeMouse(mouse);
                    root.lr2ReadmeMouseHeld = false;
                    mouse.accepted = true;
                }
                onCanceled: root.lr2ReadmeMouseHeld = false
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }
        }
    }
}
