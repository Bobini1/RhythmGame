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
        root.forceActiveFocus();
    }

    Shortcut {
        enabled: root.enabled && !root.selectSearchHasFocus()
        sequence: "Esc"

        onActivated: {
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
            options.push(900, 905, 910, 915); // stock LR2 select defaults
        } else if (root.effectiveScreenKey === "decide") {
            options.push(900); // stock LR2 decide default: show stagefile
        }
        return options;
    }
    property int selectPanel: 0
    property double selectPanelStartMs: 0
    property int selectPanelElapsed: 0
    readonly property int selectPanelHoldTime: 250

    onSelectPanelChanged: {
        if (root.selectPanel > 0) {
            root.clearSelectSearchFocus();
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
            && src
            && src.slider
            && src.sliderType === 1
            && src.sliderRange > 0
            && src.sliderDisabled === 0;
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

    function selectSearchHasFocus() {
        return selectSearchInput.activeFocus;
    }

    function selectNavigationReady() {
        return root.effectiveScreenKey === "select"
            && root.acceptsInput
            && !root.selectSearchHasFocus();
    }

    function focusSelectSearch() {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput || root.selectPanel > 0) {
            return;
        }
        if (selectSearchInput.text !== selectContext.searchText) {
            selectSearchInput.text = selectContext.searchText;
        }
        selectSearchInput.cursorPosition = selectSearchInput.text.length;
        selectSearchInput.forceActiveFocus();
    }

    function clearSelectSearchFocus() {
        if (selectSearchInput.activeFocus) {
            root.forceActiveFocus();
        }
    }

    function resetSelectSearch() {
        if (selectContext.searchText.length > 0) {
            selectContext.searchText = "";
            selectContext.touch();
        }
        if (selectSearchInput.text.length > 0) {
            selectSearchInput.text = "";
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
        if (root.effectiveScreenKey === "select"
            && type === 3
            && src
            && src.kind === 0
            && src.row > 0) {
            let bodyState = root.barBaseState(src.row);
            if (bodyState && bodyState.sortId !== undefined) {
                return bodyState.sortId + index * 0.000001;
            }
        }
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
        // Readtext is not implemented yet, so LR2's readtext button must stay
        // in the disabled option state even when the song folder has .txt files.
        addOption(options, 174);
        addOption(options, (chartData.maxBpm || 0) !== (chartData.minBpm || 0) ? 177 : 176);
        addOption(options, chartData.isRandom ? 179 : 178);
        addOption(options, selectContext.judgeOption(chartData));
        addOption(options, selectContext.highLevelOption(chartData));
        addOption(options, selectContext.hasReplay(chartData) ? 197 : 196);

        let difficulty = chartData.difficulty || 0;
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
        root.addOption(result, 50);  // offline
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

            for (let diff = 1; diff <= 5; ++diff) {
                if (selectContext.hasDifficulty(diff)) {
                    root.addOption(result, 504 + diff);
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
    property bool selectSideEffectsReady: false
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? root.deferredSelectChart
        : root.chart

    onSelectRevisionChanged: root.scheduleSelectSideEffects()
    onEffectiveScreenKeyChanged: root.scheduleSelectSideEffects()
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
            root.refreshDeferredSelectChart();
            playMusic.stop();
            return;
        }

        if (!root.deferredSelectChart) {
            root.refreshDeferredSelectChart();
        } else {
            selectChartSettleTimer.restart();
        }

        root.pendingPreviewRevision = root.selectRevision;
        root.activePreviewSource = "";
        playMusic.stop();
        previewDelayTimer.restart();
    }
    readonly property bool acceptsInput: root.effectiveScreenKey !== "select"
        || root.globalSkinTime >= skinModel.startInput

    function optionText(labels, index) {
        return index >= 0 && index < labels.length ? labels[index] : "";
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
            return chartData ? String(chartData.difficulty || "") : "";
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
            return chartData ? String(chartData.difficulty || "") : "";
        case 29:
            return chartData ? String(chartData.rank || "") : "";
        case 30:
            if (root.effectiveScreenKey !== "select") {
                return "";
            }
            return selectContext.searchText.length > 0
                ? selectContext.searchText
                : root.lr2SearchPlaceholderText;
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
                    || (src.buttonId >= 18 && src.buttonId <= 33)
                    || (src.buttonId >= 230 && src.buttonId <= 268)) {
                return 0;
            }
            return -1;
        }
    }

    function closeSelectPanel() {
        root.selectPanel = 0;
        root.selectPanelElapsed = 0;
    }

    function toggleSelectPanel(panel) {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel === panel) {
            root.closeSelectPanel();
            return;
        }
        root.selectPanel = panel;
        root.selectPanelStartMs = Date.now();
        root.selectPanelElapsed = 0;
    }

    function buttonPanelMatches(src) {
        if (!src || !src.button) {
            return false;
        }
        return root.panelMatches(src.buttonPanel || 0);
    }

    function handleLr2Button(buttonId, delta, panel) {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) {
            return;
        }
        root.resetSelectSearch();
        if (!root.selectNavigationReady()) {
            return;
        }
        if (buttonId >= 1 && buttonId <= 9) {
            root.toggleSelectPanel(buttonId);
            return;
        }
        if (buttonId === 200 && panel > 0) {
            root.closeSelectPanel();
            return;
        }
        if (buttonId >= 201 && buttonId <= 206 && panel > 0) {
            root.toggleSelectPanel(panel);
            return;
        }
        switch (buttonId) {
        case 10:
            selectContext.difficultyFilter = root.wrapValue(selectContext.difficultyFilter + delta, 6);
            selectContext.sortOrFilterChanged();
            break;
        case 11:
            selectContext.keyFilter = root.wrapValue(selectContext.keyFilter + delta, 7);
            selectContext.sortOrFilterChanged();
            break;
        case 12:
            selectContext.sortMode = root.wrapValue(selectContext.sortMode + delta, 5);
            selectContext.sortOrFilterChanged();
            break;
        case 15:
            selectContext.goForward(selectContext.current);
            break;
        case 16:
            selectContext.goForward(selectContext.current, true);
            break;
        case 19: {
            let replayScore = selectContext.replayScoreForType(selectContext.current, root.lr2ReplayType);
            if (replayScore) {
                selectContext.goForward(selectContext.current, false, true, replayScore);
            }
            break;
        }
        case 57:
            root.setHiSpeedNumber(1, root.hiSpeedNumber(1) + delta);
            break;
        case 58:
            root.setHiSpeedNumber(2, root.hiSpeedNumber(2) + delta);
            break;
        case 40:
            root.setGaugeIndex(1, root.gaugeIndex(1) + delta);
            break;
        case 41:
            root.setGaugeIndex(2, root.gaugeIndex(2) + delta);
            break;
        case 42:
            root.setRandomIndex(1, root.randomIndex(1) + delta);
            break;
        case 43:
            root.setRandomIndex(2, root.randomIndex(2) + delta);
            break;
        case 44:
        case 45:
            // Assist options are intentionally unsupported; keep LR2 menus at OFF.
            break;
        case 46:
            root.setLaneCoverIndex(root.laneCoverIndex() + delta);
            break;
        case 50:
            root.setHidSudIndex(1, root.hidSudIndex(1) + delta);
            break;
        case 51:
            root.setHidSudIndex(2, root.hidSudIndex(2) + delta);
            break;
        case 54:
            root.setFlipIndex(root.flipIndex() + delta);
            break;
        case 55:
            root.setHiSpeedFixIndex(root.hiSpeedFixIndex() + delta);
            break;
        case 56:
            root.setBattleIndex(root.battleIndex() + delta);
            break;
        case 70:
        case 71:
            // Score graph and ghost options are not backed yet.
            break;
        case 72:
            root.setBgaIndex(root.bgaIndex() + delta);
            break;
        case 73:
        case 75:
            // BGA size and judge auto-adjust are not selectable from LR2 skins yet.
            break;
        case 74:
            root.adjustOffset(delta);
            break;
        case 76:
            root.setTargetPercent(root.targetPercent() + delta);
            break;
        case 77:
            root.setScoreTargetIndex(root.scoreTargetIndex() + delta);
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
            break;
        default:
            if (buttonId >= 91 && buttonId <= 96) {
                selectContext.difficultyFilter = buttonId - 91;
                selectContext.sortOrFilterChanged();
                break;
            }
            if ((buttonId >= 13 && buttonId <= 14)
                    || (buttonId >= 18 && buttonId <= 33)
                    || (buttonId >= 230 && buttonId <= 268)) {
                // These LR2 controls cover skin/key config, audio EQ/FX, and
                // tag-editor actions that do not have safe backing state here.
                break;
            }
            console.info("LR2 select button " + buttonId + " is not implemented yet");
            break;
        }
    }

    property real selectWheelRemainder: 0

    function handleSelectWheel(wheel) {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) {
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

    function selectScrollSliderTrackState(src, dsts) {
        if (!root.isSelectScrollSlider(src)) {
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

    function setSelectScrollFromSliderPointer(src, dsts, pointerX, pointerY) {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0) {
            return;
        }
        let track = root.selectScrollSliderTrackState(src, dsts);
        if (!track) {
            return;
        }

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
            return;
        }
        position = Math.max(0, Math.min(1, position));
        let maxFixed = Math.max(0, selectContext.logicalCount * 1000 - 1);
        selectContext.setScrollFixedPoint(position * maxFixed, 100, false);
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
            selectContext.goBack();
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
            selectContext.goForward(selectContext.current);
            return;
        }

        selectContext.selectVisibleRow(row, skinModel.barCenter);
    }

    // Monotonic scene clock. Reset after the parsed model is ready so skin
    // loading does not eat the first second or two of LR2's decide animation.
    property double sceneStartMs: Date.now()
    property int globalSkinTime: 0
    readonly property int selectAnimationLimit: Math.max(3200, skinModel.startInput)
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
        if (root.shouldAutoAdvance) {
            sceneEndTimer.restart();
        } else {
            sceneEndTimer.stop();
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
            // LR2 restarts timer 11 for the selected song information. We keep
            // it fired so the title/BPM/difficulty detail layer is visible.
            result[11] = 0;
        }
        if (root.effectiveScreenKey === "select" && root.selectPanel > 0) {
            result[20 + root.selectPanel] = root.renderSkinTime - root.selectPanelElapsed;
        }
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
        Qt.callLater(root.restartSkinClock);
        Qt.callLater(root.openSelectIfNeeded);
        Qt.callLater(root.scheduleSelectSideEffects);
    }

    TextInput {
        id: selectSearchInput
        x: -1
        y: -1
        width: 1
        height: 1
        opacity: 0
        activeFocusOnTab: false
        inputMethodHints: Qt.ImhNoPredictiveText

        onTextEdited: {
            if (selectContext.searchText !== text) {
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
                if (selectSearchInput.text !== selectContext.searchText) {
                    selectSearchInput.text = selectContext.searchText;
                    selectSearchInput.cursorPosition = selectSearchInput.text.length;
                }
            }
        }
    }

    function submitSelectSearch() {
        let query = selectContext.searchText.trim();
        if (query.length > 0) {
            selectContext.search(query);
        }
    }

    Keys.onUpPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        selectContext.decrementViewIndex(event.isAutoRepeat);
    }
    Keys.onDownPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        selectContext.incrementViewIndex(event.isAutoRepeat);
    }
    Keys.onLeftPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        selectContext.goBack();
    }
    Keys.onRightPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        selectContext.goForward(selectContext.current);
    }
    Keys.onReturnPressed: (event) => {
        if (!root.selectNavigationReady()) return;
        event.accepted = true;
        selectContext.goForward(selectContext.current);
    }

    property var lastNavigateKey: []

    function navigate(number, type, up, key) {
        if (!root.selectNavigationReady() || root.lastNavigateKey[root.lastNavigateKey.length - 1] !== key) {
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
    Input.onCol1sDownPressed: if (root.selectNavigationReady()) root.lastNavigateKey.push(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: if (root.selectNavigationReady()) root.lastNavigateKey.push(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: if (root.selectNavigationReady()) root.lastNavigateKey.push(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: if (root.selectNavigationReady()) root.lastNavigateKey.push(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sDown)
    Input.onCol1sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sUp)
    Input.onCol2sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sDown)
    Input.onCol2sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sUp)
    Input.onCol11Pressed: if (root.selectNavigationReady()) selectContext.goForward(selectContext.current)
    Input.onCol17Pressed: if (root.selectNavigationReady()) selectContext.goForward(selectContext.current)
    Input.onCol21Pressed: if (root.selectNavigationReady()) selectContext.goForward(selectContext.current)
    Input.onCol27Pressed: if (root.selectNavigationReady()) selectContext.goForward(selectContext.current)
    Input.onButtonPressed: (key) => {
        if (root.selectNavigationReady() && (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26)) {
            selectContext.goBack();
        }
    }

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.effectiveScreenKey === "select" ? root.activePreviewSource : ""
        onSourceChanged: {
            stop();
        }
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

            let source = selectContext.selectedPreviewSource();
            root.activePreviewSource = source || "";
            if (source) {
                Qt.callLater(() => {
                    if (root.enabled
                        && root.effectiveScreenKey === "select"
                        && root.activePreviewSource === source
                        && root.pendingPreviewRevision === root.selectRevision) {
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
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select" && root.acceptsInput
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                z: -100000
                onClicked: root.resetSelectSearch()
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
                            return imageComponent;
                        } else if (model.type === 1) {
                            return numberComponent;
                        } else if (model.type === 2) {
                            return textComponent;
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

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: root.renderSkinTime
                                activeOptions: root.runtimeActiveOptions
                                timers: root.timers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                frameOverride: root.buttonFrame(model.src)
                                stateOverride: root.selectScrollSliderState(model.src, model.dsts)
                            }

                            readonly property var buttonState: model.src && model.src.button
                                ? Lr2Timeline.getCurrentState(model.dsts, root.renderSkinTime, root.timers, root.runtimeActiveOptions)
                                : null

                            MouseArea {
                                enabled: root.effectiveScreenKey === "select"
                                    && root.acceptsInput
                                    && model.src
                                    && model.src.button
                                    && model.src.buttonClick !== 0
                                    && root.buttonPanelMatches(model.src)
                                    && !!parent.buttonState
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                x: parent.buttonState ? Math.min(parent.buttonState.x, parent.buttonState.x + parent.buttonState.w) * skinScale : 0
                                y: parent.buttonState ? Math.min(parent.buttonState.y, parent.buttonState.y + parent.buttonState.h) * skinScale : 0
                                width: parent.buttonState ? Math.abs(parent.buttonState.w) * skinScale : 0
                                height: parent.buttonState ? Math.abs(parent.buttonState.h) * skinScale : 0
                                onClicked: (mouse) => {
                                    let delta = mouse.button === Qt.RightButton ? -root.buttonDelta(model.src) : root.buttonDelta(model.src);
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
                            width: skinW * skinScale
                            height: skinH * skinScale

                            Lr2TextRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: root.renderSkinTime
                                activeOptions: root.runtimeActiveOptions
                                timers: root.timers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                resolvedText: root.resolveText(model.src ? model.src.st : -1)
                            }

                            readonly property var searchTextState: root.selectSearchTextState(model.src, model.dsts)

                            MouseArea {
                                enabled: !!parent.searchTextState
                                acceptedButtons: Qt.LeftButton
                                x: parent.searchTextState ? Math.min(parent.searchTextState.x, parent.searchTextState.x + parent.searchTextState.w) * skinScale : 0
                                y: parent.searchTextState ? Math.min(parent.searchTextState.y, parent.searchTextState.y + parent.searchTextState.h) * skinScale : 0
                                width: parent.searchTextState ? Math.abs(parent.searchTextState.w) * skinScale : 0
                                height: parent.searchTextState ? Math.abs(parent.searchTextState.h) * skinScale : 0
                                onClicked: (mouse) => {
                                    mouse.accepted = true;
                                    root.focusSelectSearch();
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

            Repeater {
                model: root.effectiveScreenKey === "select" && skinModel.barRows ? skinModel.barRows.length : 0

                MouseArea {
                    readonly property var rowState: root.barBaseState(index)
                    enabled: root.acceptsInput && root.barRowCanClick(index) && !!rowState
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    x: rowState ? Math.min(rowState.x, rowState.x + rowState.w) * skinScale : 0
                    y: rowState ? Math.min(rowState.y, rowState.y + rowState.h) * skinScale : 0
                    width: rowState ? Math.abs(rowState.w) * skinScale : 0
                    height: rowState ? Math.abs(rowState.h) * skinScale : 0
                    z: 100000 + index
                    onClicked: (mouse) => {
                        root.handleBarRowClick(index, mouse);
                    }
                    onDoubleClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            selectContext.selectVisibleRow(index, skinModel.barCenter);
                            selectContext.goForward(selectContext.current);
                        }
                    }
                    onWheel: (wheel) => root.handleSelectWheel(wheel)
                }
            }

            Repeater {
                model: skinModel

                MouseArea {
                    readonly property var trackState: root.selectScrollSliderTrackState(model.src, model.dsts)
                    enabled: root.acceptsInput && !!trackState
                    acceptedButtons: Qt.LeftButton
                    preventStealing: true
                    x: trackState ? Math.min(trackState.x, trackState.x + trackState.w) * skinScale : 0
                    y: trackState ? Math.min(trackState.y, trackState.y + trackState.h) * skinScale : 0
                    width: trackState ? Math.abs(trackState.w) * skinScale : 0
                    height: trackState ? Math.abs(trackState.h) * skinScale : 0
                    z: 100300 + index
                    onPressed: (mouse) => {
                        root.setSelectScrollFromSliderPointer(model.src,
                                                              model.dsts,
                                                              (x + mouse.x) / skinScale,
                                                              (y + mouse.y) / skinScale);
                        mouse.accepted = true;
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            root.setSelectScrollFromSliderPointer(model.src,
                                                                  model.dsts,
                                                                  (x + mouse.x) / skinScale,
                                                                  (y + mouse.y) / skinScale);
                        }
                    }
                    onReleased: {
                        selectContext.finishScrollFixedPoint(100);
                    }
                    onCanceled: {
                        selectContext.finishScrollFixedPoint(100);
                    }
                    onWheel: (wheel) => root.handleSelectWheel(wheel)
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select" && root.acceptsInput
                acceptedButtons: Qt.NoButton
                z: 100200
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }
        }
    }
}
