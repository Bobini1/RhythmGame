pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: controller

    required property var screenRoot
    required property var selectContext
    property var scratchSound
    property var optionOpenSound
    property var optionCloseSound
    property var optionChangeSound

    readonly property var root: screenRoot
    property int selectPanel: 0
    property int selectPanelHeldByStart: 0
    property int selectPanelStartSkinTime: 0
    property int selectPanelClosing: 0
    property int selectPanelCloseStartSkinTime: 0
    readonly property int selectPanelHoldTime: 250
    readonly property int selectPanelElapsed: controller.selectPanel > 0
        ? Math.max(0, Math.min(controller.selectPanelHoldTime, root.selectLiveSkinTime - controller.selectPanelStartSkinTime))
        : 0
    property int selectPanelCloseElapsed: 0
    property bool selectPanelCloseFinished: false
    property int selectHeldButtonSkinTime: 0
    property var selectHeldButtonTimerStarts: ({})
    readonly property bool hasSelectHeldButtonTimers: Object.keys(controller.selectHeldButtonTimerStarts).length > 0
    readonly property int selectTargetScratchInitialRepeatMillis: 300
    readonly property int selectTargetScratchRepeatMillis: 50
    property int selectTargetScratchDirection: 0
    property real selectTargetScratchNextMs: 0
    readonly property int gameplayOptionInitialRepeatMillis: 300
    readonly property int gameplayOptionRepeatMillis: 50
    readonly property int gameplayLaneCoverDoublePressMillis: 500
    property int gameplayOptionRepeatKey: -1
    property bool gameplayOptionRepeating: false
    property var gameplayLastStartPressMs: ({})
    property var gameplayScratchLastDirectionUp: ({ "1": false, "2": false })
    readonly property bool anyStartHeld: Input.start1 || Input.start2
    readonly property bool anySelectHeld: Input.select1 || Input.select2
    readonly property int heldOptionPanel: controller.anyStartHeld && controller.anySelectHeld ? 3
        : controller.anyStartHeld ? 1
        : controller.anySelectHeld ? 2
        : 0
    property bool startHoldSuppressed: false

    property Timer selectPanelCloseTimer: Timer {
        id: selectPanelCloseTimer
        interval: controller.selectPanelHoldTime
        repeat: false
        onTriggered: controller.selectPanelClosing = 0
    }

    property Timer gameplayOptionRepeatTimer: Timer {
        id: gameplayOptionRepeatTimer
        repeat: false
        onTriggered: controller.repeatGameplayOptionKey()
    }

    onSelectPanelChanged: {
        if (controller.selectPanel > 0) {
            root.clearSelectSearchFocus();
        } else {
            root.resetLr2SelectScratchRepeat();
        }
    }

    onSelectPanelClosingChanged: {
        if (controller.selectPanelClosing <= 0) {
            selectPanelCloseTimer.stop();
        }
        controller.updateSelectPanelCloseProgress();
    }

    onSelectPanelCloseStartSkinTimeChanged: {
        controller.updateSelectPanelCloseProgress();
    }

    onSelectPanelElapsedChanged: {
        if (root.selectHoverHasPoint && controller.selectPanel > 0) {
            root.refreshSelectHoverState();
        }
    }

    onSelectPanelCloseElapsedChanged: {
        if (root.selectHoverHasPoint && controller.selectPanelClosing > 0) {
            root.refreshSelectHoverState();
        }
    }

    onHeldOptionPanelChanged: controller.updateHeldOptionPanel()
    onAnyStartHeldChanged: controller.handleGameplayOptionModifierChanged()
    onAnySelectHeldChanged: controller.handleGameplayOptionModifierChanged()
    onHasSelectHeldButtonTimersChanged: controller.updateSelectHeldButtonSkinTime()

    property Connections selectPanelClockConnection: Connections {
        target: controller.root || null
        enabled: !!controller.root
            && (controller.selectPanel > 0
                || controller.selectPanelClosing > 0
                || controller.hasSelectHeldButtonTimers)

        function onSelectLiveSkinTimeChanged() : void {
            controller.updateSelectPanelCloseProgress();
            if (controller.hasSelectHeldButtonTimers) {
                controller.updateSelectHeldButtonSkinTime();
            }
        }
    }

    function updateSelectHeldButtonSkinTime() : void {
        let next = controller.hasSelectHeldButtonTimers ? controller.currentSelectHeldButtonSkinTime() : 0;
        if (controller.selectHeldButtonSkinTime !== next) {
            controller.selectHeldButtonSkinTime = next;
        }
    }

    function updateSelectPanelCloseProgress() : void {
        let elapsed = 0;
        if (controller.selectPanelClosing > 0) {
            elapsed = Math.max(0, Math.min(
                controller.selectPanelHoldTime,
                root.selectLiveSkinTime - controller.selectPanelCloseStartSkinTime));
        }

        let finished = controller.selectPanelClosing > 0
            && elapsed >= controller.selectPanelHoldTime;
        if (controller.selectPanelCloseElapsed !== elapsed) {
            controller.selectPanelCloseElapsed = elapsed;
        }
        if (controller.selectPanelCloseFinished !== finished) {
            controller.selectPanelCloseFinished = finished;
        }
    }

    function updateHeldOptionPanel() : var {
        if (controller.heldOptionPanel > 0) {
            if (controller.selectPanel > 0 && controller.selectPanelHeldByStart === 0) {
                controller.startHoldSuppressed = true;
                controller.closeSelectPanel();
                return;
            }
            if (!controller.startHoldSuppressed) {
                controller.holdSelectPanel(controller.heldOptionPanel);
            }
        } else {
            controller.startHoldSuppressed = false;
            if (controller.selectPanelHeldByStart > 0) {
                controller.closeSelectPanel();
            }
        }
    }

    function handleGameplayOptionModifierChanged() : var {
        if (!root.isGameplayScreen()) {
            return;
        }
        root.refreshGameplayRuntimeActiveOptions();
        root.bumpGameplayRevision();
        if (controller.gameplayOptionRepeating
                && !root.gameplayOptionModifierHeldForKey(controller.gameplayOptionRepeatKey)) {
            controller.stopLr2GameplayOptionRepeat();
        }
    }

    readonly property var splitArrowButtons: controller.lookup([
        10, 11, 12, 20, 21, 22, 26, 27, 28, 33,
        40, 41, 42, 43, 46, 50, 51, 54, 55, 56,
        57, 58, 59, 72, 74, 76, 77, 78, 83, 190, 308
    ])
    readonly property var imageSetButtons: controller.lookup([
        40, 41, 42, 43, 54, 55, 72, 74, 77, 78,
        301, 302, 303, 304, 305, 306, 307, 308
    ])
    readonly property var fixedZeroButtonFrames: controller.lookup([
        13, 14, 18, 44, 45, 74, 75, 80, 81, 82, 83,
        210, 301, 302, 303, 304, 305, 306, 307
    ])
    readonly property var player2Keys: controller.lookup([
        BmsKey.Col21, BmsKey.Col22, BmsKey.Col23, BmsKey.Col24,
        BmsKey.Col25, BmsKey.Col26, BmsKey.Col27,
        BmsKey.Col2sUp, BmsKey.Col2sDown, BmsKey.Start2, BmsKey.Select2
    ])
    readonly property var selectPanelKeyBindings: ({
        "1": controller.bindingLookup([
            [BmsKey.Col11, 42, 1], [BmsKey.Col21, 42, 1],
            [BmsKey.Col12, 42, -1], [BmsKey.Col22, 42, -1],
            [BmsKey.Col13, 40, 1], [BmsKey.Col23, 40, 1],
            [BmsKey.Col14, 54, 1], [BmsKey.Col24, 54, 1],
            [BmsKey.Col15, 55, 1], [BmsKey.Col25, 55, 1],
            [BmsKey.Col16, 43, -1], [BmsKey.Col26, 43, -1],
            [BmsKey.Col17, 43, 1], [BmsKey.Col27, 43, 1]
        ]),
        "2": controller.bindingLookup([
            [BmsKey.Col11, 71, 1], [BmsKey.Col21, 71, 1],
            [BmsKey.Col12, 70, 1], [BmsKey.Col22, 70, 1],
            [BmsKey.Col13, 76, 1], [BmsKey.Col23, 76, 1],
            [BmsKey.Col14, 72, 1], [BmsKey.Col24, 72, 1],
            [BmsKey.Col15, 73, 1], [BmsKey.Col25, 73, 1],
            [BmsKey.Col16, 74, 1], [BmsKey.Col26, 74, 1],
            [BmsKey.Col17, 75, 1], [BmsKey.Col27, 75, 1],
            [BmsKey.Select1, 83, 1], [BmsKey.Select2, 83, 1]
        ]),
        "3": controller.bindingLookup([
            [BmsKey.Col11, 72, 1], [BmsKey.Col21, 72, 1],
            [BmsKey.Col12, 78, 1], [BmsKey.Col22, 78, 1],
            [BmsKey.Col13, 75, 1], [BmsKey.Col23, 75, 1],
            [BmsKey.Col14, 59, -1], [BmsKey.Col24, 59, -1],
            [BmsKey.Col15, 74, -1], [BmsKey.Col25, 74, -1],
            [BmsKey.Col16, 59, 1], [BmsKey.Col26, 59, 1],
            [BmsKey.Col17, 74, 1], [BmsKey.Col27, 74, 1]
        ])
    })
    readonly property var buttonFrameGetters: ({
        "10": () => selectContext.difficultyFilter,
        "11": src => selectContext.keyFilterFrameForSourceCount(root.elementSourceFrameCount(src)),
        "12": src => selectContext.sortFrameForSourceCount(root.elementSourceFrameCount(src)),
        "20": src => root.lr2FxType[src.buttonId - 20] || 0,
        "21": src => root.lr2FxType[src.buttonId - 20] || 0,
        "22": src => root.lr2FxType[src.buttonId - 20] || 0,
        "23": src => root.lr2FxOn[src.buttonId - 23] ? 1 : 0,
        "24": src => root.lr2FxOn[src.buttonId - 23] ? 1 : 0,
        "25": src => root.lr2FxOn[src.buttonId - 23] ? 1 : 0,
        "26": src => root.lr2FxTarget[src.buttonId - 26] || 0,
        "27": src => root.lr2FxTarget[src.buttonId - 26] || 0,
        "28": src => root.lr2FxTarget[src.buttonId - 26] || 0,
        "29": () => root.lr2EqOn ? 1 : 0,
        "32": () => root.lr2PitchOn ? 1 : 0,
        "33": () => root.lr2PitchType,
        "40": () => root.lr2GaugeIndexP1,
        "41": () => root.lr2GaugeIndexP2,
        "42": () => root.lr2RandomIndexP1,
        "43": () => root.lr2RandomIndexP2,
        "46": () => root.lr2LaneCoverIndex,
        "50": () => root.lr2HidSudIndexP1,
        "51": () => root.lr2HidSudIndexP2,
        "54": () => root.lr2DpOptionIndex,
        "55": () => root.lr2HiSpeedFixIndex,
        "56": () => root.lr2BattleIndex,
        "70": () => root.lr2ScoreGraphIndex,
        "71": () => root.lr2GhostIndex,
        "72": () => root.lr2BgaIndex,
        "73": () => root.lr2BgaSizeIndex,
        "77": () => root.lr2ScoreTargetIndex,
        "78": () => root.lr2GaugeAutoShiftIndex,
        "308": () => root.lr2LnModeIndex
    })
    readonly property var selectButtonActions: ({
        "10": delta => {
            selectContext.difficultyFilter = root.wrapValue(selectContext.difficultyFilter + delta, 6);
            selectContext.sortOrFilterChanged();
            return true;
        },
        "11": (delta, sourceCount) => {
            selectContext.adjustKeyFilter(delta, sourceCount || 7);
            selectContext.sortOrFilterChanged();
            return true;
        },
        "12": (delta, sourceCount) => {
            selectContext.adjustSortMode(delta, sourceCount || 5);
            selectContext.sortOrFilterChanged();
            return true;
        },
        "13": () => false,
        "14": () => {
            root.openSelectPanel(2, false);
            return false;
        },
        "15": () => {
            root.selectGoForward();
            return false;
        },
        "16": () => {
            root.selectGoForward(undefined, true);
            return false;
        },
        "18": () => true,
        "19": () => {
            let targetItem = selectContext.activationItem();
            let replayScore = selectContext.replayScoreForType(targetItem, root.lr2ReplayType);
            if (replayScore) {
                root.selectGoForward(targetItem, false, true, replayScore);
            }
            return false;
        },
        "20": delta => controller.setFxType(0, delta),
        "21": delta => controller.setFxType(1, delta),
        "22": delta => controller.setFxType(2, delta),
        "23": () => controller.toggleFx(0),
        "24": () => controller.toggleFx(1),
        "25": () => controller.toggleFx(2),
        "26": delta => controller.setFxTarget(0, delta),
        "27": delta => controller.setFxTarget(1, delta),
        "28": delta => controller.setFxTarget(2, delta),
        "29": () => {
            root.lr2EqOn = !root.lr2EqOn;
            return true;
        },
        "32": () => {
            root.lr2PitchOn = !root.lr2PitchOn;
            return true;
        },
        "33": delta => {
            root.lr2PitchType = root.wrapValue(root.lr2PitchType + delta, 3);
            return true;
        },
        "40": delta => {
            root.setGaugeIndex(1, root.lr2GaugeIndexP1 + delta);
            return true;
        },
        "41": delta => {
            root.setGaugeIndex(2, root.lr2GaugeIndexP2 + delta);
            return true;
        },
        "42": delta => {
            root.setRandomIndex(1, root.lr2RandomIndexP1 + delta);
            return true;
        },
        "43": delta => {
            root.setRandomIndex(2, root.lr2RandomIndexP2 + delta);
            return true;
        },
        "44": () => false,
        "45": () => false,
        "46": delta => {
            root.setLaneCoverIndex(root.lr2LaneCoverIndex + delta);
            return true;
        },
        "50": delta => {
            root.setHidSudIndex(1, root.lr2HidSudIndexP1 + delta);
            return true;
        },
        "51": delta => {
            root.setHidSudIndex(2, root.lr2HidSudIndexP2 + delta);
            return true;
        },
        "54": delta => {
            root.setDpOptionIndex(root.lr2DpOptionIndex + delta);
            return true;
        },
        "55": delta => {
            root.setHiSpeedFixIndex(root.lr2HiSpeedFixIndex + delta);
            return true;
        },
        "56": delta => {
            root.setBattleIndex(root.lr2BattleIndex + delta);
            return true;
        },
        "57": delta => {
            root.adjustHiSpeedNumber(1, delta);
            return true;
        },
        "58": delta => {
            root.adjustHiSpeedNumber(2, delta);
            return true;
        },
        "59": delta => {
            root.adjustDurationNumber(1, delta);
            return true;
        },
        "70": delta => {
            root.setScoreGraphIndex(root.lr2ScoreGraphIndex + delta);
            return true;
        },
        "71": delta => {
            root.setGhostIndex(root.lr2GhostIndex + delta);
            return true;
        },
        "72": delta => {
            root.setBgaIndex(root.lr2BgaIndex + delta);
            return true;
        },
        "73": delta => {
            root.setBgaSizeIndex(root.lr2BgaSizeIndex + delta);
            return true;
        },
        "74": delta => {
            root.adjustOffset(delta);
            return true;
        },
        "75": () => false,
        "76": delta => {
            root.setTargetPercent(root.lr2TargetPercent + delta);
            return true;
        },
        "77": delta => {
            root.setScoreTargetIndex(root.lr2ScoreTargetIndex + delta);
            return true;
        },
        "78": delta => {
            root.setGaugeAutoShiftIndex(root.lr2GaugeAutoShiftIndex + delta);
            return true;
        },
        "80": () => {
            globalRoot.toggleFullScreen();
            return false;
        },
        "81": () => false,
        "82": () => false,
        "83": delta => {
            root.lr2ReplayType = root.wrapValue(root.lr2ReplayType + delta, root.lr2ReplayLabels.length);
            return true;
        },
        "301": () => false,
        "302": () => false,
        "303": () => false,
        "304": () => false,
        "305": () => false,
        "306": () => false,
        "307": () => false,
        "308": delta => root.setLnModeIndex(root.lr2LnModeIndex + delta),
        "316": () => controller.playReplaySlot(1),
        "317": () => controller.playReplaySlot(2),
        "318": () => controller.playReplaySlot(3)
    })

    function lookup(values: var) : var {
        let result = {};
        for (let value of values) {
            result[String(value)] = true;
        }
        return result;
    }

    function bindingLookup(bindings: var) : var {
        let result = {};
        for (let binding of bindings) {
            result[String(binding[0])] = { buttonId: binding[1], delta: binding[2] };
        }
        return result;
    }

    function observeSelectSortButton(src: var) : void {
        if (root.effectiveScreenKey === "select" && root.elementButtonId(src) === 12) {
            selectContext.observeSortSourceFrameCount(root.elementSourceFrameCount(src));
        }
    }

    function buttonUsesSplitArrows(buttonId: var) : var {
        if (controller.splitArrowButtons[String(buttonId)]) {
            return true;
        }
        return buttonId >= 220 && buttonId <= 229;
    }

    function imageSetButtonId(src: var) : var {
        if (!src || !src.imageSet) {
            return 0;
        }
        let id = Math.floor(src.imageSetRef || 0);
        if (id === 12) {
            return root.elementSourceFrameCount(src) >= 5 ? id : 0;
        }
        return controller.imageSetButtons[String(id)] ? id : 0;
    }

    function elementButtonId(src: var) : var {
        if (!src) {
            return 0;
        }
        if (src.button) {
            return src.buttonId || 0;
        }
        return root.imageSetButtonId(src);
    }

    function elementButtonPanel(src: var) : var {
        return src && src.button ? (src.buttonPanel || 0) : root.selectPanel;
    }

    function elementButtonClickEnabled(src: var) : var {
        if (!src) {
            return false;
        }
        return src.button
            ? src.buttonClick !== 0
            : root.selectPanel > 0 && root.imageSetButtonId(src) > 0;
    }

    function elementButtonPanelMatches(src: var) : var {
        if (!src) {
            return false;
        }
        return src.button ? root.buttonPanelMatches(src) : root.selectPanel > 0;
    }

    function buttonMouseDelta(src: var, mouseX: var, width: var) : var {
        if (src && src.buttonPlusOnly === 1) {
            return 1;
        }
        if (src && src.buttonPlusOnly === 2) {
            return -1;
        }
        if (src && root.buttonUsesSplitArrows(root.elementButtonId(src))) {
            return mouseX < width / 2 ? -1 : 1;
        }
        return 1;
    }

    function buttonFrame(src: var) : var {
        if (root.effectiveScreenKey !== "select" || !src || !src.button) {
            return -1;
        }

        let buttonId = src.buttonId || 0;
        let frameGetter = controller.buttonFrameGetters[String(buttonId)];
        if (frameGetter) {
            return frameGetter(src);
        }

        if (buttonId >= 170 && buttonId <= 188) {
            let screen = root.lr2SkinTypeScreenKey(buttonId - 170);
            return screen.length > 0 && screen === root.currentLr2SkinPreviewScreen() ? 1 : 0;
        }

        if (buttonId === 190
                || (buttonId >= 220 && buttonId <= 229)
                || (buttonId >= 230 && buttonId <= 268)
                || controller.fixedZeroButtonFrames[String(buttonId)]) {
            return 0;
        }

        return -1;
    }

    function closeSelectPanel() : void {
        if (root.selectPanel > 0) {
            root.playOneShot(optionCloseSound);
            root.startSelectPanelCloseTimer(root.selectPanel);
        }
        root.selectPanel = 0;
        root.selectPanelHeldByStart = 0;
        root.selectHeldButtonTimerStarts = ({});
    }

    function startSelectPanelCloseTimer(panel: var) : var {
        if (panel <= 0) {
            selectPanelCloseTimer.stop();
            return;
        }
        root.selectPanelCloseStartSkinTime = root.selectLiveSkinTime;
        root.selectPanelClosing = panel;
        selectPanelCloseTimer.restart();
    }

    function openSelectPanel(panel: var, heldByStart: var) : var {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel !== panel) {
            root.playOneShot(optionOpenSound);
            if (root.selectPanel > 0) {
                root.startSelectPanelCloseTimer(root.selectPanel);
            }
            root.selectPanelStartSkinTime = root.selectLiveSkinTime;
            root.selectPanel = panel;
        }
        root.selectPanelHeldByStart = heldByStart ? panel : 0;
    }

    function toggleSelectPanel(panel: var) : var {
        if (panel <= 0) {
            return;
        }
        if (root.selectPanel === panel) {
            root.closeSelectPanel();
            return;
        }
        root.openSelectPanel(panel, false);
    }

    function holdSelectPanel(panel: var) : var {
        if (!root.selectInputReady()) {
            return;
        }
        if (root.selectPanel === panel && root.selectPanelHeldByStart !== panel) {
            return;
        }
        root.openSelectPanel(panel, true);
    }

    function releaseHeldSelectPanel(panel: var) : void {
        if (root.selectPanelHeldByStart === panel) {
            root.closeSelectPanel();
        }
    }

    function currentSelectHeldButtonSkinTime() : var {
        if (root.effectiveScreenKey !== "select") {
            return root.renderSkinTime;
        }
        return root.selectLiveSkinTime;
    }

    function selectHeldButtonTimerForKey(key: var) : var {
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

    function isSelectHeldButtonTimer(timer: var) : var {
        return (timer >= 101 && timer <= 107) || (timer >= 111 && timer <= 117);
    }

    function pressSelectHeldButtonTimer(key: var) : var {
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
        copy[timer] = root.currentSelectHeldButtonSkinTime();
        root.selectHeldButtonTimerStarts = copy;
    }

    function releaseSelectHeldButtonTimer(key: var) : var {
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

    function addHeldButtonTimer(result: var, timer: var, held: var) : var {
        if (!held) {
            return;
        }
        let start = root.selectHeldButtonTimerStarts[timer];
        result[timer] = start === undefined ? root.currentSelectHeldButtonSkinTime() : start;
    }

    function selectHeldButtonTimerFireTime(timer: var, liveClock: var) : var {
        if (root.effectiveScreenKey !== "select"
                || root.selectPanel !== 1
                || !root.isSelectHeldButtonTimer(timer)) {
            return -1;
        }

        let held = false;
        switch (timer) {
        case 101:
            held = Input.col11;
            break;
        case 102:
            held = Input.col12;
            break;
        case 103:
            held = Input.col13;
            break;
        case 104:
            held = Input.col14;
            break;
        case 105:
            held = Input.col15;
            break;
        case 106:
            held = Input.col16;
            break;
        case 107:
            held = Input.col17;
            break;
        case 111:
            held = Input.col21;
            break;
        case 112:
            held = Input.col22;
            break;
        case 113:
            held = Input.col23;
            break;
        case 114:
            held = Input.col24;
            break;
        case 115:
            held = Input.col25;
            break;
        case 116:
            held = Input.col26;
            break;
        case 117:
            held = Input.col27;
            break;
        }
        if (!held) {
            return -1;
        }

        let start = root.selectHeldButtonTimerStarts[timer];
        let liveStart = start === undefined ? root.currentSelectHeldButtonSkinTime() : start;
        if (liveClock === true) {
            return liveStart;
        }
        return root.renderSkinTime - Math.max(0, root.selectLiveSkinTime - liveStart);
    }

    function addHeldButtonTimers(result: var) : var {
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

    function spriteSkinTime(src: var, dsts: var) : var {
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        return root.isSelectHeldButtonTimer(timer)
            ? (root.hasSelectHeldButtonTimers ? root.selectHeldButtonSkinTime : root.currentSelectHeldButtonSkinTime())
            : root.skinTimeForElement(src, dsts);
    }

    function buttonPanelMatches(src: var) : var {
        if (!src || !src.button) {
            return false;
        }
        return root.panelMatches(src.buttonPanel || 0);
    }

    function setFxType(slot: var, delta: var) : var {
        root.lr2FxType = root.setArrayValue(
            root.lr2FxType,
            slot,
            root.wrapValue((root.lr2FxType[slot] || 0) + delta, 8));
        return true;
    }

    function toggleFx(slot: var) : var {
        root.lr2FxOn = root.setArrayValue(root.lr2FxOn, slot, !root.lr2FxOn[slot]);
        return true;
    }

    function setFxTarget(slot: var, delta: var) : var {
        root.lr2FxTarget = root.setArrayValue(
            root.lr2FxTarget,
            slot,
            root.wrapValue((root.lr2FxTarget[slot] || 0) + delta, 3));
        return true;
    }

    function playReplaySlot(replayType: var) : var {
        let targetItem = selectContext.activationItem();
        let replayScore = selectContext.replayScoreForType(targetItem, replayType);
        if (replayScore) {
            root.selectGoForward(targetItem, false, true, replayScore);
        }
        return false;
    }

    function handleLr2Button(buttonId: var, delta: var, panel: var, soundPlayer: var, sourceCount: var) : var {
        if (!root.enabled || root.effectiveScreenKey !== "select" || !root.acceptsInput) {
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
        if (buttonId === 200) {
            root.openHelpFile(0);
            return;
        }
        if (buttonId >= 201 && buttonId <= 206 && panel > 0) {
            root.openHelpFile(buttonId - 200);
            return;
        }
        if (buttonId === 210) {
            root.openLr2InternetRanking();
            return;
        }
        let action = controller.selectButtonActions[String(buttonId)];
        let optionChanged = action ? action(delta, sourceCount) === true : false;
        if (!action) {
            if (buttonId >= 91 && buttonId <= 96) {
                selectContext.clickDifficulty(buttonId - 91);
                optionChanged = true;
            } else if (buttonId >= 170 && buttonId <= 188) {
                let screen = root.lr2SkinTypeScreenKey(buttonId - 170);
                optionChanged = root.setLr2SkinPreviewScreen(screen);
            } else if (buttonId === 190) {
                optionChanged = root.changeLr2SelectedTheme(delta);
            } else if (buttonId >= 220 && buttonId <= 229) {
                optionChanged = root.changeLr2SkinSetting(buttonId - 220, delta);
            } else if (buttonId >= 230 && buttonId <= 268) {
                // These LR2 controls cover skin/key config, IR, and tag-editor
                // actions that do not have safe backing state here.
            } else {
                console.info("LR2 select button " + buttonId + " is not implemented yet");
            }
        }
        if (optionChanged) {
            root.playOneShot(soundPlayer || optionChangeSound);
        }
    }

    function triggerSelectPanelButton(buttonId: var, delta: var) : var {
        root.handleLr2Button(buttonId, delta === undefined ? 1 : delta, root.selectPanel);
        return true;
    }

    function triggerPanelButtonForKey(p1ButtonId: var, p2ButtonId: var, key: var, delta: var) : var {
        let buttonId = p1ButtonId;
        if (p2ButtonId !== undefined && root.keyUsesPlayer2(key)) {
            buttonId = p2ButtonId;
        }
        return root.triggerSelectPanelButton(buttonId, delta === undefined ? 1 : delta);
    }

    function keyUsesPlayer2(key: var) : var {
        return controller.player2Keys[String(key)] === true;
    }

    function gameplayOptionSideForKey(key: var) : var {
        return root.keyUsesPlayer2(key) && root.battleModeActive() ? 2 : 1;
    }

    function startHeldForSide(side: var) : var {
        if (side === 2 && root.battleModeActive()) {
            return Input.start2;
        }
        return Input.start1 || (!root.battleModeActive() && Input.start2);
    }

    function selectHeldForSide(side: var) : var {
        if (side === 2 && root.battleModeActive()) {
            return Input.select2;
        }
        return Input.select1 || (!root.battleModeActive() && Input.select2);
    }

    function gameplayOptionModifierHeldForKey(key: var) : var {
        let side = root.gameplayOptionSideForKey(key);
        return controller.startHeldForSide(side) || controller.selectHeldForSide(side);
    }

    function gameplayOptionKeyHeld(key: var) : var {
        switch (key) {
        case BmsKey.Col11:
            return Input.col11;
        case BmsKey.Col12:
            return Input.col12;
        case BmsKey.Col13:
            return Input.col13;
        case BmsKey.Col14:
            return Input.col14;
        case BmsKey.Col15:
            return Input.col15;
        case BmsKey.Col16:
            return Input.col16;
        case BmsKey.Col17:
            return Input.col17;
        case BmsKey.Col21:
            return Input.col21;
        case BmsKey.Col22:
            return Input.col22;
        case BmsKey.Col23:
            return Input.col23;
        case BmsKey.Col24:
            return Input.col24;
        case BmsKey.Col25:
            return Input.col25;
        case BmsKey.Col26:
            return Input.col26;
        case BmsKey.Col27:
            return Input.col27;
        default:
            return false;
        }
    }

    function isGameplayOptionModifierKey(key: var) : var {
        return key === BmsKey.Start1
            || key === BmsKey.Start2
            || key === BmsKey.Select1
            || key === BmsKey.Select2;
    }

    function startLr2GameplayOptionRepeat(key: var) : void {
        controller.gameplayOptionRepeatKey = key;
        controller.gameplayOptionRepeating = true;
        gameplayOptionRepeatTimer.interval = controller.gameplayOptionInitialRepeatMillis;
        gameplayOptionRepeatTimer.restart();
    }

    function stopLr2GameplayOptionRepeat() : void {
        controller.gameplayOptionRepeating = false;
        controller.gameplayOptionRepeatKey = -1;
        gameplayOptionRepeatTimer.stop();
    }

    function setGameplayScratchLastDirection(side: var, up: var) : void {
        let next = {};
        for (let key in controller.gameplayScratchLastDirectionUp) {
            next[key] = controller.gameplayScratchLastDirectionUp[key];
        }
        next[String(side)] = !!up;
        controller.gameplayScratchLastDirectionUp = next;
    }

    function pressLr2GameplayScratchDirection(side: var, up: var) : void {
        if (root.isGameplayScreen()) {
            controller.setGameplayScratchLastDirection(side, up);
        }
    }

    function releaseLr2GameplayScratchDirection(side: var, up: var) : void {
        if (root.isGameplayScreen()) {
            controller.setGameplayScratchLastDirection(side, !up);
        }
    }

    function releaseLr2GameplayOptionKey(key: var) : var {
        if (!controller.gameplayOptionRepeating) {
            return;
        }
        if (key === controller.gameplayOptionRepeatKey
                || (controller.isGameplayOptionModifierKey(key)
                    && !root.gameplayOptionModifierHeldForKey(controller.gameplayOptionRepeatKey))) {
            controller.stopLr2GameplayOptionRepeat();
        }
    }

    function repeatGameplayOptionKey() : var {
        if (!controller.gameplayOptionRepeating) {
            return;
        }
        let key = controller.gameplayOptionRepeatKey;
        if (!root.isGameplayScreen()
                || !root.gameplayOptionKeyHeld(key)
                || !root.gameplayOptionModifierHeldForKey(key)
                || !controller.applyLr2GameplayOptionKey(key)) {
            controller.stopLr2GameplayOptionRepeat();
            return;
        }
        gameplayOptionRepeatTimer.interval = controller.gameplayOptionRepeatMillis;
        gameplayOptionRepeatTimer.restart();
    }

    function isGameplayDecreaseKey(key: var) : var {
        switch (key) {
        case BmsKey.Col11:
        case BmsKey.Col13:
        case BmsKey.Col15:
        case BmsKey.Col17:
        case BmsKey.Col21:
        case BmsKey.Col23:
        case BmsKey.Col25:
        case BmsKey.Col27:
            return true;
        default:
            return false;
        }
    }

    function isGameplayIncreaseKey(key: var) : var {
        switch (key) {
        case BmsKey.Col12:
        case BmsKey.Col14:
        case BmsKey.Col16:
        case BmsKey.Col22:
        case BmsKey.Col24:
        case BmsKey.Col26:
            return true;
        default:
            return false;
        }
    }

    function applyLr2LaneCoverKey(side: var, key: var) : var {
        switch (key) {
        case BmsKey.Col16:
        case BmsKey.Col26:
            root.adjustLaneCoverRatio(side, -1);
            return true;
        case BmsKey.Col17:
        case BmsKey.Col27:
            root.adjustLaneCoverRatio(side, 1);
            return true;
        default:
            return false;
        }
    }

    function applyLr2GameplayOptionKey(key: var) : var {
        if (!root.isGameplayScreen() || !root.gameplayOptionModifierHeldForKey(key)) {
            return false;
        }

        let side = root.gameplayOptionSideForKey(key);
        let startHeld = controller.startHeldForSide(side);
        let selectHeld = controller.selectHeldForSide(side);
        if (startHeld === selectHeld) {
            return false;
        }

        if (selectHeld) {
            if (controller.isGameplayDecreaseKey(key)) {
                root.adjustDurationNumber(side, 1);
                return true;
            }
            if (controller.isGameplayIncreaseKey(key)) {
                root.adjustDurationNumber(side, -1);
                return true;
            }
            return false;
        }

        let vars = root.generalVarsForSide(side);
        if (vars && vars.laneCoverOn && controller.applyLr2LaneCoverKey(side, key)) {
            return true;
        }
        if (controller.isGameplayDecreaseKey(key)) {
            root.adjustHiSpeedNumber(side, -1);
            return true;
        }
        if (controller.isGameplayIncreaseKey(key)) {
            root.adjustHiSpeedNumber(side, 1);
            return true;
        }
        return false;
    }

    function handleGameplayStartPress(key: var) : var {
        if (!root.isGameplayScreen()
                || (key !== BmsKey.Start1 && key !== BmsKey.Start2)) {
            return false;
        }
        let side = root.gameplayOptionSideForKey(key);
        if (controller.selectHeldForSide(side)) {
            return false;
        }
        let now = Date.now();
        let sideKey = String(side);
        let last = controller.gameplayLastStartPressMs[sideKey] || 0;
        let next = {};
        for (let keyName in controller.gameplayLastStartPressMs) {
            next[keyName] = controller.gameplayLastStartPressMs[keyName];
        }
        if (last > 0 && now - last <= controller.gameplayLaneCoverDoublePressMillis) {
            next[sideKey] = 0;
            controller.gameplayLastStartPressMs = next;
            root.toggleLaneCover(side);
            root.refreshGameplayRuntimeActiveOptions();
            root.bumpGameplayRevision();
            return true;
        }
        next[sideKey] = now;
        controller.gameplayLastStartPressMs = next;
        return false;
    }

    function handleLr2GameplayOptionKey(key: var) : var {
        if (controller.handleGameplayStartPress(key)) {
            return true;
        }
        if (!controller.applyLr2GameplayOptionKey(key)) {
            return false;
        }
        controller.startLr2GameplayOptionRepeat(key);
        return true;
    }

    function gameplayScratchBothDirectionsHeld(side: var) : var {
        return side === 2
            ? Input.col2sUp && Input.col2sDown
            : Input.col1sUp && Input.col1sDown;
    }

    function gameplayScratchAmount(side: var, up: var, number: var) : var {
        let amount = up ? -1 : 1;
        let lastDirectionUp = controller.gameplayScratchLastDirectionUp[String(side)] === true;
        if ((amount > 0 && lastDirectionUp) || (amount < 0 && !lastDirectionUp)) {
            return 0;
        }
        if ((number || 0) > 50 || controller.gameplayScratchBothDirectionsHeld(side)) {
            return 10 * amount;
        }
        return amount;
    }

    function handleLr2GameplayScratchTick(side: var, up: var, number: var) : var {
        if (!root.isGameplayScreen()) {
            return false;
        }
        let key = side === 2 ? BmsKey.Col2sUp : BmsKey.Col1sUp;
        if (!root.gameplayOptionModifierHeldForKey(key)) {
            return false;
        }
        let controlSide = root.gameplayOptionSideForKey(key);
        let startHeld = controller.startHeldForSide(controlSide);
        let selectHeld = controller.selectHeldForSide(controlSide);
        if (startHeld === selectHeld) {
            return false;
        }
        let amount = controller.gameplayScratchAmount(side, up, number);
        if (amount === 0) {
            return true;
        }
        if (startHeld) {
            root.adjustScratchDurationNumber(controlSide, amount);
        } else {
            root.adjustScratchCoverNumber(controlSide, amount);
        }
        return true;
    }

    function handleLr2GameplayArrow(key: var) : var {
        if (!root.isGameplayScreen()) {
            return false;
        }
        switch (key) {
        case Qt.Key_Up:
            root.adjustHiSpeedNumber(1, 1);
            return true;
        case Qt.Key_Down:
            root.adjustHiSpeedNumber(1, -1);
            return true;
        case Qt.Key_Left:
            root.adjustLaneCoverRatio(1, 1);
            return true;
        case Qt.Key_Right:
            root.adjustLaneCoverRatio(1, -1);
            return true;
        default:
            return false;
        }
    }

    function handleSelectPanelKey(key: var) : var {
        if (!root.selectInputReady() || root.selectPanel <= 0) {
            return false;
        }

        let panelBindings = controller.selectPanelKeyBindings[String(root.selectPanel)];
        let binding = panelBindings ? panelBindings[String(key)] : null;
        return binding ? root.triggerSelectPanelButton(binding.buttonId, binding.delta) : false;
    }

    property real selectWheelRemainder: 0

    function handleSelectWheel(wheel: var) : var {
        if (root.lr2ReadmeMode === 1) {
            let readmeDelta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
            if (readmeDelta !== 0) {
                root.scrollReadmeBy(0, readmeDelta / 120.0 * Math.max(1, root.lr2ReadmeLineSpacing));
            }
            wheel.accepted = true;
            return;
        }
        if (!root.selectPointerScrollReady()) {
            return;
        }
        let delta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
        if (delta === 0) {
            return;
        }
        controller.selectWheelRemainder += delta / 120.0;
        let steps = controller.selectWheelRemainder > 0
            ? Math.floor(controller.selectWheelRemainder)
            : Math.ceil(controller.selectWheelRemainder);
        if (steps !== 0) {
            controller.selectWheelRemainder -= steps;
            selectContext.queueWheelSteps(steps);
        }
        wheel.accepted = true;
    }

}
