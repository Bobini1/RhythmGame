pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host

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

    function wrapValue(value, count) {
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
    readonly property var lr2BattleLabels: ["OFF", "BATTLE", "SP TO DP"]
    readonly property var lr2TargetLabels: ["GRADE", "BEST SCORE", "LAST SCORE"]
    readonly property var lr2TargetValues: [ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore]
    readonly property var lr2BeatorajaTargetLabels: [
        "PACEMAKER A-",
        "PACEMAKER A",
        "PACEMAKER A+",
        "PACEMAKER AA-",
        "PACEMAKER AA",
        "PACEMAKER AA+",
        "PACEMAKER AAA-",
        "PACEMAKER AAA",
        "PACEMAKER AAA+",
        "PACEMAKER MAX",
        "PACEMAKER NEXT"
    ]
    readonly property var lr2BeatorajaTargetFractions: [0.666, 0.703, 0.740, 0.777, 0.814, 0.851, 0.888, 0.925, 0.962, 1.0]
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

    function arrayContains(values, value) {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return true;
            }
        }
        return false;
    }

    function cycleSupportedIndex(current, delta, supportedIndexes, count) {
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

    function setDpOptionIndex(index) {
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

    readonly property int lr2BeatorajaBgaIndex: {
        let vars = root.mainGeneralVars();
        return vars && !vars.bgaOn ? 2 : 0;
    }

    function setBgaIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.bgaOn = root.wrapValue(index, 2) === 0;
        }
    }

    readonly property int lr2BgaSizeIndex: {
        let vars = root.mainGeneralVars();
        return vars ? Math.max(0, Math.min(1, vars.bgaSize || 0)) : 0;
    }

    function setBgaSizeIndex(index) {
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

    function setGaugeAutoShiftIndex(index) {
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

    readonly property int lr2ScoreGraphIndex: {
        let vars = root.mainGeneralVars();
        return vars && vars.scoreGraphEnabled === false ? 0 : 1;
    }

    function setScoreGraphIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.scoreGraphEnabled = root.wrapValue(index, 2) === 1;
        }
    }

    readonly property int lr2GhostIndex: {
        let vars = root.mainGeneralVars();
        return vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0;
    }

    function setGhostIndex(index) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.ghostPosition = root.wrapValue(index, root.lr2GhostLabels.length);
        }
    }

    function lr2BgaEnabled() {
        let vars = root.mainGeneralVars();
        return !vars || vars.bgaOn !== false;
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

    readonly property int lr2BeatorajaTargetIndex: {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return 6;
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

    function setBeatorajaTargetIndex(index) {
        let vars = root.mainGeneralVars();
        if (!vars) {
            return;
        }
        let normalized = root.wrapValue(index, root.lr2BeatorajaTargetFractions.length);
        vars.scoreTarget = ScoreTarget.Fraction;
        vars.targetScoreFraction = root.lr2BeatorajaTargetFractions[normalized];
    }

    readonly property int lr2TargetPercent: {
        let vars = root.mainGeneralVars();
        return vars ? Math.floor((vars.targetScoreFraction || 0) * 100) : 80;
    }

    function setTargetPercent(percent) {
        let vars = root.mainGeneralVars();
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

    function setHiSpeedNumber(side, value) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        let clamped = Math.max(1, Math.min(999, value));
        vars.noteScreenTimeMillis = 100000 / clamped;
    }

    function nextLr2HiSpeedNumber(current, steps) {
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

    function adjustHiSpeedNumber(side, steps) {
        let current = side === 2 ? root.lr2HiSpeedP2 : root.lr2HiSpeedP1;
        root.setHiSpeedNumber(side, root.nextLr2HiSpeedNumber(current, steps));
    }

    function adjustLaneCoverRatio(side, steps) {
        let vars = root.generalVarsForSide(side);
        if (!vars) {
            return;
        }
        vars.laneCoverRatio = Math.max(0, Math.min(1, (vars.laneCoverRatio || 0) + steps * 0.005));
    }

    function adjustOffset(delta) {
        let vars = root.mainGeneralVars();
        if (vars) {
            vars.offset = Math.max(-99, Math.min(99, (vars.offset || 0) + delta));
        }
    }

    function clearStatusOption() {
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

    function clearStatusIsBest() {
        let vars = root.mainGeneralVars();
        return !!vars && vars.gaugeMode === GaugeMode.Best;
    }

    function isLoggedIn() {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        return !!profile
            && profile.loginState === Profile.LoggedIn
            && !!profile.onlineUserData;
    }

}
