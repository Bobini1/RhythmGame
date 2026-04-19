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
        root.forceActiveFocus();
    }

    Shortcut {
        enabled: root.enabled
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

    property int lr2Gauge1: 0
    property int lr2Gauge2: 0
    property int lr2Random1: 0
    property int lr2Random2: 0
    property int lr2Assist1: 0
    property int lr2Assist2: 0
    property int lr2HidSud1: 0
    property int lr2HidSud2: 0
    property int lr2Battle: 0
    property int lr2Flip: 0
    property int lr2HsFix: 0
    property int lr2ScoreGraph: 0
    property int lr2Ghost: 0
    property int lr2LaneCover: 0
    property int lr2Bga: 0
    property int lr2BgaSize: 0
    property int lr2AutoJudge: 0
    property int lr2ScreenMode: 0
    property int lr2HighColor: 1
    property int lr2Vsync: 0
    property int lr2Replay: 0
    property int lr2TargetPercent: 80

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

    function elementZ(type, index) {
        // LR2 renders the song bar list through select-scene drawing code,
        // not as ordinary static CSV sprites. Keep those generated bar items
        // above later decorative images so the wheel is not buried.
        if (root.effectiveScreenKey === "select" && type >= 3 && type <= 5) {
            return 40000 + index;
        }
        return index;
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
            return;
        }
        addOption(options, chartData.stageFile ? 191 : 190);
        addOption(options, chartData.banner ? 193 : 192);
        addOption(options, chartData.backBmp ? 195 : 194);

        let difficulty = chartData.difficulty || 0;
        if (difficulty >= 1 && difficulty <= 5) {
            addOption(options, 150 + difficulty);
        }

        let keymode = chartData.keymode || 0;
        if (keymode === 7) addOption(options, 160);
        else if (keymode === 5) addOption(options, 161);
        else if (keymode === 14) addOption(options, 162);
        else if (keymode === 10) addOption(options, 163);
        else if (keymode === 9) addOption(options, 164);
    }

    readonly property var runtimeActiveOptions: {
        let result = [];
        let revision = selectContext.revision;
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
            }
            root.appendChartOptions(result, selectContext.selectedChartData());
        } else {
            root.appendChartOptions(result, root.chart && root.chart.chartData ? root.chart.chartData : null);
        }

        return result;
    }

    readonly property int selectRevision: selectContext.revision
    readonly property var renderChart: root.effectiveScreenKey === "select"
        ? (root.selectRevision, selectContext.selectedChartWrapper())
        : root.chart
    readonly property bool acceptsInput: root.effectiveScreenKey !== "select"
        || root.globalSkinTime >= skinModel.startInput

    function optionText(labels, index) {
        return index >= 0 && index < labels.length ? labels[index] : "";
    }

    function lr2SelectOptionText(st) {
        switch (st) {
        case 63:
            return root.optionText(["OFF", "MIRROR", "RANDOM", "S-RANDOM", "H-RANDOM", "ALL-SCR"], root.lr2Random1);
        case 64:
            return root.optionText(["OFF", "MIRROR", "RANDOM", "S-RANDOM", "H-RANDOM", "ALL-SCR"], root.lr2Random2);
        case 65:
            return root.optionText(["GROOVE", "SURVIVAL", "DEATH", "EASY", "P-ATTACK", "G-ATTACK"], root.lr2Gauge1);
        case 66:
            return root.optionText(["GROOVE", "SURVIVAL", "DEATH", "EASY", "P-ATTACK", "G-ATTACK"], root.lr2Gauge2);
        case 67:
            return root.optionText(["OFF", "AUTOSCR", "LEGACY", "ASSIST"], root.lr2Assist1);
        case 68:
            return root.optionText(["OFF", "AUTOSCR", "LEGACY", "ASSIST"], root.lr2Assist2);
        case 69:
            return root.optionText(["OFF", "BATTLE", "DBATTLE", "SP TO DP", "G-BATTLE", "DP", "POP'N", "DBATTLE DP", "DBATTLE 9"], root.lr2Battle);
        case 70:
            return root.optionText(["OFF", "FLIP"], root.lr2Flip);
        case 71:
            return root.optionText(["OFF", "ON"], root.lr2ScoreGraph);
        case 72:
            return root.optionText(["OFF", "TYPE-A", "TYPE-B", "TYPE-C"], root.lr2Ghost);
        case 73:
            return root.optionText(["OFF", "ON", "SUDDEN", "HIDDEN"], root.lr2LaneCover);
        case 74:
            return root.optionText(["OFF", "MAXBPM", "MINBPM", "AVERAGE", "CONSTANT"], root.lr2HsFix);
        case 75:
            return root.optionText(["NORMAL", "EXTEND"], root.lr2BgaSize);
        case 76:
            return root.optionText(["ON", "OFF", "AUTO"], root.lr2Bga);
        case 77:
            return root.optionText(["16BIT", "32BIT"], root.lr2HighColor);
        case 78:
            return root.optionText(["OFF", "ON"], root.lr2Vsync);
        case 79:
            return root.optionText(["WINDOW", "FULL"], root.lr2ScreenMode);
        case 80:
            return root.optionText(["OFF", "ON", "ALWAYS"], root.lr2AutoJudge);
        case 81:
            return root.optionText(["OFF", "ON", "BEST", "ALL", "AUTO"], root.lr2Replay);
        case 82:
            return "MISSION COMPLETED";
        case 83:
            return "";
        case 84:
            return root.optionText(["OFF", "HIDDEN", "SUDDEN", "HID+SUD"], root.lr2HidSud1);
        case 85:
            return root.optionText(["OFF", "HIDDEN", "SUDDEN", "HID+SUD"], root.lr2HidSud2);
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
        let revision = root.effectiveScreenKey === "select" ? selectContext.revision : 0;
        let chartData = root.effectiveScreenKey === "select"
            ? selectContext.selectedChartData()
            : (root.chart ? root.chart.chartData : null);
        let currentEntry = root.effectiveScreenKey === "select" ? selectContext.current : null;
        switch (st) {
        case 1:
            return "NO TARGET";
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
        case 21:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
            return root.effectiveScreenKey === "select" ? selectContext.entryDisplayName(currentEntry, true) : "";
        case 30:
            return "";
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
            if (num === 13) {
                return root.lr2TargetPercent;
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
            return selectContext.keyFilter;
        case 12:
            return selectContext.sortMode;
        case 40:
            return root.lr2Gauge1;
        case 41:
            return root.lr2Gauge2;
        case 42:
            return root.lr2Random1;
        case 43:
            return root.lr2Random2;
        case 44:
            return root.lr2Assist1;
        case 45:
            return root.lr2Assist2;
        case 46:
            return root.lr2LaneCover;
        case 50:
            return root.lr2HidSud1;
        case 51:
            return root.lr2HidSud2;
        case 54:
            return root.lr2Flip;
        case 55:
            return root.lr2HsFix;
        case 56:
            return root.lr2Battle;
        case 70:
            return root.lr2ScoreGraph;
        case 71:
            return root.lr2Ghost;
        case 72:
            return root.lr2Bga;
        case 73:
            return root.lr2BgaSize;
        case 75:
            return root.lr2AutoJudge;
        case 80:
            return root.lr2ScreenMode;
        case 81:
            return root.lr2HighColor;
        case 82:
            return root.lr2Vsync;
        case 83:
            return root.lr2Replay;
        default:
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
        let panel = src.buttonPanel || 0;
        if (panel > 0) {
            return root.selectPanel === panel;
        }
        if (panel < 0) {
            return root.selectPanel === 0;
        }
        return true;
    }

    function handleLr2Button(buttonId, delta, panel) {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) {
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
            selectContext.keyFilter = root.wrapValue(selectContext.keyFilter + delta, 8);
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
        case 19:
            selectContext.goForward(selectContext.current);
            break;
        case 40:
            root.lr2Gauge1 = root.wrapValue(root.lr2Gauge1 + delta, 6);
            break;
        case 41:
            root.lr2Gauge2 = root.wrapValue(root.lr2Gauge2 + delta, 6);
            break;
        case 42:
            root.lr2Random1 = root.wrapValue(root.lr2Random1 + delta, 6);
            break;
        case 43:
            root.lr2Random2 = root.wrapValue(root.lr2Random2 + delta, 6);
            break;
        case 44:
            root.lr2Assist1 = root.wrapValue(root.lr2Assist1 + delta, 4);
            break;
        case 45:
            root.lr2Assist2 = root.wrapValue(root.lr2Assist2 + delta, 4);
            break;
        case 46:
            root.lr2LaneCover = root.wrapValue(root.lr2LaneCover + delta, 2);
            break;
        case 50:
            root.lr2HidSud1 = root.wrapValue(root.lr2HidSud1 + delta, 4);
            break;
        case 51:
            root.lr2HidSud2 = root.wrapValue(root.lr2HidSud2 + delta, 4);
            break;
        case 54:
            root.lr2Flip = root.wrapValue(root.lr2Flip + delta, 2);
            break;
        case 55:
            root.lr2HsFix = root.wrapValue(root.lr2HsFix + delta, 5);
            break;
        case 56:
            root.lr2Battle = root.wrapValue(root.lr2Battle + delta, 9);
            break;
        case 70:
            root.lr2ScoreGraph = root.wrapValue(root.lr2ScoreGraph + delta, 2);
            break;
        case 71:
            root.lr2Ghost = root.wrapValue(root.lr2Ghost + delta, 4);
            break;
        case 72:
            root.lr2Bga = root.wrapValue(root.lr2Bga + delta, 3);
            break;
        case 73:
            root.lr2BgaSize = root.wrapValue(root.lr2BgaSize + delta, 2);
            break;
        case 75:
            root.lr2AutoJudge = root.wrapValue(root.lr2AutoJudge + delta, 3);
            break;
        case 76:
            root.lr2TargetPercent = Math.max(50, Math.min(100, root.lr2TargetPercent + delta));
            break;
        case 77:
            break;
        case 80:
            root.lr2ScreenMode = root.wrapValue(root.lr2ScreenMode + delta, 2);
            break;
        case 81:
            root.lr2HighColor = root.wrapValue(root.lr2HighColor + delta, 2);
            break;
        case 82:
            root.lr2Vsync = root.wrapValue(root.lr2Vsync + delta, 2);
            break;
        case 83:
            root.lr2Replay = root.wrapValue(root.lr2Replay + delta, 5);
            break;
        default:
            if (buttonId >= 91 && buttonId <= 96) {
                selectContext.difficultyFilter = buttonId - 91;
                selectContext.sortOrFilterChanged();
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

    function barBaseState(row) {
        let rows = skinModel.barRows;
        if (!rows || row < 0 || row >= rows.length) {
            return null;
        }
        let data = rows[row];
        let useOn = row === root.selectedBarRow() && data.onDsts && data.onDsts.length > 0;
        let dstList = useOn ? data.onDsts : data.offDsts;
        if (!dstList || dstList.length === 0) {
            dstList = data.onDsts || [];
        }
        return Lr2Timeline.getCurrentState(dstList, root.barSkinTime, root.timers, root.runtimeActiveOptions);
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
        Qt.callLater(root.restartSkinClock);
        Qt.callLater(root.openSelectIfNeeded);
    }

    Keys.onUpPressed: (event) => {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) return;
        event.accepted = true;
        selectContext.decrementViewIndex(event.isAutoRepeat);
    }
    Keys.onDownPressed: (event) => {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) return;
        event.accepted = true;
        selectContext.incrementViewIndex(event.isAutoRepeat);
    }
    Keys.onLeftPressed: (event) => {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) return;
        event.accepted = true;
        selectContext.goBack();
    }
    Keys.onRightPressed: (event) => {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) return;
        event.accepted = true;
        selectContext.goForward(selectContext.current);
    }
    Keys.onReturnPressed: (event) => {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput) return;
        event.accepted = true;
        selectContext.goForward(selectContext.current);
    }

    property var lastNavigateKey: []

    function navigate(number, type, up, key) {
        if (root.effectiveScreenKey !== "select" || !root.acceptsInput || root.lastNavigateKey[root.lastNavigateKey.length - 1] !== key) {
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
    Input.onCol1sDownPressed: if (root.effectiveScreenKey === "select") root.lastNavigateKey.push(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: if (root.effectiveScreenKey === "select") root.lastNavigateKey.push(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: if (root.effectiveScreenKey === "select") root.lastNavigateKey.push(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: if (root.effectiveScreenKey === "select") root.lastNavigateKey.push(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sDown)
    Input.onCol1sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col1sUp)
    Input.onCol2sDownReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sDown)
    Input.onCol2sUpReleased: root.lastNavigateKey = root.lastNavigateKey.filter(k => k !== BmsKey.Col2sUp)
    Input.onCol11Pressed: if (root.effectiveScreenKey === "select" && root.acceptsInput) selectContext.goForward(selectContext.current)
    Input.onCol17Pressed: if (root.effectiveScreenKey === "select" && root.acceptsInput) selectContext.goForward(selectContext.current)
    Input.onCol21Pressed: if (root.effectiveScreenKey === "select" && root.acceptsInput) selectContext.goForward(selectContext.current)
    Input.onCol27Pressed: if (root.effectiveScreenKey === "select" && root.acceptsInput) selectContext.goForward(selectContext.current)
    Input.onButtonPressed: (key) => {
        if (root.effectiveScreenKey === "select" && root.acceptsInput && (key === BmsKey.Col12 || key === BmsKey.Col14 || key === BmsKey.Col16 || key === BmsKey.Col22 || key === BmsKey.Col24 || key === BmsKey.Col26)) {
            selectContext.goBack();
        }
    }

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.effectiveScreenKey === "select" ? selectContext.selectedPreviewSource() : ""
        onSourceChanged: {
            stop();
            if (source) {
                previewDelayTimer.restart();
            } else {
                previewDelayTimer.stop();
            }
        }
    }

    Timer {
        id: previewDelayTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (root.enabled && root.effectiveScreenKey === "select" && playMusic.source) {
                playMusic.play();
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
                    z: root.elementZ(model.type, index)

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
                        Lr2TextRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            resolvedText: root.resolveText(model.src ? model.src.st : -1)
                        }
                    }

                    Component {
                        id: barImageComponent
                        Lr2BarSpriteRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barTextComponent
                        Lr2BarTextRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barNumberComponent
                        Lr2BarNumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.runtimeActiveOptions
                            timers: root.timers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
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
