pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

QtObject {
    id: controller

    required property var screenRoot
    required property var selectContext
    property var selectHoverState: null
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
    readonly property var selectHeldButtonTimerFireTimes: {
        let result = {};
        controller.addHeldButtonTimers(result);
        return result;
    }
    readonly property int selectTargetScratchInitialRepeatMillis: 300
    readonly property int selectTargetScratchRepeatMillis: 50
    property int selectTargetScratchDirection: 0
    property real selectTargetScratchNextMs: 0
    readonly property int selectPanelButtonInitialRepeatMillis: 300
    readonly property int selectPanelButtonRepeatMillis: 50
    property int selectPanelRepeatKey: -1
    property int selectPanelRepeatButtonId: 0
    property int selectPanelRepeatDelta: 0
    property int selectPanelRepeatPanel: 0
    property bool selectPanelButtonRepeating: false
    readonly property int gameplayOptionInitialRepeatMillis: 300
    readonly property int gameplayOptionRepeatMillis: 50
    readonly property int gameplayLaneCoverDoublePressMillis: 500
    property int gameplayOptionRepeatKey: -1
    property bool gameplayOptionRepeating: false
    property var gameplayLastStartPressMs: ({})
    property var gameplayStartSelectHeld: ({ "1": false, "2": false })
    property var gameplayCoverChangeLift: ({ "1": true, "2": true })
    property var gameplayScratchLastDirectionUp: ({ "1": false, "2": false })
    property var selectButtonSourcesByElement: ({})
    property var observedSelectButtonSourceCounts: ({})
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

    property Timer selectPanelButtonRepeatTimer: Timer {
        id: selectPanelButtonRepeatTimer
        repeat: false
        onTriggered: controller.repeatSelectPanelButton()
    }

    onSelectPanelChanged: {
        if (controller.selectPanel > 0) {
            root.clearSelectSearchFocus();
            if (controller.selectPanelButtonRepeating
                    && controller.selectPanelRepeatPanel !== controller.selectPanel) {
                controller.stopSelectPanelButtonRepeat();
            }
        } else {
            root.resetLr2SelectScratchRepeat();
            controller.stopSelectPanelButtonRepeat();
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
        if (controller.selectHoverState
                && controller.selectHoverState.hasPoint
                && controller.selectPanel > 0) {
            controller.selectHoverState.refreshVisibleState();
        }
    }

    onSelectPanelCloseElapsedChanged: {
        if (controller.selectHoverState
                && controller.selectHoverState.hasPoint
                && controller.selectPanelClosing > 0) {
            controller.selectHoverState.refreshVisibleState();
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
        if (!root.gameplayScreenActive) {
            return;
        }
        controller.updateGameplayCoverChangeTarget(1);
        controller.updateGameplayCoverChangeTarget(2);
        root.refreshGameplayRuntimeActiveOptions();
        root.bumpGameplayRevision();
        if (controller.gameplayOptionRepeating
                && !root.gameplayOptionModifierHeldForKey(controller.gameplayOptionRepeatKey)) {
            controller.stopLr2GameplayOptionRepeat();
        }
    }

    function setGameplayStartSelectHeld(side: var, held: var) : void {
        let key = String(side);
        let next = {};
        for (let name in controller.gameplayStartSelectHeld) {
            next[name] = controller.gameplayStartSelectHeld[name];
        }
        next[key] = held;
        controller.gameplayStartSelectHeld = next;
    }

    function toggleGameplayCoverChangeLift(side: var) : void {
        let key = String(side);
        let next = {};
        for (let name in controller.gameplayCoverChangeLift) {
            next[name] = controller.gameplayCoverChangeLift[name];
        }
        next[key] = !(controller.gameplayCoverChangeLift[key] !== false);
        controller.gameplayCoverChangeLift = next;
    }

    function updateGameplayCoverChangeTarget(side: var) : void {
        let key = String(side);
        let held = controller.startHeldForSide(side) && controller.selectHeldForSide(side);
        if (held && controller.gameplayStartSelectHeld[key] !== true) {
            controller.toggleGameplayCoverChangeLift(side);
        }
        if (controller.gameplayStartSelectHeld[key] !== held) {
            controller.setGameplayStartSelectHeld(side, held);
        }
    }

    readonly property var splitArrowButtons: controller.lookup([
        10, 11, 12, 20, 21, 22, 26, 27, 28, 33,
        40, 41, 42, 43, 46, 50, 51, 54, 55, 56,
        57, 58, 59, 72, 74, 76, 77, 78, 83, 190, 308,
        340, 341, 342, 344, 350, 351, 352, 353, 360, 361, 400
    ])
    readonly property var imageSetButtons: controller.lookup([
        40, 41, 42, 43, 54, 55, 72, 74, 77, 78,
        301, 302, 303, 304, 305, 306, 307, 308,
        340, 341, 342, 343, 350, 351, 352, 353, 360, 361, 400
    ])
    readonly property var fixedZeroButtonFrames: controller.lookup([
        13, 14, 18, 44, 45, 74, 75, 80, 81, 82, 83,
        210, 301, 302, 303, 304, 305, 306, 307,
        342, 343, 344, 350, 351, 352, 353, 360, 361, 400
    ])
    readonly property var player2Keys: controller.lookup([
        BmsKey.Col21, BmsKey.Col22, BmsKey.Col23, BmsKey.Col24,
        BmsKey.Col25, BmsKey.Col26, BmsKey.Col27,
        BmsKey.Col2sUp, BmsKey.Col2sDown, BmsKey.Start2, BmsKey.Select2
    ])
    readonly property var lr2SelectPanelKeyBindings: ({
        "1": controller.bindingLookup([
            [BmsKey.Col11, 11, 1], [BmsKey.Col21, 11, 1],
            [BmsKey.Col12, 9042, 1], [BmsKey.Col22, 9043, 1],
            [BmsKey.Col13, 9003, 1], [BmsKey.Col23, 9003, 1],
            [BmsKey.Col14, 40, 1], [BmsKey.Col24, 41, 1],
            [BmsKey.Col15, 57, -1], [BmsKey.Col25, 58, -1],
            [BmsKey.Col16, 44, 1], [BmsKey.Col26, 45, 1],
            [BmsKey.Col17, 57, 1], [BmsKey.Col27, 58, 1]
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
    readonly property var beatorajaSelectPanelKeyBindings: ({
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
            [BmsKey.Col11, 301, 1], [BmsKey.Col21, 301, 1],
            [BmsKey.Col12, 302, 1], [BmsKey.Col22, 302, 1],
            [BmsKey.Col13, 303, 1], [BmsKey.Col23, 303, 1],
            [BmsKey.Col14, 304, 1], [BmsKey.Col24, 304, 1],
            [BmsKey.Col15, 305, 1], [BmsKey.Col25, 305, 1],
            [BmsKey.Col16, 306, 1], [BmsKey.Col26, 306, 1],
            [BmsKey.Col17, 307, 1], [BmsKey.Col27, 307, 1]
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
    readonly property var activeSelectPanelKeyBindings: root.lr2SkinUsesBeatorajaSemantics
        ? controller.beatorajaSelectPanelKeyBindings
        : controller.lr2SelectPanelKeyBindings
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
        "40": src => root.lr2GaugeButtonFrame(1, root.elementSourceFrameCount(src)),
        "41": src => root.lr2GaugeButtonFrame(2, root.elementSourceFrameCount(src)),
        "42": src => root.lr2RandomButtonFrame(1, root.elementSourceFrameCount(src)),
        "43": src => root.lr2RandomButtonFrame(2, root.elementSourceFrameCount(src)),
        "46": () => controller.lr2ClassicHidSudControlPresent() ? -2 : root.lr2LaneCoverIndex,
        "50": () => root.lr2HidSudIndexP1,
        "51": () => root.lr2HidSudIndexP2,
        "54": () => root.lr2DpOptionIndex,
        "55": () => root.lr2HiSpeedFixIndex,
        "56": () => root.lr2BattleIndex,
        "70": () => root.lr2ScoreGraphIndex,
        "71": () => root.lr2GhostIndex,
        "72": () => root.lr2BgaIndex,
        "73": () => root.lr2BgaSizeIndex,
        "77": src => root.lr2TargetButtonFrame(root.elementSourceFrameCount(src)),
        "78": () => root.lr2GaugeAutoShiftIndex,
        "330": () => root.lr2LaneCoverIndex,
        "331": () => root.lr2LiftIndex,
        "332": () => root.lr2HiddenIndex,
        "340": () => root.lr2JudgeAlgorithmIndex,
        "341": () => root.lr2BottomShiftableGaugeIndex,
        "308": () => root.lr2LnModeIndex
    })
    readonly property var selectButtonActions: ({
        "10": delta => {
            selectContext.difficultyFilter = root.wrapValue(selectContext.difficultyFilter + delta, 6);
            selectContext.sortOrFilterChanged();
            return true;
        },
        "11": (delta, sourceCount) => {
            selectContext.adjustKeyFilter(delta, sourceCount || (root.lr2SkinUsesBeatorajaSemantics ? 5 : 7));
            selectContext.sortOrFilterChanged();
            return true;
        },
        "12": (delta, sourceCount) => {
            selectContext.adjustSortMode(delta, sourceCount || (root.lr2SkinUsesBeatorajaSemantics ? 8 : 5));
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
        "19": (delta, sourceCount, mouseButton) => controller.launchReplayType(root.lr2ReplayType, mouseButton),
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
        "40": (delta, sourceCount) => {
            root.adjustGaugeButtonIndex(1, delta, sourceCount);
            return true;
        },
        "41": (delta, sourceCount) => {
            root.adjustGaugeButtonIndex(2, delta, sourceCount);
            return true;
        },
        "42": (delta, sourceCount) => {
            root.adjustRandomButtonIndex(1, delta, sourceCount);
            return true;
        },
        "43": (delta, sourceCount) => {
            root.adjustRandomButtonIndex(2, delta, sourceCount);
            return true;
        },
        "44": () => false,
        "45": () => false,
        "46": delta => {
            if (controller.lr2ClassicHidSudControlPresent()) {
                return false;
            }
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
        "77": (delta, sourceCount) => {
            root.adjustTargetButtonIndex(delta, sourceCount);
            return true;
        },
        "78": delta => {
            root.setGaugeAutoShiftIndex(root.lr2GaugeAutoShiftIndex + delta);
            return true;
        },
        "341": delta => {
            root.setBottomShiftableGaugeIndex(root.lr2BottomShiftableGaugeIndex + delta);
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
        "301": () => root.toggleBeatorajaAssistOption(301),
        "302": () => root.toggleBeatorajaAssistOption(302),
        "303": () => root.toggleBeatorajaAssistOption(303),
        "304": () => root.toggleBeatorajaAssistOption(304),
        "305": () => root.toggleBeatorajaAssistOption(305),
        "306": () => root.toggleBeatorajaAssistOption(306),
        "307": () => root.toggleBeatorajaAssistOption(307),
        "308": delta => root.setLnModeIndex(root.lr2LnModeIndex + delta),
        "316": (delta, sourceCount, mouseButton) => controller.playReplaySlot(1, mouseButton),
        "317": (delta, sourceCount, mouseButton) => controller.playReplaySlot(2, mouseButton),
        "318": (delta, sourceCount, mouseButton) => controller.playReplaySlot(3, mouseButton),
        "330": delta => {
            root.setLaneCoverIndex(root.lr2LaneCoverIndex + delta);
            return true;
        },
        "331": delta => {
            root.setLiftIndex(root.lr2LiftIndex + delta);
            return true;
        },
        "332": delta => {
            root.setHiddenIndex(root.lr2HiddenIndex + delta);
            return true;
        },
        "340": delta => root.setJudgeAlgorithmIndex(root.lr2JudgeAlgorithmIndex + delta),
        "342": () => false,
        "343": () => false,
        "344": () => false,
        "350": () => false,
        "351": () => false,
        "352": () => false,
        "353": () => false,
        "360": () => false,
        "361": () => false,
        "400": () => false,
        "9003": delta => controller.setLr2BattleOrFlip(delta),
        "9042": delta => controller.setLr2PanelRandomOption(1, delta),
        "9043": delta => controller.setLr2PanelRandomOption(2, delta),
        "9050": delta => controller.adjustLr2PanelHidSudOption(1, delta),
        "9051": delta => controller.adjustLr2PanelHidSudOption(2, delta)
    })

    function lr2SelectKeyFilterIsDouble() : var {
        switch (selectContext.selectKeymodeFilter) {
        case SelectKeymodeFilter.Double:
        case SelectKeymodeFilter.K10:
        case SelectKeymodeFilter.K14:
            return true;
        default:
            return false;
        }
    }

    function lr2Player2PanelOptionsActive() : var {
        return root.battleModeActive() || controller.lr2SelectKeyFilterIsDouble();
    }

    function setLr2PanelRandomOption(side: var, delta: var) : var {
        let effectiveSide = side === 2 && controller.lr2Player2PanelOptionsActive() ? 2 : 1;
        let buttonId = effectiveSide === 2 ? 43 : 42;
        root.adjustRandomButtonIndex(
            effectiveSide,
            delta,
            controller.observedSelectButtonSourceCount(buttonId));
        return true;
    }

    function setLr2BattleOrFlip(delta: var) : var {
        if (controller.lr2SelectKeyFilterIsDouble()) {
            root.setFlipIndex(root.lr2FlipIndex + delta);
        } else {
            root.setBattleIndex(root.lr2BattleIndex + delta);
        }
        return true;
    }

    function adjustLr2PanelHidSudOption(side: var, delta: var) : var {
        if (side === 2 && root.battleModeActive()) {
            root.setHidSudIndex(2, root.lr2HidSudIndexP2 + delta);
            return true;
        }
        let next = root.lr2HidSudIndexP1 + delta;
        root.setHidSudIndex(1, next);
        root.setHidSudIndex(2, next);
        return true;
    }

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

    function copyObject(object: var) : var {
        let result = {};
        const keys = Object.keys(object || {});
        for (let key of keys) {
            result[key] = object[key];
        }
        return result;
    }

    function rebuildObservedSelectButtonSourceCounts() : void {
        let counts = {};
        const keys = Object.keys(controller.selectButtonSourcesByElement);
        for (let key of keys) {
            const entry = controller.selectButtonSourcesByElement[key];
            if (!entry || entry.buttonId <= 0) {
                continue;
            }
            const countKey = String(entry.buttonId);
            counts[countKey] = Math.max(counts[countKey] || 0, entry.sourceCount || 0);
        }
        controller.observedSelectButtonSourceCounts = counts;
    }

    function registerSelectButtonSource(elementIndex: var, src: var) : void {
        const key = String(elementIndex);
        const buttonId = controller.elementButtonId(src);
        const sourceCount = root.elementSourceFrameCount(src);
        let next = controller.copyObject(controller.selectButtonSourcesByElement);

        if (buttonId > 0 && sourceCount > 0) {
            next[key] = { buttonId: buttonId, sourceCount: sourceCount };
        } else {
            delete next[key];
        }

        controller.selectButtonSourcesByElement = next;
        controller.rebuildObservedSelectButtonSourceCounts();

        if (root.effectiveScreenKey === "select" && buttonId === 12) {
            selectContext.observeSortSourceFrameCount(sourceCount);
        }
    }

    function unregisterSelectButtonSource(elementIndex: var) : void {
        const key = String(elementIndex);
        if (controller.selectButtonSourcesByElement[key] === undefined) {
            return;
        }
        let next = controller.copyObject(controller.selectButtonSourcesByElement);
        delete next[key];
        controller.selectButtonSourcesByElement = next;
        controller.rebuildObservedSelectButtonSourceCounts();
    }

    function observedSelectButtonSourceCount(buttonId: var) : var {
        return Math.floor(controller.observedSelectButtonSourceCounts[String(buttonId)] || 0);
    }

    function lr2ClassicHidSudControlPresent() : var {
        return controller.observedSelectButtonSourceCount(50) > 0
            || controller.observedSelectButtonSourceCount(51) > 0;
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
        return controller.imageSetButtonId(src);
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
            : root.selectPanel > 0 && controller.imageSetButtonId(src) > 0;
    }

    function elementButtonPanelMatches(src: var) : var {
        if (!src) {
            return false;
        }
        return src.button ? controller.buttonPanelMatches(src) : root.selectPanel > 0;
    }

    function buttonMouseDelta(src: var, mouseX: var, width: var) : var {
        if (src && src.buttonPlusOnly === 1) {
            return 1;
        }
        if (src && src.buttonPlusOnly === 2) {
            return -1;
        }
        if (src && controller.buttonUsesSplitArrows(controller.elementButtonId(src))) {
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
        controller.stopSelectPanelButtonRepeat();
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
        let timer = controller.selectHeldButtonTimerForKey(key);
        if (!timer) {
            return;
        }
        let starts = controller.selectHeldButtonTimerStarts;
        if (starts[timer] !== undefined) {
            return;
        }
        let copy = {};
        for (let keyName in starts) {
            copy[keyName] = starts[keyName];
        }
        copy[timer] = controller.currentSelectHeldButtonSkinTime();
        controller.selectHeldButtonTimerStarts = copy;
    }

    function releaseSelectHeldButtonTimer(key: var) : var {
        let timer = controller.selectHeldButtonTimerForKey(key);
        if (!timer || controller.selectHeldButtonTimerStarts[timer] === undefined) {
            return;
        }
        let copy = {};
        for (let keyName in controller.selectHeldButtonTimerStarts) {
            if (Number(keyName) !== timer) {
                copy[keyName] = controller.selectHeldButtonTimerStarts[keyName];
            }
        }
        controller.selectHeldButtonTimerStarts = copy;
    }

    function selectPanelButtonCanRepeat(buttonId: var) : var {
        return buttonId === 59 || buttonId === 74;
    }

    function startSelectPanelButtonRepeat(key: var, binding: var) : void {
        if (!binding || !controller.selectPanelButtonCanRepeat(binding.buttonId)) {
            controller.stopSelectPanelButtonRepeat();
            return;
        }
        controller.selectPanelRepeatKey = key;
        controller.selectPanelRepeatButtonId = binding.buttonId;
        controller.selectPanelRepeatDelta = binding.delta === undefined ? 1 : binding.delta;
        controller.selectPanelRepeatPanel = root.selectPanel;
        controller.selectPanelButtonRepeating = true;
        selectPanelButtonRepeatTimer.interval = controller.selectPanelButtonInitialRepeatMillis;
        selectPanelButtonRepeatTimer.restart();
    }

    function stopSelectPanelButtonRepeat() : void {
        controller.selectPanelButtonRepeating = false;
        controller.selectPanelRepeatKey = -1;
        controller.selectPanelRepeatButtonId = 0;
        controller.selectPanelRepeatDelta = 0;
        controller.selectPanelRepeatPanel = 0;
        selectPanelButtonRepeatTimer.stop();
    }

    function releaseSelectPanelButtonRepeat(key: var) : var {
        if (!controller.selectPanelButtonRepeating || key !== controller.selectPanelRepeatKey) {
            return;
        }
        controller.stopSelectPanelButtonRepeat();
    }

    function repeatSelectPanelButton() : var {
        if (!controller.selectPanelButtonRepeating) {
            return;
        }

        let key = controller.selectPanelRepeatKey;
        if (!root.selectInputReady()
                || root.selectPanel !== controller.selectPanelRepeatPanel
                || !controller.gameplayOptionKeyHeld(key)) {
            controller.stopSelectPanelButtonRepeat();
            return;
        }

        let panelBindings = controller.activeSelectPanelKeyBindings[String(root.selectPanel)];
        let binding = panelBindings ? panelBindings[String(key)] : null;
        if (!binding
                || binding.buttonId !== controller.selectPanelRepeatButtonId
                || !controller.selectPanelButtonCanRepeat(binding.buttonId)) {
            controller.stopSelectPanelButtonRepeat();
            return;
        }

        root.triggerSelectPanelButton(binding.buttonId, controller.selectPanelRepeatDelta);
        selectPanelButtonRepeatTimer.interval = controller.selectPanelButtonRepeatMillis;
        selectPanelButtonRepeatTimer.restart();
    }

    function addHeldButtonTimer(result: var, timer: var, held: var) : var {
        if (!held) {
            return;
        }
        let start = controller.selectHeldButtonTimerStarts[timer];
        result[timer] = start === undefined ? controller.currentSelectHeldButtonSkinTime() : start;
    }

    function selectHeldButtonTimerFireTime(timer: var, liveClock: var) : var {
        if (root.effectiveScreenKey !== "select"
                || root.selectPanel !== 1
                || !controller.isSelectHeldButtonTimer(timer)) {
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

        let start = controller.selectHeldButtonTimerStarts[timer];
        let liveStart = start === undefined ? controller.currentSelectHeldButtonSkinTime() : start;
        if (liveClock === true) {
            return liveStart;
        }
        return root.renderSkinTime - Math.max(0, root.selectLiveSkinTime - liveStart);
    }

    function addHeldButtonTimers(result: var) : var {
        if (root.effectiveScreenKey !== "select" || root.selectPanel !== 1) {
            return;
        }
        controller.addHeldButtonTimer(result, 101, Input.col11);
        controller.addHeldButtonTimer(result, 102, Input.col12);
        controller.addHeldButtonTimer(result, 103, Input.col13);
        controller.addHeldButtonTimer(result, 104, Input.col14);
        controller.addHeldButtonTimer(result, 105, Input.col15);
        controller.addHeldButtonTimer(result, 106, Input.col16);
        controller.addHeldButtonTimer(result, 107, Input.col17);
        controller.addHeldButtonTimer(result, 111, Input.col21);
        controller.addHeldButtonTimer(result, 112, Input.col22);
        controller.addHeldButtonTimer(result, 113, Input.col23);
        controller.addHeldButtonTimer(result, 114, Input.col24);
        controller.addHeldButtonTimer(result, 115, Input.col25);
        controller.addHeldButtonTimer(result, 116, Input.col26);
        controller.addHeldButtonTimer(result, 117, Input.col27);
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

    function launchReplayType(replayType: var, mouseButton: var) : var {
        if (root.launchLr2RankingReplayType(replayType, mouseButton)) {
            return false;
        }
        let targetItem = selectContext.activationItem();
        let replayScore = selectContext.replayScoreForType(targetItem, replayType);
        if (!replayScore) {
            return false;
        }
        if (mouseButton === Qt.RightButton) {
            selectContext.openReplayResult(targetItem, replayScore);
            return false;
        }
        root.selectGoForward(targetItem, false, mouseButton !== Qt.MiddleButton, replayScore);
        return false;
    }

    function playReplaySlot(replayType: var, mouseButton: var) : var {
        return controller.launchReplayType(replayType, mouseButton);
    }

    function handleLr2Button(buttonId: var, delta: var, panel: var, soundPlayer: var, sourceCount: var, mouseButton: var) : var {
        if (!root.enabled || root.effectiveScreenKey !== "select" || !root.acceptsInput) {
            return;
        }
        root.resetSelectSearch();
        if (buttonId >= 1 && buttonId <= 9) {
            root.toggleSelectPanel(buttonId);
            return;
        }
        if (buttonId === 17) {
            let path = selectContext.attachedTextFile(selectContext.selectedStateChartData);
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
        let optionChanged = action ? action(delta, sourceCount, mouseButton) === true : false;
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

    function handleLr2SelectPanelChord(key: var, panel: var) : var {
        if (root.lr2SkinUsesBeatorajaSemantics || panel !== 1) {
            return false;
        }

        if ((key === BmsKey.Col16 && Input.col17)
                || (key === BmsKey.Col17 && Input.col16)) {
            return root.triggerSelectPanelButton(9050, -1);
        }
        if ((key === BmsKey.Col26 && Input.col27)
                || (key === BmsKey.Col27 && Input.col26)) {
            return root.triggerSelectPanelButton(9051, 1);
        }

        if (key === BmsKey.Col15 && Input.col17) {
            return root.triggerSelectPanelButton(55, 1);
        }
        if (key === BmsKey.Col17 && Input.col15) {
            return root.triggerSelectPanelButton(55, root.battleModeActive() ? -1 : 1);
        }
        if (key === BmsKey.Col25 && Input.col27) {
            return root.triggerSelectPanelButton(55, 1);
        }
        if (key === BmsKey.Col27 && Input.col25) {
            return root.triggerSelectPanelButton(55, root.battleModeActive() ? -1 : 1);
        }
        return false;
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
        if (root.gameplayScreenActive) {
            controller.setGameplayScratchLastDirection(side, up);
        }
    }

    function releaseLr2GameplayScratchDirection(side: var, up: var) : void {
        if (root.gameplayScreenActive) {
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
        if (!root.gameplayScreenActive
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
        if (!root.gameplayScreenActive || !root.gameplayOptionModifierHeldForKey(key)) {
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
        if (!root.gameplayScreenActive
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
        if (!root.gameplayScreenActive) {
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
            root.adjustScratchCoverNumber(
                controlSide,
                amount,
                controller.gameplayCoverChangeLift[String(controlSide)] !== false);
        }
        return true;
    }

    function handleLr2GameplayArrow(key: var) : var {
        if (!root.gameplayScreenActive) {
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

        if (controller.handleLr2SelectPanelChord(key, root.selectPanel)) {
            return true;
        }

        let panelBindings = controller.activeSelectPanelKeyBindings[String(root.selectPanel)];
        let binding = panelBindings ? panelBindings[String(key)] : null;
        if (!binding) {
            return false;
        }
        if ((binding.buttonId || 0) <= 0) {
            return true;
        }
        let handled = root.triggerSelectPanelButton(binding.buttonId, binding.delta);
        if (handled) {
            controller.startSelectPanelButtonRepeat(key, binding);
        }
        return handled;
    }

    function handleSelectPanelArrow(key: var) : var {
        if (!root.selectInputReady() || root.selectPanel <= 0) {
            return false;
        }
        if (!root.lr2SkinUsesBeatorajaSemantics && root.selectPanel === 1) {
            if (key === Qt.Key_Right) {
                return root.triggerSelectPanelButton(77, 1);
            }
            if (key === Qt.Key_Left) {
                return root.triggerSelectPanelButton(77, -1);
            }
        }
        return true;
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
