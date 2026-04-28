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
    readonly property var splitArrowButtons: controller.lookup([
        10, 11, 12, 20, 21, 22, 26, 27, 28, 33,
        40, 41, 42, 43, 46, 50, 51, 54, 55, 56,
        57, 58, 72, 74, 76, 77, 78, 83, 190, 308
    ])
    readonly property var imageSetButtons: controller.lookup([
        40, 41, 42, 43, 54, 55, 72, 74, 77, 78,
        301, 302, 303, 304, 305, 306, 307, 308
    ])
    readonly property var fixedZeroButtonFrames: controller.lookup([
        13, 14, 18, 44, 45, 74, 75, 80, 81, 82, 83,
        210, 301, 302, 303, 304, 305, 306, 307, 308
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
        ])
    })
    readonly property var buttonFrameGetters: ({
        "10": () => selectContext.difficultyFilter,
        "11": () => Math.min(selectContext.keyFilter, 6),
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
        "77": () => root.lr2BeatorajaTargetIndex,
        "78": () => root.lr2GaugeAutoShiftIndex
    })

    function lookup(values) {
        let result = {};
        for (let value of values) {
            result[String(value)] = true;
        }
        return result;
    }

    function bindingLookup(bindings) {
        let result = {};
        for (let binding of bindings) {
            result[String(binding[0])] = { buttonId: binding[1], delta: binding[2] };
        }
        return result;
    }

    function observeSelectSortButton(src) {
        if (root.effectiveScreenKey === "select" && root.elementButtonId(src) === 12) {
            selectContext.observeSortSourceFrameCount(root.elementSourceFrameCount(src));
        }
    }

    function buttonUsesSplitArrows(buttonId) {
        if (controller.splitArrowButtons[String(buttonId)]) {
            return true;
        }
        return buttonId >= 220 && buttonId <= 229;
    }

    function imageSetButtonId(src) {
        if (!src || !src.imageSet) {
            return 0;
        }
        let id = Math.floor(src.imageSetRef || 0);
        if (id === 12) {
            return root.elementSourceFrameCount(src) >= 5 ? id : 0;
        }
        return controller.imageSetButtons[String(id)] ? id : 0;
    }

    function elementButtonId(src) {
        if (!src) {
            return 0;
        }
        if (src.button) {
            return src.buttonId || 0;
        }
        return root.imageSetButtonId(src);
    }

    function elementButtonPanel(src) {
        return src && src.button ? (src.buttonPanel || 0) : root.selectPanel;
    }

    function elementButtonClickEnabled(src) {
        if (!src) {
            return false;
        }
        return src.button
            ? src.buttonClick !== 0
            : root.selectPanel > 0 && root.imageSetButtonId(src) > 0;
    }

    function elementButtonPanelMatches(src) {
        if (!src) {
            return false;
        }
        return src.button ? root.buttonPanelMatches(src) : root.selectPanel > 0;
    }

    function buttonMouseDelta(src, mouseX, width) {
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

    function buttonFrame(src) {
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

    function spriteSkinTime(src, dsts) {
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        return root.isSelectHeldButtonTimer(timer)
            ? (root.hasSelectHeldButtonTimers ? root.selectHeldButtonSkinTime : root.currentSelectHeldButtonSkinTime())
            : root.skinTimeForElement(src, dsts);
    }

    function buttonPanelMatches(src) {
        if (!src || !src.button) {
            return false;
        }
        return root.panelMatches(src.buttonPanel || 0);
    }

    function handleLr2Button(buttonId, delta, panel, soundPlayer, sourceCount) {
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
            selectContext.adjustSortMode(delta, sourceCount || 5);
            selectContext.sortOrFilterChanged();
            optionChanged = true;
            break;
        case 13:
            break;
        case 14:
            root.openSelectPanel(2, false);
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
        case 316:
        case 317:
        case 318: {
            let replayType = buttonId - 315;
            let replayScore = selectContext.replayScoreForType(selectContext.current, replayType);
            if (replayScore) {
                root.selectGoForward(selectContext.current, false, true, replayScore);
            }
            break;
        }
        case 57:
            root.adjustHiSpeedNumber(1, delta);
            optionChanged = true;
            break;
        case 58:
            root.adjustHiSpeedNumber(2, delta);
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
            root.setDpOptionIndex(root.lr2DpOptionIndex + delta);
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
            root.setScoreGraphIndex(root.lr2ScoreGraphIndex + delta);
            optionChanged = true;
            break;
        case 71:
            root.setGhostIndex(root.lr2GhostIndex + delta);
            optionChanged = true;
            break;
        case 72:
            root.setBgaIndex(root.lr2BgaIndex + delta);
            optionChanged = true;
            break;
        case 73:
            root.setBgaSizeIndex(root.lr2BgaSizeIndex + delta);
            optionChanged = true;
            break;
        case 75:
            // Judge auto-adjust is unsupported, so LR2 menus stay at OFF.
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
            root.setBeatorajaTargetIndex(root.lr2BeatorajaTargetIndex + delta);
            optionChanged = true;
            break;
        case 78:
            root.setGaugeAutoShiftIndex(root.lr2GaugeAutoShiftIndex + delta);
            optionChanged = true;
            break;
        case 301:
        case 302:
        case 303:
        case 304:
        case 305:
        case 306:
        case 307:
        case 308:
            // Beatoraja-only options that do not have backing gameplay support yet.
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
            if (buttonId >= 170 && buttonId <= 188) {
                let screen = root.lr2SkinTypeScreenKey(buttonId - 170);
                optionChanged = root.setLr2SkinPreviewScreen(screen);
                break;
            }
            if (buttonId === 190) {
                optionChanged = root.changeLr2SelectedTheme(delta);
                break;
            }
            if (buttonId >= 220 && buttonId <= 229) {
                optionChanged = root.changeLr2SkinSetting(buttonId - 220, delta);
                break;
            }
            if (buttonId >= 230 && buttonId <= 268) {
                // These LR2 controls cover skin/key config, IR, and tag-editor
                // actions that do not have safe backing state here.
                break;
            }
            console.info("LR2 select button " + buttonId + " is not implemented yet");
            break;
        }
        if (optionChanged) {
            root.playOneShot(soundPlayer || optionChangeSound);
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

    function keyUsesPlayer2(key) {
        return controller.player2Keys[String(key)] === true;
    }

    function gameplayOptionSideForKey(key) {
        return root.keyUsesPlayer2(key) && root.battleModeActive() ? 2 : 1;
    }

    function gameplayOptionModifierHeldForKey(key) {
        if (root.keyUsesPlayer2(key) && root.battleModeActive()) {
            return Input.start2 || Input.select2;
        }
        return Input.start1 || Input.select1
            || (!root.battleModeActive() && (Input.start2 || Input.select2));
    }

    function handleLr2GameplayOptionKey(key) {
        if (!root.isGameplayScreen() || !root.gameplayOptionModifierHeldForKey(key)) {
            return false;
        }

        let side = root.gameplayOptionSideForKey(key);
        switch (key) {
        case BmsKey.Col11:
        case BmsKey.Col13:
        case BmsKey.Col15:
        case BmsKey.Col21:
        case BmsKey.Col23:
        case BmsKey.Col25:
            root.adjustHiSpeedNumber(side, -1);
            return true;
        case BmsKey.Col12:
        case BmsKey.Col14:
        case BmsKey.Col22:
        case BmsKey.Col24:
            root.adjustHiSpeedNumber(side, 1);
            return true;
        case BmsKey.Col16:
        case BmsKey.Col26: {
            let vars = root.generalVarsForSide(side);
            if (vars && vars.laneCoverOn) {
                root.adjustLaneCoverRatio(side, -1);
            } else {
                root.adjustHiSpeedNumber(side, 1);
            }
            return true;
        }
        case BmsKey.Col17:
        case BmsKey.Col27: {
            let vars = root.generalVarsForSide(side);
            if (vars && vars.laneCoverOn) {
                root.adjustLaneCoverRatio(side, 1);
            } else {
                root.adjustHiSpeedNumber(side, -1);
            }
            return true;
        }
        default:
            return false;
        }
    }

    function handleLr2GameplayScratchTick(side, up) {
        if (!root.isGameplayScreen()) {
            return false;
        }
        let key = side === 2 ? BmsKey.Col2sUp : BmsKey.Col1sUp;
        if (!root.gameplayOptionModifierHeldForKey(key)) {
            return false;
        }
        root.adjustHiSpeedNumber(root.gameplayOptionSideForKey(key), up ? 1 : -1);
        return true;
    }

    function handleLr2GameplayArrow(key) {
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

    function handleSelectPanelKey(key) {
        if (!root.selectInputReady() || root.selectPanel <= 0) {
            return false;
        }

        let panelBindings = controller.selectPanelKeyBindings[String(root.selectPanel)];
        let binding = panelBindings ? panelBindings[String(key)] : null;
        return binding ? root.triggerSelectPanelButton(binding.buttonId, binding.delta) : false;
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
