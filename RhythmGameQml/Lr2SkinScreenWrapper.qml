import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root
    focus: true
    property string csvPath
    property string screenKey: ""
    property var chart
    property var skinSettings
    property var selectContextRef: null

    onEnabledChanged: {
        if (root.effectiveScreenKey === "decide" && enabled) {
            Qt.callLater(() => sceneStack.pop());
        }
        if (enabled) {
            Qt.callLater(root.openSelectIfNeeded);
        }
    }

    function openSelectIfNeeded() {
        if (root.effectiveScreenKey === "select" && selectContext.historyStack.length === 0) {
            selectContext.openRoot();
        }
        root.scheduleSelectSideEffects();
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
            sceneStack.pop();
        }
    }
    
    readonly property string effectiveScreenKey: root.screenKey || root.inferScreenKey(root.csvPath)
    readonly property var parseActiveOptions: {
        let options = [52]; // non-extra mode
        if (root.effectiveScreenKey === "select") {
            // Option 46 gates the difficulty-bar blocks structurally (#IF),
            // so it must be present before parsing, not only at render time.
            options.push(46, 900, 905, 910, 915); // stock LR2 select defaults
        } else if (root.effectiveScreenKey === "decide") {
            options.push(900); // stock LR2 decide default: show stagefile
        }
        return options;
    }
    property int selectPanel: 0
    property int selectPanelHeldByStart: 0
    property double selectPanelStartMs: 0
    property int selectPanelElapsed: 0
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
    property real selectMouseX: -100000
    property real selectMouseY: -100000
    property bool selectScratchSoundReady: false

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
        let copy = root.copyObject(root.lr2SliderValues);
        copy[String(type)] = Math.max(0, Math.min(100, Math.round(value)));
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
        readmeCloseTimer.stop();
        readmeStopwatch.restart();
        root.clearSelectSearchFocus();
    }

    function openReadmePath(path) {
        if (!path) {
            return false;
        }
        root.openReadmeText(Rg.fileQuery.readTextFile(path));
        return true;
    }

    function closeReadme() {
        if (root.lr2ReadmeMode === 0) {
            return;
        }
        root.lr2ReadmeMode = 2;
        root.lr2ReadmeStartMs = Date.now();
        root.lr2ReadmeElapsed = 0;
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

    function gaugeIndex(side) {
        let vars = root.generalVarsForSide(side);
        return vars ? root.indexOfValue(root.lr2GaugeValues, vars.gaugeType) : 0;
    }

    function setGaugeIndex(side, index) {
        let vars = root.generalVarsForSide(side);
        if (vars) {
            vars.gaugeType = root.lr2GaugeValues[root.wrapValue(index, root.lr2GaugeValues.length)];
        }
    }

    function randomIndex(side) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return 0;
        }
        let value = side === 2 && !Rg.profileList.battleActive
            ? vars.noteOrderAlgorithmP2
            : vars.noteOrderAlgorithm;
        return root.indexOfValue(root.lr2RandomValues, value);
    }

    function setRandomIndex(side, index) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let value = root.lr2RandomValues[root.wrapValue(index, root.lr2RandomValues.length)];
        if (side === 2 && !Rg.profileList.battleActive) {
            vars.noteOrderAlgorithmP2 = value;
        } else {
            vars.noteOrderAlgorithm = value;
        }
    }

    function hidSudIndex(side) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return 0;
        }
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

    function hiSpeedFixIndex() {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2HiSpeedFixValues, vars.hiSpeedFix) : 0;
    }

    function setHiSpeedFixIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.hiSpeedFix = root.lr2HiSpeedFixValues[root.wrapValue(index, root.lr2HiSpeedFixValues.length)];
        }
    }

    function battleIndex() {
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

    function flipIndex() {
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

    function laneCoverIndex() {
        let vars = root.mainGeneralVars();
        return vars && vars.laneCoverOn ? 1 : 0;
    }

    function setLaneCoverIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.laneCoverOn = root.wrapValue(index, 2) === 1;
        }
    }

    function bgaIndex() {
        let vars = root.mainGeneralVars();
        return vars && !vars.bgaOn ? 1 : 0;
    }

    function setBgaIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.bgaOn = root.wrapValue(index, 2) === 0;
        }
    }

    function scoreTargetIndex() {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2TargetValues, vars.scoreTarget) : 0;
    }

    function setScoreTargetIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.scoreTarget = root.lr2TargetValues[root.wrapValue(index, root.lr2TargetValues.length)];
        }
    }

    function targetPercent() {
        let vars = root.mainGeneralVars();
        return vars ? Math.round((vars.targetScoreFraction || 0) * 100) : 80;
    }

    function setTargetPercent(percent) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.targetScoreFraction = Math.max(0, Math.min(1, percent / 100));
        }
    }

    function hiSpeedNumber(side) {
        let vars = root.generalVarsForSide(side);
        if (!vars || vars.noteScreenTimeMillis <= 0) {
            return 100;
        }
        return Math.max(1, Math.min(999, Math.round(100000 / vars.noteScreenTimeMillis)));
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

    function clearStatusOption() {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return 62;
        }
        if (vars.gaugeMode === GaugeMode.Best) {
            return 0;
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
        return root.effectiveScreenKey === "select"
            && root.acceptsInput
            && !root.selectSearchHasFocus();
    }

    function selectScrollReady() {
        return root.selectInputReady() && root.lr2ReadmeMode === 0;
    }

    function selectNavigationReady() {
        return root.selectInputReady()
            && root.lr2ReadmeMode === 0
            && root.selectPanel === 0;
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

    function timelineSortId(dsts, skinTime, activeOptions) {
        let state = Lr2Timeline.getCurrentState(
            dsts,
            skinTime,
            root.timers,
            activeOptions || root.runtimeActiveOptions);
        if (state && state.sortId !== undefined) {
            return state.sortId;
        }
        return root.fallbackSortId(dsts);
    }

    function elementZ(type, index, src, dsts) {
        // OpenLR2 stamps every DST keyframe with the skin command counter and
        // globally sorts DrawingBuf by that value before drawing.
        let skinTime = root.effectiveScreenKey === "select" && type >= 3 && type <= 5
            ? root.barSkinTime
            : root.renderSkinTime;
        let activeOptions = root.effectiveScreenKey === "select" && type >= 3 && type <= 5
            ? root.barActiveOptions
            : root.runtimeActiveOptions;
        return root.timelineSortId(dsts, skinTime, activeOptions) + index * 0.000001;
    }

    function addOption(options, option) {
        if (option && options.indexOf(option) === -1) {
            options.push(option);
        }
    }

    function appendSelectStatusOptions(options) {
        root.addOption(options, root.isLoggedIn() ? 51 : 50);
        root.addOption(options, 61);
        root.addOption(options, root.clearStatusOption());
    }

    function appendChartOptions(options, chartData) {
        if (!chartData) {
            addOption(options, 190);
            addOption(options, 192);
            addOption(options, 194);
            addOption(options, 170);
            addOption(options, 172);
            addOption(options, 174);
            addOption(options, 176);
            addOption(options, 178);
            addOption(options, 182);
            addOption(options, 185);
            addOption(options, 196);
            return;
        }
        addOption(options, chartData.stageFile ? 191 : 190);
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
        }

        let keymode = chartData.keymode || 0;
        if (keymode === 7) addOption(options, 160);
        else if (keymode === 5) addOption(options, 161);
        else if (keymode === 14) addOption(options, 162);
        else if (keymode === 10) addOption(options, 163);
    }

    readonly property var baseActiveOptions: {
        let result = [];
        let staticOptions = skinModel.effectiveActiveOptions && skinModel.effectiveActiveOptions.length
            ? skinModel.effectiveActiveOptions
            : root.parseActiveOptions;
        for (let option of staticOptions) {
            root.addOption(result, option);
        }

        root.addOption(result, 46);  // difficulty filter enabled
        root.addOption(result, 52);  // non-extra mode
        root.addOption(result, 572); // not course-select mode
        root.addOption(result, 600); // not IR target
        root.addOption(result, 620); // not ranking display
        root.addOption(result, 622); // not ghost battle
        root.addOption(result, 624); // not rival compare
        if (root.selectPanel > 0) {
            root.addOption(result, 20);
            root.addOption(result, 20 + root.selectPanel);
        }

        return result;
    }

    readonly property var barActiveOptions: root.baseActiveOptions

    readonly property var runtimeActiveOptions: {
        let result = root.baseActiveOptions.slice();
        let revision = selectContext.selectionRevision + selectContext.scoreRevision;

        if (root.effectiveScreenKey === "select") {
            root.appendSelectStatusOptions(result);
            let item = selectContext.current;
            if (selectContext.isChart(item) || selectContext.isEntry(item)) {
                root.addOption(result, 2);
                root.addOption(result, 5);
            } else if (selectContext.isCourse(item)) {
                root.addOption(result, 3);
                root.addOption(result, 5);
                root.addOption(result, 290);
            } else {
                root.addOption(result, 1);
            }

            let selectedChart = selectContext.selectedChartData();
            let selectedKeymode = selectedChart ? (selectedChart.keymode || 0) : 0;
            if (Rg.profileList.battleActive) {
                root.addOption(result, 11);
                root.addOption(result, 12);
            } else if (selectedKeymode === 10 || selectedKeymode === 14
                       || (root.mainGeneralVars() && root.mainGeneralVars().dpOptions === DpOptions.Battle)) {
                root.addOption(result, 12);
            }
            let lamp = selectContext.entryLamp(item);
            if (lamp === 0) root.addOption(result, 100);
            else if (lamp === 1) root.addOption(result, 101);
            else if (lamp === 2) root.addOption(result, 102);
            else if (lamp === 3) root.addOption(result, 103);
            else if (lamp === 4) root.addOption(result, 104);
            else if (lamp === 5) root.addOption(result, 105);

            let rank = selectContext.entryRank(item);
            if (rank >= 8) root.addOption(result, 110);
            else if (rank === 7) root.addOption(result, 111);
            else if (rank === 6) root.addOption(result, 112);
            else if (rank === 5) root.addOption(result, 113);
            else if (rank === 4) root.addOption(result, 114);
            else if (rank === 3) root.addOption(result, 115);
            else if (rank === 2) root.addOption(result, 116);
            else if (rank === 1) root.addOption(result, 117);

            if (selectedChart) {
                for (let diff = 1; diff <= 5; ++diff) {
                    if (selectContext.hasDifficulty(diff)) {
                        root.addOption(result, 504 + diff);
                        root.addOption(result, selectContext.difficultyLevelBarOption(diff));
                    } else {
                        root.addOption(result, 499 + diff);
                    }
                    let diffCount = selectContext.difficultyCount(diff);
                    if (diffCount === 1) {
                        root.addOption(result, 509 + diff);
                    } else if (diffCount > 1) {
                        root.addOption(result, 514 + diff);
                    }
                    root.addOption(result, 510 + diff * 10 + selectContext.difficultyLamp(diff));
                }
            }
            root.appendChartOptions(result, selectContext.selectedChartData());
            for (let optionId of selectContext.scoreOptionIds(item)) {
                root.addOption(result, optionId);
            }
        } else {
            root.appendChartOptions(result, root.chart && root.chart.chartData ? root.chart.chartData : null);
        }

        return result;
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
        : root.chart
    readonly property var selectedCourseStages: {
        let revision = root.effectiveScreenKey === "select" ? selectContext.selectionRevision : 0;
        let item = root.effectiveScreenKey === "select" ? selectContext.current : null;
        if (!item || !selectContext.isCourse(item) || !item.loadCharts) {
            return [];
        }
        return item.loadCharts();
    }

    onSelectRevisionChanged: root.scheduleSelectSideEffects()
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
            root.lr2ReadmeMode = 0;
        }
        root.restartSelectInfoTimer();
        root.scheduleSelectSideEffects();
    }
    onChartChanged: {
        if (root.effectiveScreenKey !== "select") {
            root.deferredSelectChart = root.chart;
        }
    }

    function refreshDeferredSelectChart() {
        if (root.effectiveScreenKey === "select") {
            root.deferredSelectChart = selectContext.selectedChartWrapper();
        } else {
            root.deferredSelectChart = root.chart;
        }
    }

    function scheduleSelectSideEffects() {
        if (!root.selectSideEffectsReady) {
            return;
        }

        if (root.effectiveScreenKey !== "select") {
            selectChartSettleTimer.stop();
            previewDelayTimer.stop();
            root.activePreviewSource = "";
            root.pendingPreviewRevision = -1;
            root.pendingPreviewRequest += 1;
            root.pendingPreviewSource = "";
            root.refreshDeferredSelectChart();
            playMusic.stop();
            selectBgm.stop();
            return;
        }

        if (!root.deferredSelectChart || skinModel.reloadBanner) {
            root.refreshDeferredSelectChart();
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
        return chart ? (chart.title || "") : "";
    }

    function chartSubtitle(chart) {
        return chart ? (chart.subtitle || "") : "";
    }

    function lr2SelectOptionText(st) {
        switch (st) {
        case 63:
            return root.optionText(root.lr2RandomLabels, root.randomIndex(1));
        case 64:
            return root.optionText(root.lr2RandomLabels, root.randomIndex(2));
        case 65:
            return root.optionText(root.lr2GaugeLabels, root.gaugeIndex(1));
        case 66:
            return root.optionText(root.lr2GaugeLabels, root.gaugeIndex(2));
        case 67:
            return "OFF";
        case 68:
            return "OFF";
        case 69:
            return root.optionText(root.lr2BattleLabels, root.battleIndex());
        case 70:
            return root.optionText(["OFF", "FLIP"], root.flipIndex());
        case 71:
            return "OFF";
        case 72:
            return "OFF";
        case 73:
            return root.optionText(["OFF", "ON"], root.laneCoverIndex());
        case 74:
            return root.optionText(root.lr2HiSpeedFixLabels, root.hiSpeedFixIndex());
        case 75:
            return "NORMAL";
        case 76:
            return root.optionText(["ON", "OFF"], root.bgaIndex());
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
            return root.optionText(root.lr2HidSudLabels, root.hidSudIndex(1));
        case 85:
            return root.optionText(root.lr2HidSudLabels, root.hidSudIndex(2));
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
            : 0;
        let chartData = root.effectiveScreenKey === "select"
            ? selectContext.selectedChartData()
            : (root.chart ? root.chart.chartData : null);
        let currentEntry = root.effectiveScreenKey === "select" ? selectContext.current : null;
        switch (st) {
        case 1:
            return root.optionText(root.lr2TargetLabels, root.scoreTargetIndex());
        case 2:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 10:
            if (chartData) {
                return (chartData.title || "").replace(/\r\n|\n|\r/g, " ");
            }
            if (root.effectiveScreenKey === "select") {
                return selectContext.entryMainTitle(currentEntry);
            }
            return root.chart && root.chart.course ? root.chart.course.name : "";
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
            return root.effectiveScreenKey === "select" ? selectContext.entryMainTitle(currentEntry) : "";
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

    function resolveNumber(num) {
        if (root.effectiveScreenKey === "select") {
            if (num === 10) {
                return root.hiSpeedNumber(1);
            }
            if (num === 11) {
                return root.hiSpeedNumber(2);
            }
            if (num === 13) {
                return root.targetPercent();
            }
            if ((num >= 50 && num <= 66) || num === 8) {
                return root.lr2SliderNumber(num);
            }
            if (num >= 250 && num <= 254) {
                let stage = root.courseStage(num - 250);
                return stage ? (stage.playLevel || 0) : 0;
            }
            if ((num >= 300 && num <= 382) || (num >= 92 && num <= 95)
                    || num === 220 || num === 271 || num === 272
                    || num === 274 || num === 275 || num === 276
                    || num === 292 || num === 293) {
                return 0;
            }
            return selectContext.numberValue(num);
        }
        return 0;
    }

    function resolveBarGraph(type) {
        if (root.effectiveScreenKey === "select") {
            return selectContext.barGraphValue(type);
        }
        return 0;
    }

    function wrapValue(value, count) {
        return ((value % count) + count) % count;
    }

    function buttonDelta(src) {
        if (src && src.buttonPlusOnly !== 0) {
            return src.buttonPlusOnly;
        }
        return 1;
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
        let delta = root.buttonDelta(src);
        if (src && root.buttonUsesSplitArrows(src.buttonId)) {
            return mouseX < width / 2 ? -delta : delta;
        }
        return delta;
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
            return root.gaugeIndex(1);
        case 41:
            return root.gaugeIndex(2);
        case 42:
            return root.randomIndex(1);
        case 43:
            return root.randomIndex(2);
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
            return root.laneCoverIndex();
        case 50:
            return root.hidSudIndex(1);
        case 51:
            return root.hidSudIndex(2);
        case 54:
            return root.flipIndex();
        case 55:
            return root.hiSpeedFixIndex();
        case 56:
            return root.battleIndex();
        case 70:
            return 0;
        case 71:
            return 0;
        case 72:
            return root.bgaIndex();
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
        }
        root.selectPanel = 0;
        root.selectPanelHeldByStart = 0;
        root.selectPanelElapsed = 0;
        root.selectHeldButtonTimerStarts = ({});
    }

    function openSelectPanel(panel, heldByStart) {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel !== panel) {
            root.playOneShot(optionOpenSound);
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
            root.selectGoForward(selectContext.current);
            break;
        case 16:
            root.selectGoForward(selectContext.current, true);
            break;
        case 19: {
            let replayScore = selectContext.replayScoreForType(selectContext.current, root.lr2ReplayType);
            if (replayScore) {
                root.selectGoForward(selectContext.current, false, true, replayScore);
            }
            break;
        }
        case 57:
            root.setHiSpeedNumber(1, root.hiSpeedNumber(1) + delta * 10);
            optionChanged = true;
            break;
        case 58:
            root.setHiSpeedNumber(2, root.hiSpeedNumber(2) + delta * 10);
            optionChanged = true;
            break;
        case 40:
            root.setGaugeIndex(1, root.gaugeIndex(1) + delta);
            optionChanged = true;
            break;
        case 41:
            root.setGaugeIndex(2, root.gaugeIndex(2) + delta);
            optionChanged = true;
            break;
        case 42:
            root.setRandomIndex(1, root.randomIndex(1) + delta);
            optionChanged = true;
            break;
        case 43:
            root.setRandomIndex(2, root.randomIndex(2) + delta);
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
            root.setLaneCoverIndex(root.laneCoverIndex() + delta);
            optionChanged = true;
            break;
        case 50:
            root.setHidSudIndex(1, root.hidSudIndex(1) + delta);
            optionChanged = true;
            break;
        case 51:
            root.setHidSudIndex(2, root.hidSudIndex(2) + delta);
            optionChanged = true;
            break;
        case 54:
            root.setFlipIndex(root.flipIndex() + delta);
            optionChanged = true;
            break;
        case 55:
            root.setHiSpeedFixIndex(root.hiSpeedFixIndex() + delta);
            optionChanged = true;
            break;
        case 56:
            root.setBattleIndex(root.battleIndex() + delta);
            optionChanged = true;
            break;
        case 70:
        case 71:
            // Score graph and ghost options are not backed yet.
            break;
        case 72:
            root.setBgaIndex(root.bgaIndex() + delta);
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
            root.setTargetPercent(root.targetPercent() + delta);
            optionChanged = true;
            break;
        case 77:
            root.setScoreTargetIndex(root.scoreTargetIndex() + delta);
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
            selectContext.scrollBy(-steps, selectContext.lr2WheelDuration);
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

    function spriteStateOverride(src, dsts) {
        if (root.isSelectScrollSlider(src)) {
            return root.selectScrollSliderState(src, dsts);
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

    function selectScrollSliderState(src, dsts) {
        if (!root.isSelectScrollSlider(src)) {
            return null;
        }

        let base = Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
        if (!base) {
            return null;
        }

        let logicalCount = Math.max(1, selectContext.logicalCount);
        let maxFixed = Math.max(1, logicalCount * 1000 - 1);
        let fixedValue = Math.max(0, Math.min(maxFixed, selectContext.normalizedVisualIndex * 1000));
        let position = logicalCount > 1
            ? fixedValue / maxFixed
            : 0;
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

        let state = {
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
            filter: base.filter || 0
        };
        return state;
    }

    function lr2GenericSliderState(src, dsts) {
        if (!root.isLr2GenericSlider(src)) {
            return null;
        }

        let base = Lr2Timeline.getCurrentState(dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions);
        if (!base) {
            return null;
        }

        let position = root.sliderRawValue(src.sliderType) / 100;
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
            filter: base.filter || 0
        };
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
        if (!track) {
            return;
        }

        let position = root.sliderPositionFromPointer(src, track, pointerX, pointerY);
        let maxFixed = Math.max(0, selectContext.logicalCount * 1000 - 1);
        selectContext.setScrollFixedPoint(position * maxFixed, 100, false);
    }

    function setLr2GenericSliderFromPointer(src, dsts, pointerX, pointerY) {
        let track = root.lr2GenericSliderTrackState(src, dsts);
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
        return Lr2Timeline.getCurrentState(dstList, root.barSkinTime, root.timers, root.barActiveOptions);
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
            root.selectGoForward(selectContext.current);
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
    readonly property int selectAnimationLimit: Math.max(3200, skinModel.startInput)
    readonly property int selectInfoAnimationLimit: 1000
    readonly property int renderSkinTime: root.effectiveScreenKey === "select"
        ? Math.min(root.globalSkinTime, root.selectAnimationLimit)
        : root.globalSkinTime
    readonly property int barSkinTime: root.effectiveScreenKey === "select"
        ? Math.min(root.globalSkinTime, Math.max(2200, skinModel.startInput))
        : root.renderSkinTime
    readonly property bool shouldAutoAdvance: root.effectiveScreenKey === "decide"
        && !!root.chart
        && skinModel.sceneTime > 0

    function restartSkinClock() {
        root.sceneStartMs = Date.now();
        root.globalSkinTime = 0;
        root.selectHeldButtonSkinTime = 0;
        root.selectHeldButtonTimerStarts = ({});
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

    // Timer fire times (ms since scene start). LR2 select panels use timers
    // 21..26 for their side-drawer animations, so synthesize those while the
    // matching panel is open without unfreezing the whole select skin clock.
    readonly property var timers: {
        let result = { "0": 0 };
        if (root.effectiveScreenKey === "select") {
            // LR2 restarts timer 11 for the selected song information. The
            // select skin clock is intentionally capped after the intro, so
            // synthesize timer 11 from its own small stopwatch.
            result[11] = root.renderSkinTime - root.selectInfoElapsed;
        }
        if (root.effectiveScreenKey === "select" && root.selectPanel > 0) {
            result[20 + root.selectPanel] = root.renderSkinTime - root.selectPanelElapsed;
        }
        if (root.effectiveScreenKey === "select" && root.lr2ReadmeMode === 1) {
            result[15] = root.renderSkinTime - root.lr2ReadmeElapsed;
        } else if (root.effectiveScreenKey === "select" && root.lr2ReadmeMode === 2) {
            result[16] = root.renderSkinTime - root.lr2ReadmeElapsed;
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
    }

    Lr2SkinModel {
        id: skinModel
        csvPath: root.csvPath
        settingValues: root.skinSettings || {}
        activeOptions: root.parseActiveOptions

        onSkinLoaded: {
            Qt.callLater(root.restartSkinClock);
            Qt.callLater(root.openSelectIfNeeded);
        }
    }

    onCsvPathChanged: Qt.callLater(root.openSelectIfNeeded)
    onScreenKeyChanged: Qt.callLater(root.openSelectIfNeeded)

    Component.onCompleted: {
        root.selectSideEffectsReady = true;
        Qt.callLater(() => root.selectScratchSoundReady = true);
        Qt.callLater(root.restartSkinClock);
        Qt.callLater(root.openSelectIfNeeded);
        Qt.callLater(root.scheduleSelectSideEffects);
    }

    function playOneShot(player) {
        if (!player || !player.source) {
            return;
        }
        player.stop();
        player.play();
    }

    function selectGoBack() {
        let before = selectContext.historyStack.length;
        selectContext.goBack();
        if (selectContext.historyStack.length < before) {
            root.playOneShot(closeFolderSound);
        }
    }

    function selectGoForward(item, autoplay, replay, replayScore) {
        let before = selectContext.historyStack.length;
        selectContext.goForward(item, autoplay, replay, replayScore);
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

    function submitSelectSearch() {
        let query = selectContext.searchText.trim();
        if (query.length > 0) {
            selectContext.search(query);
        }
    }

    Keys.onUpPressed: (event) => {
        if (!root.selectScrollReady()) return;
        event.accepted = true;
        selectContext.decrementViewIndex(event.isAutoRepeat);
    }
    Keys.onDownPressed: (event) => {
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
        root.selectGoForward(selectContext.current);
    }
    Keys.onReturnPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        root.selectGoForward(selectContext.current);
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
    Input.onCol11Pressed: if (root.selectNavigationReady()) root.selectGoForward(selectContext.current)
    Input.onCol17Pressed: if (root.selectNavigationReady()) root.selectGoForward(selectContext.current)
    Input.onCol21Pressed: if (root.selectNavigationReady()) root.selectGoForward(selectContext.current)
    Input.onCol27Pressed: if (root.selectNavigationReady()) root.selectGoForward(selectContext.current)
    Input.onButtonPressed: (key) => {
        root.pressSelectHeldButtonTimer(key);
        if (root.handleSelectPanelKey(key)) {
            return;
        }
        if (root.selectNavigationReady() && (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26)) {
            root.selectGoBack();
        }
    }
    Input.onButtonReleased: (key) => root.releaseSelectHeldButtonTimer(key)

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.effectiveScreenKey === "select" ? root.activePreviewSource : ""
        onSourceChanged: {
            stop();
        }
    }

    AudioPlayer {
        id: selectBgm
        looping: true
        fadeInMillis: 1000
        source: root.mainGeneralVars() ? root.mainGeneralVars().bgmPath + "select" : ""
        property bool canPlay: root.enabled
            && root.effectiveScreenKey === "select"
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
        onTriggered: root.refreshDeferredSelectChart()
    }

    Timer {
        id: previewDelayTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (!root.enabled
                || root.effectiveScreenKey !== "select"
                || root.pendingPreviewRevision !== root.selectRevision) {
                return;
            }

            let source = root.pendingPreviewSource;
            let request = root.pendingPreviewRequest;
            root.activePreviewSource = source || "";
            if (source) {
                Qt.callLater(() => {
                    if (root.enabled
                        && root.effectiveScreenKey === "select"
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
                    // Match LR2/Vibes drawQueue order: earlier CSV elements
                    // are further back, later elements are painted on top.
                    z: root.elementZ(model.type, index, model.src, model.dsts)

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
                        }
                        return undefined;
                    }

                    Component {
                        id: imageComponent
                        Item {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property int spriteSkinClock: root.spriteSkinTime(model.dsts)

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: parent.spriteSkinClock
                                activeOptions: root.runtimeActiveOptions
                                timers: root.timers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                transColor: skinModel.transColor
                                frameOverride: root.buttonFrame(model.src)
                                stateOverride: root.spriteStateOverride(model.src, model.dsts)
                                forceHidden: root.spriteForceHidden(model.src, model.dsts)
                            }

                            readonly property var buttonState: model.src && model.src.button
                                ? Lr2Timeline.getCurrentState(model.dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions)
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
                        id: numberComponent
                        Lr2NumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            scaleOverride: skinScale
                            value: root.resolveNumber(model.src ? model.src.num : 0)
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
                                skinTime: root.renderSkinTime
                                activeOptions: root.runtimeActiveOptions
                                timers: root.timers
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
                            readonly property var readmeSrc: model.src
                            readonly property var readmeDsts: model.dsts

                            Repeater {
                                model: root.readmeLinesForSource(readmeTextDelegateRoot.readmeSrc)

                                Lr2TextRenderer {
                                    anchors.fill: parent
                                    dsts: readmeTextDelegateRoot.readmeDsts
                                    srcData: readmeTextDelegateRoot.readmeSrc
                                    skinTime: root.renderSkinTime
                                    activeOptions: root.runtimeActiveOptions
                                    timers: root.timers
                                    chart: root.renderChart
                                    scaleOverride: skinScale
                                    offsetY: index * (readmeTextDelegateRoot.readmeSrc ? readmeTextDelegateRoot.readmeSrc.readmeLineSpacing : 18)
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
                            timers: root.timers
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
                            timers: root.timers
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
                            timers: root.timers
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
                            skinTime: root.renderSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            value: root.resolveBarGraph(model.src ? model.src.graphType : 0)
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

            Repeater {
                model: root.effectiveScreenKey === "select" && skinModel.barRows ? skinModel.barRows.length : 0

                MouseArea {
                    id: barRowMouseArea
                    readonly property var rowState: root.barBaseState(index)
                    enabled: root.selectNavigationReady() && root.barRowCanClick(index) && !!rowState
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    x: rowState ? Math.min(rowState.x, rowState.x + rowState.w) * skinScale : 0
                    y: rowState ? Math.min(rowState.y, rowState.y + rowState.h) * skinScale : 0
                    width: rowState ? Math.abs(rowState.w) * skinScale : 0
                    height: rowState ? Math.abs(rowState.h) * skinScale : 0
                    z: 100000 + index
                    onPressed: (mouse) => root.updateSelectMouseFromArea(barRowMouseArea, mouse)
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            root.updateSelectMouseFromArea(barRowMouseArea, mouse);
                        }
                    }
                    onClicked: (mouse) => {
                        root.updateSelectMouseFromArea(barRowMouseArea, mouse);
                        root.handleBarRowClick(index, mouse);
                    }
                    onDoubleClicked: (mouse) => {
                        root.updateSelectMouseFromArea(barRowMouseArea, mouse);
                        if (mouse.button === Qt.LeftButton) {
                            selectContext.selectVisibleRow(index, skinModel.barCenter);
                            root.selectGoForward(selectContext.current);
                        }
                    }
                    onWheel: (wheel) => root.handleSelectWheel(wheel)
                }
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
                        if (selectScroll) {
                            root.setSelectScrollFromSliderPointer(model.src,
                                                                  model.dsts,
                                                                  (x + mouse.x) / skinScale,
                                                                  (y + mouse.y) / skinScale);
                        } else if (genericSlider) {
                            root.setLr2GenericSliderFromPointer(model.src,
                                                                model.dsts,
                                                                (x + mouse.x) / skinScale,
                                                                (y + mouse.y) / skinScale);
                        }
                    }
                    onPressed: (mouse) => {
                        root.updateSelectMouseFromArea(sliderMouseArea, mouse);
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
                        }
                    }
                    onCanceled: {
                        if (selectScroll) {
                            selectContext.finishScrollFixedPoint(100);
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
                enabled: root.selectScrollReady()
                acceptedButtons: Qt.NoButton
                z: 100200
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }
        }
    }
}
