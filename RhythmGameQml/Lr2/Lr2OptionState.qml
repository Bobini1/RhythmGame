pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: root

    required property var host

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

    function wrapValue(value: var, count: var) : var {
        return host.wrapValue(value, count);
    }

    readonly property var lr2GaugeLabels: ["ASSISTED EASY", "EASY", "NORMAL", "HARD", "EX HARD", "HAZARD"]
    readonly property var lr2GaugeValues: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
    readonly property var lr2RandomLabels: ["OFF", "MIRROR", "RANDOM", "R-RANDOM", "S-RANDOM", "SPIRAL", "H-RANDOM", "ALL-SCR", "RANDOM+", "S-RAN+"]
    readonly property var lr2RandomValues: [
        NoteOrderAlgorithm.Normal,
        NoteOrderAlgorithm.Mirror,
        NoteOrderAlgorithm.Random,
        NoteOrderAlgorithm.RRandom,
        NoteOrderAlgorithm.SRandom,
        null,
        null,
        null,
        NoteOrderAlgorithm.RandomPlus,
        NoteOrderAlgorithm.SRandomPlus
    ]
    readonly property var lr2RandomSupportedIndexes: [0, 1, 2, 3, 4, 8, 9]
    readonly property var lr2HiSpeedFixLabels: ["OFF", "START BPM", "MAX BPM", "MAIN BPM", "MIN BPM"]
    readonly property var lr2HiSpeedFixValues: [
        HiSpeedFix.Off,
        HiSpeedFix.Start,
        HiSpeedFix.Max,
        HiSpeedFix.Main,
        HiSpeedFix.Min
    ]
    readonly property var lr2DpOptionLabels: ["OFF", "FLIP", "BATTLE", "BATTLE AS"]
    readonly property var lr2DpOptionSupportedIndexes: [0, 1, 2]
    readonly property var lr2GaugeAutoShiftLabels: ["OFF", "CONTINUE", "SURVIVAL->GROOVE", "BEST CLEAR", "SELECT->UNDER"]
    readonly property var lr2GaugeAutoShiftSupportedIndexes: [0, 3, 4]
    readonly property var lr2BottomShiftableGaugeLabels: ["ASSIST EASY", "EASY", "NORMAL"]
    readonly property var lr2BottomShiftableGaugeValues: ["AEASY", "EASY", "NORMAL"]
    readonly property var lr2LnModeLabels: ["LN", "CN", "HCN"]
    readonly property var lr2JudgeAlgorithmLabels: ["LR2", "AC", "BOTTOM PRIORITY"]
    readonly property var lr2BattleLabels: ["OFF", "BATTLE", "SP TO DP"]
    readonly property var lr2TargetLabels: ["GRADE", "BEST SCORE"]
    readonly property var lr2TargetValues: [ScoreTarget.Fraction, ScoreTarget.BestScore]
    readonly property var lr2ClassicTargetLabels: lr2TargetLabels
    readonly property var lr2BeatorajaTargetLabels: [
        "RANK A-",
        "RANK A",
        "RANK A+",
        "RANK AA-",
        "RANK AA",
        "RANK AA+",
        "RANK AAA-",
        "RANK AAA",
        "RANK AAA+",
        "RANK MAX-",
        "MAX",
        "NEXT RANK"
    ]
    readonly property var lr2BeatorajaCompactTargetLabels: [
        "RANK A-",
        "RANK A",
        "RANK A+",
        "RANK AA-",
        "RANK AA",
        "RANK AA+",
        "RANK AAA-",
        "RANK AAA",
        "RANK AAA+",
        "MAX",
        "NEXT RANK"
    ]
    readonly property var lr2BeatorajaTargetFractions: [
        0.6296296296,
        0.6666666667,
        0.7037037037,
        0.7407407407,
        0.7777777778,
        0.8148148148,
        0.8518518519,
        0.8888888889,
        0.9259259259,
        0.9629629630,
        1.0,
        null
    ]
    readonly property var lr2BeatorajaCompactTargetFractions: [
        0.6296296296,
        0.6666666667,
        0.7037037037,
        0.7407407407,
        0.7777777778,
        0.8148148148,
        0.8518518519,
        0.8888888889,
        0.9259259259,
        1.0,
        null
    ]
    readonly property int lr2BeatorajaTargetCount: lr2BeatorajaTargetFractions.length
    readonly property int lr2BeatorajaCompactTargetCount: lr2BeatorajaCompactTargetFractions.length
    readonly property real lr2BeatorajaTargetTolerance: 0.001
    readonly property var lr2BgaSizeLabels: ["NORMAL", "EXTEND"]
    readonly property var lr2GhostLabels: ["OFF", "TYPE A", "TYPE B", "TYPE C"]
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

    function setArrayValue(array: var, index: var, value: var) : var {
        let copy = array.slice();
        copy[index] = value;
        return copy;
    }

    function sliderInitialValue(type: var) : var {
        if (type === 17 || type === 18 || type === 19) {
            return 100;
        }
        if (type === 8) {
            return 0;
        }
        return 50;
    }

    function sliderRawValue(type: var) : var {
        let key = String(type);
        let value = root.lr2SliderValues[key];
        return value === undefined ? root.sliderInitialValue(type) : value;
    }

    function setSliderRawValue(type: var, value: var) : var {
        let key = String(type);
        let rounded = Math.max(0, Math.min(100, Math.round(value)));
        if (root.sliderRawValue(type) === rounded && root.lr2SliderValues[key] !== undefined) {
            return;
        }
        let copy = root.copyObject(root.lr2SliderValues);
        copy[key] = rounded;
        root.lr2SliderValues = copy;
    }

    function lr2SliderNumber(num: var) : var {
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

    readonly property var mainGeneralVarsValue: Rg.profileList && Rg.profileList.mainProfile
            ? Rg.profileList.mainProfile.vars.generalVars
            : null


    function profileForSide(side) {
        if (host.gameplayScreenActive && host.gameplayPlayer) {
            let player = host.gameplayPlayer(side === 2 ? 2 : 1);
            if (player && player.profile) {
                return player.profile;
            }
        }
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

    function indexOfValue(values: var, value: var) : var {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return i;
            }
        }
        return 0;
    }

    function indexOfValueOr(values: var, value: var, fallback: var) : var {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return i;
            }
        }
        return fallback;
    }

    function cycleArrayIndex(index: var, count: var, delta: var) : var {
        return root.wrapValue(index + delta, count);
    }

    function wrappedListValue(values: var, index: var) : var {
        return values[root.wrapValue(index, values.length)];
    }

    function clampedButtonFrame(index: var, sourceCount: var) : var {
        let frame = Math.max(0, Math.floor(index || 0));
        let count = Math.floor(sourceCount || 0);
        return count > 0 ? Math.max(0, Math.min(count - 1, frame)) : frame;
    }

    function optionFrameCount(sourceCount: var, fallbackCount: var) : var {
        let source = Math.floor(sourceCount || 0);
        let fallback = Math.max(0, Math.floor(fallbackCount || 0));
        if (source > 1) {
            return fallback > 0 ? Math.min(source, fallback) : source;
        }
        return fallback;
    }

    function adjustedOptionIndex(current: var, delta: var, count: var) : var {
        let normalizedCount = Math.max(0, Math.floor(count || 0));
        if (normalizedCount <= 0) {
            return -1;
        }
        let normalizedCurrent = Math.floor(current);
        if (isNaN(normalizedCurrent)
                || normalizedCurrent < 0
                || normalizedCurrent >= normalizedCount) {
            return 0;
        }
        return root.wrapValue(normalizedCurrent + delta, normalizedCount);
    }

    function arrayContains(values: var, value: var) : var {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return true;
            }
        }
        return false;
    }

    function cycleSupportedIndex(current: var, delta: var, supportedIndexes: var, count: var) : var {
        if (!supportedIndexes || supportedIndexes.length <= 0 || count <= 0) {
            return 0;
        }
        let currentIndex = root.arrayContains(supportedIndexes, current)
            ? current
            : supportedIndexes[0];
        if (!delta) {
            return currentIndex;
        }
        let direction = delta < 0 ? -1 : 1;
        let candidate = root.wrapValue(current + direction, count);
        for (let guard = 0; guard < count; ++guard) {
            if (root.arrayContains(supportedIndexes, candidate)) {
                return candidate;
            }
            candidate = root.wrapValue(candidate + direction, count);
        }
        return currentIndex;
    }

    readonly property int lr2GaugeIndexP1: {
        let vars = root.generalVarsForSide(1);
        return vars ? root.indexOfValue(root.lr2GaugeValues, vars.gaugeType) : 0;
    }
    readonly property int lr2GaugeIndexP2: {
        let vars = root.generalVarsForSide(2);
        return vars ? root.indexOfValue(root.lr2GaugeValues, vars.gaugeType) : 0;
    }

    function setGaugeIndex(side: var, index: var) : void {
        let vars = root.generalVarsForSide(side);
        if (vars) {
            vars.gaugeType = root.wrappedListValue(root.lr2GaugeValues, index);
        }
    }

    function lr2GaugeButtonFrame(side: var, sourceCount: var) : var {
        return root.clampedButtonFrame(side === 2 ? root.lr2GaugeIndexP2 : root.lr2GaugeIndexP1, sourceCount);
    }

    function setGaugeButtonIndex(side: var, index: var, sourceCount: var) : void {
        root.setGaugeIndex(side, root.clampedButtonFrame(index, sourceCount));
    }

    function adjustGaugeButtonIndex(side: var, delta: var, sourceCount: var) : void {
        let current = side === 2 ? root.lr2GaugeIndexP2 : root.lr2GaugeIndexP1;
        root.setGaugeIndex(side, current + delta);
    }

    function lr2GaugeText(side: var, sourceCount: var) : var {
        let index = side === 2 ? root.lr2GaugeIndexP2 : root.lr2GaugeIndexP1;
        return root.lr2GaugeLabels[root.clampedButtonFrame(index, root.lr2GaugeLabels.length)];
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

    function setRandomIndex(side: var, index: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let current = side === 2 ? root.lr2RandomIndexP2 : root.lr2RandomIndexP1;
        let normalized = root.cycleSupportedIndex(
            current,
            index - current,
            root.lr2RandomSupportedIndexes,
            root.lr2RandomValues.length);
        let value = root.lr2RandomValues[normalized] || NoteOrderAlgorithm.Normal;
        if (side === 2 && !Rg.profileList.battleActive) {
            vars.noteOrderAlgorithmP2 = value;
        } else {
            vars.noteOrderAlgorithm = value;
        }
    }

    function randomSupportedIndexesForSourceCount(sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        if (count <= 0 || count >= root.lr2RandomValues.length) {
            return root.lr2RandomSupportedIndexes;
        }
        let result = [];
        for (let i = 0; i < root.lr2RandomSupportedIndexes.length; ++i) {
            let index = root.lr2RandomSupportedIndexes[i];
            if (index < count) {
                result.push(index);
            }
        }
        return result.length > 0 ? result : [0];
    }

    function lr2RandomButtonFrame(side: var, sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        let current = side === 2 ? root.lr2RandomIndexP2 : root.lr2RandomIndexP1;
        let supported = root.randomSupportedIndexesForSourceCount(count);
        return root.clampedButtonFrame(
            root.arrayContains(supported, current) ? current : supported[0],
            count);
    }

    function setRandomButtonIndex(side: var, index: var, sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        let current = side === 2 ? root.lr2RandomIndexP2 : root.lr2RandomIndexP1;
        let normalized = root.cycleSupportedIndex(
            current,
            index - current,
            root.randomSupportedIndexesForSourceCount(count),
            count > 0 ? count : root.lr2RandomValues.length);
        root.setRandomIndex(side, normalized);
    }

    function adjustRandomButtonIndex(side: var, delta: var, sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        let current = side === 2 ? root.lr2RandomIndexP2 : root.lr2RandomIndexP1;
        let normalized = root.cycleSupportedIndex(
            current,
            delta,
            root.randomSupportedIndexesForSourceCount(count),
            count > 0 ? count : root.lr2RandomValues.length);
        root.setRandomIndex(side, normalized);
    }

    function lr2RandomText(side: var, sourceCount: var) : var {
        let frame = root.lr2RandomButtonFrame(side, sourceCount);
        return root.lr2RandomLabels[root.clampedButtonFrame(frame, root.lr2RandomLabels.length)];
    }

    readonly property int lr2HidSudIndexP1: {
        return root.lr2HidSudIndex(1);
    }
    readonly property int lr2HidSudIndexP2: {
        return root.lr2HidSudIndex(2);
    }

    function lr2HidSudIndex(side: var) : var {
        let normalizedSide = side === 2 ? 2 : 1;
        let vars = root.generalVarsForSide(normalizedSide);
        if (!vars) {
            return 0;
        }
        return (vars.hiddenOn ? 1 : 0) + (vars.laneCoverOn ? 2 : 0);
    }

    function setHidSudIndex(side: var, index: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let normalized = root.wrapValue(index, root.lr2HidSudLabels.length);
        vars.hiddenOn = (normalized & 1) !== 0;
        vars.laneCoverOn = (normalized & 2) !== 0;
    }

    readonly property int lr2HiSpeedFixIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars ? root.indexOfValue(root.lr2HiSpeedFixValues, vars.hiSpeedFix) : 0;
    }

    function setHiSpeedFixIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.hiSpeedFix = root.wrappedListValue(root.lr2HiSpeedFixValues, index);
        }
    }

    readonly property int lr2BattleIndex: {
        let vars = root.mainGeneralVarsValue;
        if (Rg.profileList.battleActive) {
            return 1;
        }
        if (vars && vars.dpOptions === DpOptions.Battle) {
            return 2;
        }
        return 0;
    }

    function ensureBattleProfiles() : var {
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

    function setBattleIndex(index: var) : var {
        let vars = root.mainGeneralVarsValue;
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

    readonly property int lr2DpOptionIndex: {
        let vars = root.mainGeneralVarsValue;
        if (Rg.profileList.battleActive || (vars && vars.dpOptions === DpOptions.Battle)) {
            return 2;
        }
        if (vars && vars.dpOptions === DpOptions.Flip) {
            return 1;
        }
        return 0;
    }

    function setDpOptionIndex(index: var) : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return;
        }
        let normalized = root.cycleSupportedIndex(
            root.lr2DpOptionIndex,
            index - root.lr2DpOptionIndex,
            root.lr2DpOptionSupportedIndexes,
            root.lr2DpOptionLabels.length);
        Rg.profileList.battleActive = false;
        if (normalized === 1) {
            vars.dpOptions = DpOptions.Flip;
        } else if (normalized === 2) {
            vars.dpOptions = DpOptions.Battle;
        } else {
            vars.dpOptions = DpOptions.Off;
        }
    }

    readonly property int lr2FlipIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars && vars.dpOptions === DpOptions.Flip ? 1 : 0;
    }

    function setFlipIndex(index: var) : var {
        let vars = root.mainGeneralVarsValue;
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
        let vars = root.mainGeneralVarsValue;
        return vars && vars.laneCoverOn ? 1 : 0;
    }

    function setLaneCoverIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.laneCoverOn = root.wrapValue(index, 2) === 1;
        }
    }

    function toggleLaneCover(side: var) : void {
        let vars = root.generalVarsForSide(side);
        if (vars) {
            vars.laneCoverOn = !vars.laneCoverOn;
        }
    }

    readonly property int lr2BgaIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars && !vars.bgaOn ? 1 : 0;
    }

    readonly property int lr2BeatorajaBgaIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars && !vars.bgaOn ? 2 : 0;
    }

    function setBgaIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.bgaOn = root.wrapValue(index, 2) === 0;
        }
    }

    readonly property int lr2BgaSizeIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars ? Math.max(0, Math.min(1, vars.bgaSize || 0)) : 0;
    }

    function setBgaSizeIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.bgaSize = root.wrapValue(index, root.lr2BgaSizeLabels.length);
        }
    }

    readonly property int lr2GaugeAutoShiftIndex: {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return 0;
        }
        if (vars.gaugeMode === GaugeMode.Best) {
            return 3;
        }
        if (vars.gaugeMode === GaugeMode.SelectToUnder) {
            return 4;
        }
        return 0;
    }

    function setGaugeAutoShiftIndex(index: var) : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return;
        }
        let normalized = root.cycleSupportedIndex(
            root.lr2GaugeAutoShiftIndex,
            index - root.lr2GaugeAutoShiftIndex,
            root.lr2GaugeAutoShiftSupportedIndexes,
            root.lr2GaugeAutoShiftLabels.length);
        if (normalized === 3) {
            vars.gaugeMode = GaugeMode.Best;
        } else if (normalized === 4) {
            vars.gaugeMode = GaugeMode.SelectToUnder;
        } else {
            vars.gaugeMode = GaugeMode.Exclusive;
        }
    }

    readonly property int lr2BottomShiftableGaugeIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars ? root.indexOfValue(root.lr2BottomShiftableGaugeValues, vars.bottomShiftableGauge) : 0;
    }

    function setBottomShiftableGaugeIndex(index: var) : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return false;
        }
        let normalized = root.wrapValue(index, root.lr2BottomShiftableGaugeValues.length);
        let value = root.lr2BottomShiftableGaugeValues[normalized];
        if (vars.bottomShiftableGauge === value) {
            return false;
        }
        vars.bottomShiftableGauge = value;
        return true;
    }

    readonly property int lr2LnModeIndex: 0
    function setLnModeIndex(index: var) : var {
        return false;
    }

    function beatorajaAssistOptionFrame(buttonId: var) : var {
        return 0;
    }

    function toggleBeatorajaAssistOption(buttonId: var) : var {
        return false;
    }

    readonly property int lr2JudgeAlgorithmIndex: 0
    function setJudgeAlgorithmIndex(index: var) : var {
        return false;
    }

    readonly property int lr2ScoreGraphIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars && vars.scoreGraphEnabled === false ? 0 : 1;
    }

    function setScoreGraphIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.scoreGraphEnabled = root.wrapValue(index, 2) === 1;
        }
    }

    readonly property int lr2GhostIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0;
    }

    function setGhostIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.ghostPosition = root.wrapValue(index, root.lr2GhostLabels.length);
        }
    }

    readonly property bool lr2BgaEnabled: {
        let vars = root.mainGeneralVarsValue;
        return !vars || vars.bgaOn !== false;
    }

    readonly property int lr2ScoreTargetIndex: {
        let vars = root.mainGeneralVarsValue;
        return vars ? root.indexOfValueOr(root.lr2TargetValues, vars.scoreTarget, -1) : -1;
    }

    function setScoreTargetIndex(index: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.scoreTarget = root.wrappedListValue(root.lr2TargetValues, index);
        }
    }

    readonly property int lr2ClassicTargetIndex: lr2ScoreTargetIndex
    function setClassicTargetIndex(index: var) : void {
        root.setScoreTargetIndex(index);
    }

    readonly property int lr2BeatorajaTargetIndex: root.beatorajaTargetIndexForFractions(root.lr2BeatorajaTargetFractions)

    function beatorajaUsesCompactTargetFrames(sourceCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        return count === root.lr2BeatorajaCompactTargetCount
            && count < root.lr2BeatorajaTargetCount;
    }

    function beatorajaTargetLabelsForSourceCount(sourceCount: var) : var {
        return root.beatorajaUsesCompactTargetFrames(sourceCount)
            ? root.lr2BeatorajaCompactTargetLabels
            : root.lr2BeatorajaTargetLabels;
    }

    function beatorajaTargetFractionsForSourceCount(sourceCount: var) : var {
        return root.beatorajaUsesCompactTargetFrames(sourceCount)
            ? root.lr2BeatorajaCompactTargetFractions
            : root.lr2BeatorajaTargetFractions;
    }

    function beatorajaTargetIndexForFractions(fractions: var) : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return -1;
        }
        if (vars.scoreTarget === ScoreTarget.NextRank) {
            for (let i = 0; i < fractions.length; ++i) {
                if (fractions[i] === null || fractions[i] === undefined) {
                    return i;
                }
            }
            return -1;
        }
        if (vars.scoreTarget !== ScoreTarget.Fraction) {
            return -1;
        }
        let fraction = Number(vars.targetScoreFraction || 0);
        let bestIndex = 0;
        let bestDistance = Number.MAX_VALUE;
        for (let i = 0; i < fractions.length; ++i) {
            let targetFraction = fractions[i];
            if (targetFraction === null || targetFraction === undefined) {
                continue;
            }
            let distance = Math.abs(fraction - targetFraction);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }
        return bestDistance <= root.lr2BeatorajaTargetTolerance ? bestIndex : -1;
    }

    function setBeatorajaTargetIndex(index: var, sourceCount: var) : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return;
        }
        let fractions = root.beatorajaTargetFractionsForSourceCount(sourceCount);
        let count = fractions.length;
        if (count <= 0) {
            return;
        }
        let normalized = root.wrapValue(index, count);
        if (fractions[normalized] === null || fractions[normalized] === undefined) {
            vars.scoreTarget = ScoreTarget.NextRank;
            return;
        }
        vars.scoreTarget = ScoreTarget.Fraction;
        vars.targetScoreFraction = fractions[normalized];
    }

    function targetUsesBeatorajaFrames(sourceCount: var) : var {
        return root.host && root.host.lr2SkinUsesBeatorajaSemantics === true;
    }

    function targetButtonFrameCount(sourceCount: var) : var {
        if (!root.targetUsesBeatorajaFrames(sourceCount)) {
            return root.optionFrameCount(sourceCount, root.lr2TargetLabels.length);
        }
        let source = Math.floor(sourceCount || 0);
        let targetCount = root.beatorajaTargetFractionsForSourceCount(sourceCount).length;
        if (source > 1) {
            return Math.min(source, targetCount);
        }
        return source === 1 ? 1 : targetCount;
    }

    function lr2TargetButtonFrame(sourceCount: var) : var {
        let frame = root.targetUsesBeatorajaFrames(sourceCount)
            ? root.beatorajaTargetIndexForFractions(
                  root.beatorajaTargetFractionsForSourceCount(sourceCount))
            : root.lr2ScoreTargetIndex;
        let count = root.targetButtonFrameCount(sourceCount);
        return frame >= 0 && frame < count ? frame : -1;
    }

    function setTargetButtonIndex(index: var, sourceCount: var) : void {
        if (root.targetUsesBeatorajaFrames(sourceCount)) {
            root.setBeatorajaTargetIndex(index, sourceCount);
        } else {
            root.setScoreTargetIndex(index);
        }
    }

    function adjustTargetButtonIndex(delta: var, sourceCount: var) : void {
        let count = root.targetButtonFrameCount(sourceCount);
        let next = root.adjustedOptionIndex(root.lr2TargetButtonFrame(sourceCount), delta, count);
        if (next >= 0) {
            root.setTargetButtonIndex(next, sourceCount);
        }
    }

    function lr2TargetText(sourceCount: var) : var {
        let frame = root.lr2TargetButtonFrame(sourceCount);
        if (frame < 0) {
            return "";
        }
        if (root.targetUsesBeatorajaFrames(sourceCount)) {
            let labels = root.beatorajaTargetLabelsForSourceCount(sourceCount);
            return frame < labels.length
                ? labels[frame]
                : "";
        }
        return frame < root.lr2TargetLabels.length ? root.lr2TargetLabels[frame] : "";
    }

    function lr2TargetNameText(textId: var) : var {
        if (!root.targetUsesBeatorajaFrames(0)) {
            return "";
        }
        let id = Math.floor(textId || 0);
        if (id < 200 || id > 219) {
            return "";
        }
        let current = root.lr2BeatorajaTargetIndex;
        let count = root.lr2BeatorajaTargetCount;
        if (current < 0 || count <= 0) {
            return "";
        }
        let offset = id < 210 ? id - 210 : id - 209;
        return root.lr2BeatorajaTargetLabels[root.wrapValue(current + offset, count)];
    }

    readonly property int lr2TargetPercent: {
        let vars = root.mainGeneralVarsValue;
        return vars ? Math.floor((vars.targetScoreFraction || 0) * 100) : 80;
    }

    function setTargetPercent(percent: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.targetScoreFraction = Math.max(0, Math.min(1, Math.floor(percent) / 100));
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

    function setHiSpeedNumber(side: var, value: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let clamped = Math.max(1, Math.min(999, value));
        vars.noteScreenTimeMillis = 100000 / clamped;
    }

    function nextLr2HiSpeedNumber(current: var, steps: var) : var {
        if (steps === 0) {
            return current;
        }
        let value = isFinite(current) ? current : 100;
        let direction = steps < 0 ? -1 : 1;
        let count = Math.max(1, Math.abs(steps || 1));
        for (let i = 0; i < count; ++i) {
            if (direction > 0) {
                value = value % 10 === 0
                    ? value + 10
                    : Math.ceil(value / 10) * 10;
                if (value > 900) {
                    value = 10;
                }
            } else {
                value = value % 10 === 0
                    ? value - 10
                    : Math.floor(value / 10) * 10;
                if (value < 10) {
                    value = 900;
                }
            }
        }
        return value;
    }

    function adjustHiSpeedNumber(side: var, steps: var) : void {
        let current = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        root.setHiSpeedNumber(side, root.nextLr2HiSpeedNumber(current, steps));
    }

    function adjustDurationNumber(side: var, steps: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let current = vars.noteScreenTimeMillis > 0 ? vars.noteScreenTimeMillis : 1000;
        vars.noteScreenTimeMillis = Math.max(1, Math.min(10000, Math.round(current + (steps || 1))));
    }

    function adjustScratchDurationNumber(side: var, amount: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let current = vars.noteScreenTimeMillis > 0 ? vars.noteScreenTimeMillis : 1000;
        vars.noteScreenTimeMillis = current + current / 1000 * amount;
    }

    function adjustScratchCoverNumber(side: var, amount: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let value = 0.0005 * amount;
        if (vars.laneCoverOn) {
            vars.laneCoverRatio = (vars.laneCoverRatio || 0) + value;
        } else if (vars.liftOn) {
            vars.liftRatio = (vars.liftRatio || 0) + value;
        }
    }

    function adjustLaneCoverRatio(side: var, steps: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        vars.laneCoverRatio = Math.max(0, Math.min(1, (vars.laneCoverRatio || 0) + steps * 0.005));
    }

    function adjustGameplayCoverValue(side: var, steps: var, changeLift: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let value = (steps || 0) * 0.005;
        if (vars.laneCoverOn || (!vars.liftOn && !vars.hiddenOn)) {
            vars.laneCoverRatio = Math.max(0, Math.min(1, (vars.laneCoverRatio || 0) + value));
        } else if (vars.liftOn && (!vars.hiddenOn || changeLift)) {
            vars.liftRatio = Math.max(0, Math.min(1, (vars.liftRatio || 0) - value));
        } else {
            vars.hiddenRatio = Math.max(0, Math.min(1, (vars.hiddenRatio || 0) - value));
        }
    }

    function adjustOffset(delta: var) : void {
        let vars = root.mainGeneralVarsValue;
        if (vars) {
            vars.offset = Math.max(-500, Math.min(500, (vars.offset || 0) + delta));
        }
    }

    function clearStatusOption() : var {
        let vars = root.mainGeneralVarsValue;
        if (!vars) {
            return 64;
        }
        if (vars.gaugeMode === GaugeMode.Best) {
            return 66;
        }
        let gauge = String(vars.gaugeType || "").toUpperCase();
        if (gauge === "AEASY" || gauge === "EASY") {
            return 63;
        }
        if (gauge === "NORMAL" || gauge === "DAN") {
            return 64;
        }
        if (gauge === "HARD" || gauge === "EXHARD" || gauge === "EXDAN" || gauge === "EXHARDDAN") {
            return 65;
        }
        if (gauge === "FC" || gauge === "PERFECT" || gauge === "MAX") {
            return 66;
        }
        return 64;
    }

    function onlineUserId(data: var) : var {
        return data ? Number(data.userId || 0) : 0;
    }

    function isLoggedIn() : var {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        let vars = root.mainGeneralVarsValue;
        let provider = vars ? vars.rankingProvider : OnlineRankingModel.RhythmGame;
        if (provider === OnlineRankingModel.LR2IR) {
            return true;
        }
        if (!profile) {
            return false;
        }
        if (provider === OnlineRankingModel.Tachi) {
            return root.onlineUserId(profile.tachiData) !== 0;
        }
        return root.onlineUserId(profile.onlineUserData) !== 0;
    }

}
