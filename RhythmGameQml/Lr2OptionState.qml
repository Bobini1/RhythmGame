pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    property int revision: 0

    function usesBeatorajaOptionSemantics() : var {
        return !!root.host && root.host.lr2SkinUsesBeatorajaSemantics === true;
    }

    function bumpRevision() : void {
        ++root.revision;
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

    function wrapValue(value: var, count: var) : var {
        return host.wrapValue(value, count);
    }

    readonly property var lr2GaugeLabels: ["ASSISTED EASY", "EASY", "NORMAL", "HARD", "EX HARD", "HAZARD"]
    readonly property var lr2GaugeValues: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
    readonly property var lr2ClassicGaugeLabels: ["NORMAL", "HARD", "EX HARD", "EASY", "P-ATTACK", "G-ATTACK"]
    readonly property var lr2ClassicGaugeValues: ["NORMAL", "HARD", "EXHARD", "EASY", "FC", null]
    readonly property var lr2ClassicGaugeSupportedIndexes: [0, 1, 2, 3, 4]
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
    readonly property var lr2ClassicRandomLabels: ["OFF", "MIRROR", "RANDOM", "S-RANDOM", "H-RANDOM", "ALL SCRATCH"]
    readonly property var lr2ClassicRandomValues: [
        NoteOrderAlgorithm.Normal,
        NoteOrderAlgorithm.Mirror,
        NoteOrderAlgorithm.Random,
        NoteOrderAlgorithm.SRandom,
        NoteOrderAlgorithm.RRandom,
        null
    ]
    readonly property var lr2ClassicRandomSupportedIndexes: [0, 1, 2, 3, 4]
    readonly property int noSelectedButtonFrame: -2
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
    readonly property var lr2TargetSupportedIndexes: [0, 1]
    readonly property var lr2ClassicTargetLabels: [
        "NO TARGET",
        "MY BEST",
        "RANK AAA",
        "RANK AA",
        "RANK A",
        "DEFAULT",
        "IR TOP",
        "IR NEXT",
        "IR AVERAGE"
    ]
    readonly property var lr2ClassicTargetFractions: [0.0, null, 8.0 / 9.0, 7.0 / 9.0, 6.0 / 9.0]
    readonly property var lr2ClassicTargetSupportedIndexes: [1, 2, 3, 4, 5]
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
        "MAX"
    ]
    readonly property var lr2BeatorajaTargetFractions: [
        17.0 / 27.0,
        18.0 / 27.0,
        19.0 / 27.0,
        20.0 / 27.0,
        21.0 / 27.0,
        22.0 / 27.0,
        23.0 / 27.0,
        24.0 / 27.0,
        25.0 / 27.0,
        26.0 / 27.0,
        1.0
    ]
    readonly property var lr2BeatorajaTargetSupportedIndexes: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
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

    function indexOfValue(values: var, value: var) : var {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return i;
            }
        }
        return 0;
    }

    function indexOfValueOrNone(values: var, value: var) : var {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] !== null && values[i] === value) {
                return i;
            }
        }
        return root.noSelectedButtonFrame;
    }

    function cycleArrayIndex(index: var, count: var, delta: var) : var {
        return root.wrapValue(index + delta, count);
    }

    function wrappedListValue(values: var, index: var) : var {
        return values[root.wrapValue(index, values.length)];
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
        if (current === root.noSelectedButtonFrame) {
            return supportedIndexes[0];
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

    function sourceFrameCount(sourceCount: var, defaultCount: var) : var {
        let count = Math.floor(sourceCount || 0);
        if (count > 0) {
            return count;
        }
        return Math.max(0, Math.floor(defaultCount || 0));
    }

    function supportedIndexesForSource(sourceCount: var, supportedIndexes: var, defaultCount: var) : var {
        let count = root.sourceFrameCount(sourceCount, defaultCount);
        let result = [];
        if (!supportedIndexes) {
            return result;
        }
        for (let i = 0; i < supportedIndexes.length; ++i) {
            let index = Math.floor(supportedIndexes[i]);
            if (index >= 0 && (count <= 0 || index < count)) {
                result.push(index);
            }
        }
        return result;
    }

    function supportedFrameOrNone(index: var, sourceCount: var, supportedIndexes: var, defaultCount: var) : var {
        let rounded = Math.floor(index);
        if (!isFinite(rounded) || rounded < 0) {
            return root.noSelectedButtonFrame;
        }
        let count = root.sourceFrameCount(sourceCount, defaultCount);
        if (count > 0 && rounded >= count) {
            return root.noSelectedButtonFrame;
        }
        return root.arrayContains(root.supportedIndexesForSource(count, supportedIndexes, defaultCount), rounded)
            ? rounded
            : root.noSelectedButtonFrame;
    }

    function normalizeSupportedFrame(index: var, sourceCount: var, supportedIndexes: var, defaultCount: var) : var {
        let supported = root.supportedIndexesForSource(sourceCount, supportedIndexes, defaultCount);
        if (supported.length <= 0) {
            return 0;
        }
        let rounded = Math.floor(index);
        return root.arrayContains(supported, rounded) ? rounded : supported[0];
    }

    function cycleSupportedFrame(current: var, delta: var, sourceCount: var, supportedIndexes: var, defaultCount: var) : var {
        let count = Math.max(1, root.sourceFrameCount(sourceCount, defaultCount));
        let supported = root.supportedIndexesForSource(count, supportedIndexes, count);
        if (supported.length <= 0) {
            return 0;
        }
        return root.cycleSupportedIndex(current, delta, supported, count);
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

    function currentGaugeValue(side: var) : var {
        let vars = root.generalVarsForSide(side);
        return vars ? vars.gaugeType : "NORMAL";
    }

    function gaugeButtonUsesClassicSource(sourceCount: var) : var {
        return !root.usesBeatorajaOptionSemantics();
    }

    function gaugeButtonSupportedIndexes(sourceCount: var) : var {
        return root.gaugeButtonUsesClassicSource(sourceCount)
            ? root.lr2ClassicGaugeSupportedIndexes
            : [0, 1, 2, 3, 4, 5];
    }

    function gaugeButtonDefaultCount(sourceCount: var) : var {
        return root.gaugeButtonUsesClassicSource(sourceCount)
            ? root.lr2ClassicGaugeValues.length
            : root.lr2GaugeValues.length;
    }

    function lr2ClassicGaugeIndex(side: var) : var {
        return root.indexOfValueOrNone(root.lr2ClassicGaugeValues, root.currentGaugeValue(side));
    }

    function lr2GaugeButtonFrame(side: var, sourceCount: var) : var {
        let classicSource = root.gaugeButtonUsesClassicSource(sourceCount);
        let index = classicSource
            ? root.lr2ClassicGaugeIndex(side)
            : (side === 2 ? root.lr2GaugeIndexP2 : root.lr2GaugeIndexP1);
        return root.supportedFrameOrNone(
            index,
            sourceCount,
            root.gaugeButtonSupportedIndexes(sourceCount),
            root.gaugeButtonDefaultCount(sourceCount));
    }

    function setClassicGaugeIndex(side: var, index: var, sourceCount: var) : void {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let normalized = root.normalizeSupportedFrame(
            index,
            sourceCount,
            root.lr2ClassicGaugeSupportedIndexes,
            root.lr2ClassicGaugeValues.length);
        vars.gaugeType = root.lr2ClassicGaugeValues[normalized] || "NORMAL";
    }

    function setGaugeButtonIndex(side: var, index: var, sourceCount: var) : var {
        if (root.gaugeButtonUsesClassicSource(sourceCount)) {
            root.setClassicGaugeIndex(side, index, sourceCount);
        } else {
            root.setGaugeIndex(
                side,
                root.normalizeSupportedFrame(
                    index,
                    sourceCount,
                    root.gaugeButtonSupportedIndexes(sourceCount),
                    root.lr2GaugeValues.length));
        }
    }

    function adjustGaugeButtonIndex(side: var, delta: var, sourceCount: var) : var {
        let index = root.cycleSupportedFrame(
            root.lr2GaugeButtonFrame(side, sourceCount),
            delta,
            sourceCount,
            root.gaugeButtonSupportedIndexes(sourceCount),
            root.gaugeButtonDefaultCount(sourceCount));
        root.setGaugeButtonIndex(side, index, sourceCount);
    }

    function lr2GaugeText(side: var, sourceCount: var) : var {
        let index = root.lr2GaugeButtonFrame(side, sourceCount);
        if (index < 0) {
            return "";
        }
        return root.gaugeButtonUsesClassicSource(sourceCount)
            ? root.wrappedListValue(root.lr2ClassicGaugeLabels, index)
            : root.wrappedListValue(root.lr2GaugeLabels, index);
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

    function currentRandomValue(side: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return NoteOrderAlgorithm.Normal;
        }
        return side === 2 && !Rg.profileList.battleActive
            ? vars.noteOrderAlgorithmP2
            : vars.noteOrderAlgorithm;
    }

    function randomButtonUsesClassicSource(sourceCount: var) : var {
        return !root.usesBeatorajaOptionSemantics();
    }

    function randomButtonSupportedIndexes(sourceCount: var) : var {
        return root.randomButtonUsesClassicSource(sourceCount)
            ? root.lr2ClassicRandomSupportedIndexes
            : root.lr2RandomSupportedIndexes;
    }

    function randomButtonDefaultCount(sourceCount: var) : var {
        return root.randomButtonUsesClassicSource(sourceCount)
            ? root.lr2ClassicRandomValues.length
            : root.lr2RandomValues.length;
    }

    function lr2BeatorajaRandomIndex(side: var) : var {
        return root.indexOfValueOrNone(root.lr2RandomValues, root.currentRandomValue(side));
    }

    function lr2ClassicRandomIndex(side: var) : var {
        return root.indexOfValueOrNone(root.lr2ClassicRandomValues, root.currentRandomValue(side));
    }

    function setClassicRandomIndex(side: var, index: var, sourceCount: var) : var {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let normalized = root.normalizeSupportedFrame(
            index,
            sourceCount,
            root.lr2ClassicRandomSupportedIndexes,
            root.lr2ClassicRandomValues.length);
        let value = root.lr2ClassicRandomValues[normalized] || NoteOrderAlgorithm.Normal;
        if (side === 2 && !Rg.profileList.battleActive) {
            vars.noteOrderAlgorithmP2 = value;
        } else {
            vars.noteOrderAlgorithm = value;
        }
    }

    function lr2RandomButtonFrame(side: var, sourceCount: var) : var {
        let classicSource = root.randomButtonUsesClassicSource(sourceCount);
        let index = classicSource ? root.lr2ClassicRandomIndex(side) : root.lr2BeatorajaRandomIndex(side);
        return root.supportedFrameOrNone(
            index,
            sourceCount,
            root.randomButtonSupportedIndexes(sourceCount),
            root.randomButtonDefaultCount(sourceCount));
    }

    function setRandomButtonIndex(side: var, index: var, sourceCount: var) : var {
        if (root.randomButtonUsesClassicSource(sourceCount)) {
            root.setClassicRandomIndex(side, index, sourceCount);
        } else {
            root.setRandomIndex(
                side,
                root.normalizeSupportedFrame(
                    index,
                    sourceCount,
                    root.lr2RandomSupportedIndexes,
                    root.lr2RandomValues.length));
        }
    }

    function adjustRandomButtonIndex(side: var, delta: var, sourceCount: var) : var {
        let index = root.cycleSupportedFrame(
            root.lr2RandomButtonFrame(side, sourceCount),
            delta,
            sourceCount,
            root.randomButtonSupportedIndexes(sourceCount),
            root.randomButtonDefaultCount(sourceCount));
        root.setRandomButtonIndex(side, index, sourceCount);
    }

    function lr2RandomText(side: var, sourceCount: var) : var {
        let index = root.lr2RandomButtonFrame(side, sourceCount);
        if (index < 0) {
            return "";
        }
        return root.randomButtonUsesClassicSource(sourceCount)
            ? root.wrappedListValue(root.lr2ClassicRandomLabels, index)
            : root.wrappedListValue(root.lr2RandomLabels, index);
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
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2HiSpeedFixValues, vars.hiSpeedFix) : 0;
    }

    function setHiSpeedFixIndex(index: var) : void {
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

    readonly property int lr2DpOptionIndex: {
        let vars = root.mainGeneralVars();
        if (Rg.profileList.battleActive || (vars && vars.dpOptions === DpOptions.Battle)) {
            return 2;
        }
        if (vars && vars.dpOptions === DpOptions.Flip) {
            return 1;
        }
        return 0;
    }

    function setDpOptionIndex(index: var) : var {
        let vars = root.mainGeneralVars();
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
        let vars = root.mainGeneralVars();
        return vars && vars.dpOptions === DpOptions.Flip ? 1 : 0;
    }

    function setFlipIndex(index: var) : var {
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

    function setLaneCoverIndex(index: var) : void {
        let vars = root.mainGeneralVars();
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
        let vars = root.mainGeneralVars();
        return vars && !vars.bgaOn ? 1 : 0;
    }

    readonly property int lr2BeatorajaBgaIndex: {
        let vars = root.mainGeneralVars();
        return vars && !vars.bgaOn ? 2 : 0;
    }

    function setBgaIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.bgaOn = root.wrapValue(index, 2) === 0;
        }
    }

    readonly property int lr2BgaSizeIndex: {
        let vars = root.mainGeneralVars();
        return vars ? Math.max(0, Math.min(1, vars.bgaSize || 0)) : 0;
    }

    function setBgaSizeIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.bgaSize = root.wrapValue(index, root.lr2BgaSizeLabels.length);
        }
    }

    readonly property int lr2GaugeAutoShiftIndex: {
        let vars = root.mainGeneralVars();
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
        let vars = root.mainGeneralVars();
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
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2BottomShiftableGaugeValues, vars.bottomShiftableGauge) : 0;
    }

    function setBottomShiftableGaugeIndex(index: var) : var {
        let vars = root.mainGeneralVars();
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

    readonly property int lr2JudgeAlgorithmIndex: 0
    function setJudgeAlgorithmIndex(index: var) : var {
        return false;
    }

    readonly property int lr2ScoreGraphIndex: {
        let vars = root.mainGeneralVars();
        return vars && vars.scoreGraphEnabled === false ? 0 : 1;
    }

    function setScoreGraphIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.scoreGraphEnabled = root.wrapValue(index, 2) === 1;
        }
    }

    readonly property int lr2GhostIndex: {
        let vars = root.mainGeneralVars();
        return vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0;
    }

    function setGhostIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.ghostPosition = root.wrapValue(index, root.lr2GhostLabels.length);
        }
    }

    function lr2BgaEnabled() : var {
        let vars = root.mainGeneralVars();
        return !vars || vars.bgaOn !== false;
    }

    readonly property int lr2ScoreTargetIndex: {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValue(root.lr2TargetValues, vars.scoreTarget) : 0;
    }

    function setScoreTargetIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.scoreTarget = root.wrappedListValue(root.lr2TargetValues, index);
        }
    }

    property int lr2ClassicTargetIndex: root.classicTargetIndexFromVars()
    property bool lr2ClassicUserTargetActive: false

    function classicTargetIndexFromVars() : var {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return 1;
        }
        if (vars.scoreTarget === ScoreTarget.BestScore) {
            return 1;
        }
        if (vars.scoreTarget !== ScoreTarget.Fraction) {
            return root.noSelectedButtonFrame;
        }
        let fraction = vars.targetScoreFraction || 0;
        if (Math.abs(fraction - root.lr2ClassicTargetFractions[2]) < 0.005) {
            return 2;
        }
        if (Math.abs(fraction - root.lr2ClassicTargetFractions[3]) < 0.005) {
            return 3;
        }
        if (Math.abs(fraction - root.lr2ClassicTargetFractions[4]) < 0.005) {
            return 4;
        }
        return fraction > 0 ? 5 : 0;
    }

    function setClassicTargetIndex(index: var) : void {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return;
        }
        let normalized = root.normalizeSupportedFrame(
            index,
            root.lr2ClassicTargetLabels.length,
            root.lr2ClassicTargetSupportedIndexes,
            root.lr2ClassicTargetLabels.length);
        root.lr2ClassicTargetIndex = normalized;
        root.lr2ClassicUserTargetActive = normalized === 5;
        if (normalized === 1) {
            vars.scoreTarget = ScoreTarget.BestScore;
            return;
        }
        vars.scoreTarget = ScoreTarget.Fraction;
        if (normalized >= 0 && normalized < root.lr2ClassicTargetFractions.length
                && root.lr2ClassicTargetFractions[normalized] !== null) {
            vars.targetScoreFraction = root.lr2ClassicTargetFractions[normalized];
        } else {
            vars.targetScoreFraction = normalized === 0 ? 0 : root.lr2DefaultTargetFraction();
        }
    }

    function lr2ClassicTargetCurrentFrame() : var {
        let vars = root.mainGeneralVars();
        if (root.lr2ClassicUserTargetActive
                && vars
                && vars.scoreTarget === ScoreTarget.Fraction) {
            return 5;
        }
        return root.classicTargetIndexFromVars();
    }

    readonly property int lr2BeatorajaTargetIndex: {
        let vars = root.mainGeneralVars();
        if (!vars || vars.scoreTarget !== ScoreTarget.Fraction) {
            return root.noSelectedButtonFrame;
        }
        let fraction = vars.targetScoreFraction || 0;
        let bestIndex = 0;
        let bestDistance = Number.MAX_VALUE;
        for (let i = 0; i < root.lr2BeatorajaTargetFractions.length; ++i) {
            let distance = Math.abs(fraction - root.lr2BeatorajaTargetFractions[i]);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    function setBeatorajaTargetIndex(index: var) : var {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return;
        }
        let normalized = root.normalizeSupportedFrame(
            index,
            root.lr2BeatorajaTargetLabels.length,
            root.lr2BeatorajaTargetSupportedIndexes,
            root.lr2BeatorajaTargetLabels.length);
        vars.scoreTarget = ScoreTarget.Fraction;
        vars.targetScoreFraction = root.lr2BeatorajaTargetFractions[normalized];
    }

    function targetButtonUsesBeatorajaSource(sourceCount: var) : var {
        return root.usesBeatorajaOptionSemantics();
    }

    function targetButtonUsesClassicSource(sourceCount: var) : var {
        return !root.usesBeatorajaOptionSemantics();
    }

    function targetButtonSupportedIndexes(sourceCount: var) : var {
        if (root.targetButtonUsesBeatorajaSource(sourceCount)) {
            return root.lr2BeatorajaTargetSupportedIndexes;
        }
        if (root.targetButtonUsesClassicSource(sourceCount)) {
            return root.lr2ClassicTargetSupportedIndexes;
        }
        return root.lr2TargetSupportedIndexes;
    }

    function targetButtonDefaultCount(sourceCount: var) : var {
        if (root.targetButtonUsesBeatorajaSource(sourceCount)) {
            return root.lr2BeatorajaTargetLabels.length;
        }
        if (root.targetButtonUsesClassicSource(sourceCount)) {
            return root.lr2ClassicTargetLabels.length;
        }
        return root.lr2TargetLabels.length;
    }

    function lr2ScoreTargetFrame() : var {
        let vars = root.mainGeneralVars();
        return vars ? root.indexOfValueOrNone(root.lr2TargetValues, vars.scoreTarget) : 0;
    }

    function targetButtonCurrentFrame(sourceCount: var) : var {
        if (root.targetButtonUsesBeatorajaSource(sourceCount)) {
            return root.lr2BeatorajaTargetIndex;
        }
        if (root.targetButtonUsesClassicSource(sourceCount)) {
            return root.lr2ClassicTargetCurrentFrame();
        }
        return root.lr2ScoreTargetFrame();
    }

    function lr2TargetButtonFrame(sourceCount: var) : var {
        let count = root.targetButtonDefaultCount(sourceCount);
        return root.supportedFrameOrNone(
            root.targetButtonCurrentFrame(sourceCount),
            count,
            root.targetButtonSupportedIndexes(sourceCount),
            count);
    }

    function setTargetButtonIndex(index: var, sourceCount: var) : var {
        let count = root.targetButtonDefaultCount(sourceCount);
        if (root.targetButtonUsesBeatorajaSource(sourceCount)) {
            root.setBeatorajaTargetIndex(root.normalizeSupportedFrame(
                index,
                count,
                root.lr2BeatorajaTargetSupportedIndexes,
                root.lr2BeatorajaTargetLabels.length));
        } else if (root.targetButtonUsesClassicSource(sourceCount)) {
            root.setClassicTargetIndex(root.normalizeSupportedFrame(
                index,
                count,
                root.lr2ClassicTargetSupportedIndexes,
                root.lr2ClassicTargetLabels.length));
        } else {
            root.setScoreTargetIndex(root.normalizeSupportedFrame(
                index,
                count,
                root.lr2TargetSupportedIndexes,
                root.lr2TargetLabels.length));
        }
    }

    function adjustTargetButtonIndex(delta: var, sourceCount: var) : var {
        let count = root.targetButtonDefaultCount(sourceCount);
        let index = root.cycleSupportedFrame(
            root.targetButtonCurrentFrame(sourceCount),
            delta,
            count,
            root.targetButtonSupportedIndexes(sourceCount),
            count);
        root.setTargetButtonIndex(index, sourceCount);
    }

    function lr2ClassicTargetText(index: var) : var {
        if (index === 5) {
            return String(root.lr2TargetPercent) + "%";
        }
        return root.wrappedListValue(root.lr2ClassicTargetLabels, index);
    }

    function lr2TargetText(sourceCount: var) : var {
        let index = root.lr2TargetButtonFrame(sourceCount);
        if (index < 0) {
            return "";
        }
        if (root.targetButtonUsesBeatorajaSource(sourceCount)) {
            return root.wrappedListValue(root.lr2BeatorajaTargetLabels, index);
        }
        if (root.targetButtonUsesClassicSource(sourceCount)) {
            return root.lr2ClassicTargetText(index);
        }
        return root.wrappedListValue(root.lr2TargetLabels, index);
    }

    readonly property int lr2TargetPercent: {
        let vars = root.mainGeneralVars();
        return vars ? Math.floor(root.lr2DefaultTargetFraction() * 100) : 90;
    }

    function setTargetPercent(percent: var) : void {
        let vars = root.mainGeneralVars();
        if (vars) {
            let rounded = Math.floor(percent);
            if (!isFinite(rounded)) {
                rounded = 90;
            } else if (rounded > 100) {
                rounded = 50;
            } else if (rounded < 50) {
                rounded = 100;
            }
            let fraction = rounded / 100;
            vars.defaultTargetScoreFraction = fraction;
            if (root.lr2ClassicUserTargetActive && vars.scoreTarget === ScoreTarget.Fraction) {
                vars.targetScoreFraction = fraction;
            }
        }
    }

    function lr2DefaultTargetFraction() : var {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return 0.9;
        }
        let value = vars.defaultTargetScoreFraction;
        if (value === undefined || value === null) {
            value = vars.targetScoreFraction;
        }
        return Math.max(0, Math.min(1, value || 0));
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
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.offset = Math.max(-500, Math.min(500, (vars.offset || 0) + delta));
        }
    }

    function clearStatusOption() : var {
        let vars = root.mainGeneralVars();
        if (!vars || vars.gaugeMode === GaugeMode.Best) {
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

    function clearStatusIsBest() : var {
        let vars = root.mainGeneralVars();
        return !!vars && vars.gaugeMode === GaugeMode.Best;
    }

    function isLoggedIn() : var {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        return !!profile
            && profile.loginState === Profile.LoggedIn
            && !!profile.onlineUserData;
    }

}
