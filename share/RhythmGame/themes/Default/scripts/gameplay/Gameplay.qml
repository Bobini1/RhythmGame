import QtQuick
import QtQuick.Window
import QtQml
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls
import "../common/TaoQuickCustom"
import "../common/helpers.js" as Helpers
import "popups"

Rectangle {
    id: root

    property bool customizeMode: false
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property Profile mainProfile: Rg.profileList.mainProfile
    readonly property var mainProfileVars: mainProfile.vars.themeVars[screen][themeName]
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    readonly property ChartData chartData: root.gameplay.chartData
    readonly property string screen: {
        let keys = gameplay.keymode;
        let battle = root.player1 && root.player2;
        return "k" + keys + (battle ? "battle" : "");
    }
    property var popup: null
    readonly property bool isDp: screen === "k14" || screen === "k10"
    readonly property bool isBattle: screen === "k7battle" || screen === "k5battle"
    readonly property bool isCourse: root.gameplay.isCourse
    required property GameplayContext gameplay
    readonly property Player player1: root.gameplay.players[0]
    readonly property Player player2: root.gameplay.players[1] || null
    readonly property var arenaSession: Rg.arenaSession
    readonly property bool arenaGameplayOwned: root.gameplay.arenaActive
    readonly property bool arenaOpponentTargetAvailable: root.arenaGameplayOwned
        && root.arenaSession.opponentTarget !== undefined
        && root.arenaSession.opponentTarget !== null
        && root.arenaSession.opponentTarget.available === true
    readonly property bool arenaPointTargetAvailable: !root.arenaGameplayOwned
        || root.arenaOpponentTargetAvailable
    property var pendingScoreDbReply: null

    function trackScoreDbReply(reply: var) : var {
        if (!reply || reply.resultAvailable) {
            return reply;
        }
        pendingScoreDbReply = reply;
        let forget = function() {
            reply.finished.disconnect(forget);
            if (pendingScoreDbReply === reply) {
                pendingScoreDbReply = null;
            }
        };
        reply.finished.connect(forget);
        return reply;
    }

    function cancelScoreDbReply() {
        let reply = pendingScoreDbReply;
        pendingScoreDbReply = null;
        if (reply && !reply.resultAvailable) {
            reply.cancel();
        }
    }

    function closeActivePopup() {
        if (root.popup !== null) {
            root.popup.close();
            root.popup = null;
        }
    }

    function isPlayerScratchRightSide(player) {
        return player?.profile?.vars?.themeVars[root.screen][root.themeName]?.scratchOnRightSide;
    }
    property var inputMapping: {
        let left = [0, 1, 2, 3, 4, 5, 6, 7];
        let right = [8, 9, 10, 11, 12, 13, 14, 15];
        if (root.player1.score.keymode === 5 && isPlayerScratchRightSide(root.player1)) {
            left = [6, 5, 0, 1, 2, 3, 4, 7];
        }
        if ((root.player2 && root.player2.score.keymode === 5 && isPlayerScratchRightSide(root.player2)) || root.player1.score.keymode === 10) {
            right = [14, 13, 8, 9, 10, 11, 12, 15];
        }
        return left.concat(right);
    }
    onInputMappingChanged: {
        gameplay.inputMapping = inputMapping;
    }
    readonly property string themeName: QmlUtils.themeName
    property var scores1: []
    property var scoreWithBestPoints1: Helpers.getScoreWithBestPoints(scores1)
    property var lastScore1: scores1[0]
    readonly property real savedBestPoints1: scoreWithBestPoints1 ? scoreWithBestPoints1.result.points : 0
    readonly property real targetFraction1: {
        let vars = root.player1.profile.vars.generalVars;
        if (vars.scoreTarget === ScoreTarget.NextRank) {
            let maxPoints = root.player1.score.maxPoints || 0;
            return maxPoints > 0
                ? Helpers.getNextRankTargetPoints(savedBestPoints1, maxPoints) / maxPoints
                : 0;
        }
        return vars.targetScoreFraction;
    }
    property var targetScore1: {
        switch (root.player1.profile.vars.generalVars.scoreTarget) {
        case ScoreTarget.BestScore:
            return scoreWithBestPoints1;
        case ScoreTarget.LastScore:
            return lastScore1;
        default:
            return undefined;
        }
    }
    property real p1MaxPointsNow: root.player1.score.maxPointsNow
    property real targetPoints1: {
        if (root.arenaGameplayOwned) {
            return root.arenaOpponentTargetAvailable
                ? Number(root.arenaSession.opponentTarget.exScore || 0)
                : 0;
        }
        if (isBattle) {
            return root.player2.score.points;
        }
        if (targetScore1) {
            if (targetScore1.replayData !== null) {
                return scoreReplayer1.points;
            }
            return p1MaxPointsNow * targetScore1.result.points
                / Math.max(1, targetScore1.result.maxPoints);
        }
        return p1MaxPointsNow * targetFraction1;
    }
    property real targetPoints2: root.player1.score.points
    readonly property real targetFinalPoints1: {
        if (root.arenaGameplayOwned) {
            return root.arenaOpponentTargetAvailable
                ? Number(root.arenaSession.opponentTarget.exScore || 0)
                : 0;
        }
        if (isBattle) return 0;
        if (targetScore1) return targetScore1.result.points;
        if (root.player1.profile.vars.generalVars.scoreTarget === ScoreTarget.NextRank) {
            return Helpers.getNextRankTargetPoints(savedBestPoints1, root.player1.score.maxPoints || 0);
        }
        return root.player1.score.maxPoints * targetFraction1;
    }
    ScoreReplayer {
        id: scoreReplayer1
        hitEvents: targetScore1?.replayData?.hitEvents
    }
    ScoreReplayer {
        id: bestScoreReplayer1
        hitEvents: root.scoreWithBestPoints1?.replayData?.hitEvents || []
    }

    StackView.onDeactivating: {
        cancelScoreDbReply();
        closeActivePopup();
    }
    Component.onDestruction: {
        cancelScoreDbReply();
        closeActivePopup();
    }

    function getColumnSizes(vars) {
        let sizes = [];
        for (let i = 0; i < 16; i++) {
            if (i === 7 || i === 15) {
                sizes.push(vars.scratchWidth);
            } else if (i % 2 === 0)
                sizes.push(vars.whiteWidth);
            else {
                sizes.push(vars.blackWidth);
            }
        }
        return sizes;
    }

    color: "black"

    Timer {
        id: poorLayerTimer

        interval: 400

        onTriggered: {
            bga.poorVisible = false;
        }
    }
    Connections {
        function onStatusChanged() {
            if (root.gameplay.status === ChartRunner.Ready || root.gameplay.status === ChartRunner.Running) {
                bga.clearOutput();
                gameplay.bga.layers[0].videoSink = bga.baseSink;
                gameplay.bga.layers[1].videoSink = bga.layerSink;
                gameplay.bga.layers[2].videoSink = bga.layer2Sink;
                gameplay.bga.layers[3].videoSink = bga.poorSink;
            } else if (root.gameplay.status === ChartRunner.Finished) {
                bga.clearOutput();
                root.closeActivePopup();
                gameplay.bga.layers[0].videoSink = bga.baseSink;
                gameplay.bga.layers[1].videoSink = bga.layerSink;
                gameplay.bga.layers[2].videoSink = bga.layer2Sink;
                gameplay.bga.layers[3].videoSink = bga.poorSink;
            }
        }

        target: root.gameplay
    }
    PlayAreaPopup {
        id: playAreaPopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp
        fiveKeys: root.player1.score.keymode === 5 || root.player1.score.keymode === 10

        onClosed: {
            root.popup = null;
        }
    }
    PlayAreaPopup {
        id: playAreaPopupP2

        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars
        dp: root.isDp
        fiveKeys: (root.player2 || root.player1).score.keymode === 5 || (root.player2 || root.player1).score.keymode === 10

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    GaugePopup {
        id: gaugePopupP2

        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementCountsPopup {
        id: judgementCountsPopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementCountsPopup {
        id: judgementCountsPopupP2

        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementsPopup {
        id: judgementsPopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    JudgementsPopup {
        id: judgementsPopupP2

        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    ScoreGraphPopup {
        id: scoreGraphPopup
        readonly property Profile profile: root.player1.profile
        generalVars: profile.vars.generalVars
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        onClosed: {
            root.popup = null;
        }
    }
    ScoreGraphPopup {
        id: scoreGraphPopupP2
        readonly property Profile profile: (root.player2 || root.player1).profile
        generalVars: profile.vars.generalVars
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        onClosed: {
            root.popup = null;
        }
    }
    BpmDisplayPopup {
        id: bpmDisplayPopup
        themeVars: root.mainProfileVars
        onClosed: {
            root.popup = null;
        }
    }
    TitleDisplayPopup {
        id: titleDisplayPopup
        themeVars: root.mainProfileVars
        onClosed: {
            root.popup = null;
        }
    }
    DifficultyDisplayPopup {
        id: difficultyDisplayPopup
        themeVars: root.mainProfileVars
        onClosed: {
            root.popup = null;
        }
    }
    GhostScorePopup {
        id: ghostScorePopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    GhostScorePopup {
        id: ghostScorePopupP2
        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    FastslowPopup {
        id: fastslowPopup

        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    FastslowPopup {
        id: fastslowPopupP2

        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]

        onClosed: {
            root.popup = null;
        }
    }
    BgaPopup {
        id: bgaPopup

        readonly property Profile profile: Rg.profileList.mainProfile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        generalVars: profile.vars.generalVars

        onClosed: {
            root.popup = null;
        }
    }
    DensityGraphPopup {
        id: densityGraphPopup
        themeVars: root.mainProfileVars

        onClosed: {
            root.popup = null;
        }
    }

    HitDistributionPopup {
        id: hitDistributionPopup
        readonly property Profile profile: root.player1.profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        onClosed: { root.popup = null; }
    }
    HitDistributionPopup {
        id: hitDistributionPopupP2
        readonly property Profile profile: (root.player2 || root.player1).profile
        themeVars: profile.vars.themeVars[root.screen][root.themeName]
        onClosed: { root.popup = null; }
    }
    // Used for the single centred hit distribution in DP (k14) mode.
    HitDistributionPopup {
        id: hitDistributionPopupDp
        themeVars: root.mainProfileVars
        onClosed: { root.popup = null; }
    }

    Item {
        id: scaledRoot
        enabled: !gameplayInput.retryChoosing

        anchors.centerIn: parent
        width: 1920
        height: 1080
        scale: Math.min(globalRoot.width / 1920, globalRoot.height / 1080)
        transformOrigin: Item.Center

        Text {
            color: "white"
            text: "fps: " + Rg.programSettings.presentationFps
            anchors.top: parent.top
            anchors.right: parent.right
            textFormat: Text.PlainText
        }

        BgaRenderer {
            id: bga

            readonly property Profile profile: root.player2 ? Rg.profileList.mainProfile : root.player1.profile
            readonly property var profileVars: profile.vars.themeVars[root.screen][root.themeName]

            height: profileVars.bgaSize
            bgaVisible: profile.vars.generalVars.bgaOn
            width: profileVars.bgaSize
            x: profileVars.bgaX
            y: profileVars.bgaY
            z: profileVars.bgaZ

            onHeightChanged: {
                profileVars.bgaSize = height;
            }
            onXChanged: {
                profileVars.bgaX = x;
            }
            onYChanged: {
                profileVars.bgaY = y;
            }

            TemplateDragBorder {
                id: bgaTemplate

                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                keepAspectRatio: true
                visible: root.customizeMode
            }

            MouseArea {
                id: bgaMouseArea

                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1
                enabled: root.customizeMode

                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    bgaPopup.setPosition(point);
                    bgaPopup.open();
                    root.popup = bgaPopup;
                }
            }
        }

        TitleDisplay {
            id: titleDisplayItem
            x: root.mainProfileVars.titleDisplayX
            y: root.mainProfileVars.titleDisplayY
            width: root.mainProfileVars.titleDisplayWidth
            height: root.mainProfileVars.titleDisplayHeight
            z: root.mainProfileVars.titleDisplayZ
            contentVisible: root.mainProfileVars.titleDisplayEnabled
            title: root.chartData.title
            subtitle: root.chartData.subtitle
            fontFile: root.mainProfileVars.titleDisplayFont
            onXChanged: root.mainProfileVars.titleDisplayX = x
            onYChanged: root.mainProfileVars.titleDisplayY = y
            onWidthChanged: root.mainProfileVars.titleDisplayWidth = width
            onHeightChanged: root.mainProfileVars.titleDisplayHeight = height

            TemplateDragBorder {
                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                visible: root.customizeMode
            }
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1
                enabled: root.customizeMode
                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    titleDisplayPopup.setPosition(point);
                    titleDisplayPopup.open();
                    root.popup = titleDisplayPopup;
                }
            }
        }

        DifficultyDisplay {
            id: difficultyDisplayItem
            x: root.mainProfileVars.difficultyDisplayX
            y: root.mainProfileVars.difficultyDisplayY
            width: root.mainProfileVars.difficultyDisplayWidth
            height: root.mainProfileVars.difficultyDisplayHeight
            z: root.mainProfileVars.difficultyDisplayZ
            contentVisible: root.mainProfileVars.difficultyDisplayEnabled
            difficulty: root.chartData.difficulty
            playLevel: root.chartData.playLevel
            fontFile: root.mainProfileVars.difficultyDisplayFont
            onXChanged: root.mainProfileVars.difficultyDisplayX = x
            onYChanged: root.mainProfileVars.difficultyDisplayY = y
            onWidthChanged: root.mainProfileVars.difficultyDisplayWidth = width
            onHeightChanged: root.mainProfileVars.difficultyDisplayHeight = height

            TemplateDragBorder {
                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                visible: root.customizeMode
            }
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1
                enabled: root.customizeMode
                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    difficultyDisplayPopup.setPosition(point);
                    difficultyDisplayPopup.open();
                    root.popup = difficultyDisplayPopup;
                }
            }
        }

        BpmDisplay {
            id: bpmDisplayItem
            x: root.mainProfileVars.bpmDisplayX
            y: root.mainProfileVars.bpmDisplayY
            width: root.mainProfileVars.bpmDisplayWidth
            height: root.mainProfileVars.bpmDisplayHeight
            z: root.mainProfileVars.bpmDisplayZ
            contentVisible: root.mainProfileVars.bpmDisplayEnabled
            currentBpm: root.player1.bpm
            minBpm: root.chartData.minBpm
            maxBpm: root.chartData.maxBpm
            fontFile: root.mainProfileVars.bpmDisplayFont
            onXChanged: root.mainProfileVars.bpmDisplayX = x
            onYChanged: root.mainProfileVars.bpmDisplayY = y
            onWidthChanged: root.mainProfileVars.bpmDisplayWidth = width
            onHeightChanged: root.mainProfileVars.bpmDisplayHeight = height

            TemplateDragBorder {
                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                visible: root.customizeMode
            }
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1
                enabled: root.customizeMode
                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    bpmDisplayPopup.setPosition(point);
                    bpmDisplayPopup.open();
                    root.popup = bpmDisplayPopup;
                }
            }
        }

        DensityGraph {
            id: densityGraphItem
            x:       root.mainProfileVars.densityGraphX
            y:       root.mainProfileVars.densityGraphY
            width:   root.mainProfileVars.densityGraphWidth
            height:  root.mainProfileVars.densityGraphHeight
            z:       root.mainProfileVars.densityGraphZ
            contentVisible:         root.mainProfileVars.densityGraphEnabled
            gapsEnabled:            root.mainProfileVars.densityGraphGapsEnabled
            notesOpacity:           root.mainProfileVars.densityGraphNotesOpacity
            bpmOpacity:             root.mainProfileVars.densityGraphBpmOpacity
            frameOpacity:           root.mainProfileVars.densityGraphFrameOpacity
            backgroundOpacity:      root.mainProfileVars.densityGraphBackgroundOpacity
            bpmConnectorOpacity:    root.mainProfileVars.densityGraphBpmConnectorOpacity
            vertical:               root.mainProfileVars.densityGraphVertical

            histogramData: root.chartData.histogramData
            bpms:          root.chartData.bpmChanges
            mainBpm:       root.chartData.mainBpm
            maxBpm:        root.chartData.maxBpm
            minBpm:        root.chartData.minBpm
            length:        root.chartData.length
            elapsed:       root.player1.elapsed
            positionLineOpacity: root.mainProfileVars.densityGraphPositionLineOpacity

            onXChanged:      root.mainProfileVars.densityGraphX = x
            onYChanged:      root.mainProfileVars.densityGraphY = y
            onWidthChanged:  root.mainProfileVars.densityGraphWidth = width
            onHeightChanged: root.mainProfileVars.densityGraphHeight = height

            TemplateDragBorder {
                anchors.fill: parent
                anchors.margins: -borderMargin
                color: "transparent"
                visible: root.customizeMode
            }
            MouseArea {
                acceptedButtons: Qt.RightButton
                anchors.fill: parent
                z: -1
                enabled: root.customizeMode
                onClicked: mouse => {
                    let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                    densityGraphPopup.setPosition(point);
                    densityGraphPopup.open();
                    root.popup = densityGraphPopup;
                }
            }
        }

        Side {
            anchors.fill: parent
            player: root.player1
            dpSuffix: root.isDp ? "1" : ""
            index: 0
            pointTarget: root.targetPoints1
            pointTargetAvailable: root.arenaPointTargetAvailable
            bestFinalPoints: root.scoreWithBestPoints1 ? root.scoreWithBestPoints1.result.points : 0
            bestMaxPoints: root.scoreWithBestPoints1 ? root.scoreWithBestPoints1.result.maxPoints : 0
            bestPoints: bestScoreReplayer1.points
            targetFinalPoints: root.targetFinalPoints1
            columns: {
                if (root.isDp) {
                    return [7, 0, 1, 2, 3, 4, 5, 6];
                } else {
                    if (root.isPlayerScratchRightSide(root.player1)) {
                        return root.player1.score.keymode === 7 ? [0, 1, 2, 3, 4, 5, 6, 7] : [6, 5, 0, 1, 2, 3, 4, 7];
                    } else {
                        return [7, 0, 1, 2, 3, 4, 5, 6];
                    }
                }
            }
        }
        Loader {
            id: p2SideLoader
            active: root.player2 !== null || root.isDp
            anchors.fill: parent
            sourceComponent: Side {
                id: side2
                player: root.isDp ? root.player1 : root.player2
                dpSuffix: root.isDp ? "2" : ""
                mirrored: !root.isDp
                index: 1
                pointTarget: root.isDp ? root.targetPoints1 : root.targetPoints2
                pointTargetAvailable: root.arenaPointTargetAvailable
                bestFinalPoints: root.isDp ? (root.scoreWithBestPoints1 ? root.scoreWithBestPoints1.result.points : 0) : 0
                bestMaxPoints: root.isDp ? (root.scoreWithBestPoints1 ? root.scoreWithBestPoints1.result.maxPoints : 0) : 0
                bestPoints: root.isDp ? bestScoreReplayer1.points : 0
                targetFinalPoints: root.isDp ? root.targetFinalPoints1 : 0
                columns: {
                    if (root.isDp) {
                        return root.player1.score.keymode === 14 ? [8, 9, 10, 11, 12, 13, 14, 15] : [14, 13, 8, 9, 10, 11, 12, 15];
                    } else {
                        if (root.isPlayerScratchRightSide(root.player2)) {
                            return root.player2.score.keymode === 7 ? [0, 1, 2, 3, 4, 5, 6, 7] : [6, 5, 0, 1, 2, 3, 4, 7];
                        } else {
                            return [7, 0, 1, 2, 3, 4, 5, 6];
                        }
                    }
                }
            }
        }
    }
    Connections {
        target: root.player1.score
        function onHit(tap) {
            if (targetScore1) {
                scoreReplayer1.notifyHit(tap);
            }
            bestScoreReplayer1.notifyHit(tap);
        }
    }


    HitDistribution {
        id: dpHitDistribution

        visible: root.isDp
        contentVisible: root.isDp && root.mainProfileVars.hitDistributionEnabled

        x: root.mainProfileVars.hitDistributionX
        y: root.mainProfileVars.hitDistributionY
        width: root.mainProfileVars.hitDistributionWidth
        height: root.mainProfileVars.hitDistributionHeight
        z: root.mainProfileVars.hitDistributionZ

        ewmaMode: root.mainProfileVars.hitDistributionEwmaMode
        ewmaAlpha: root.mainProfileVars.hitDistributionEwmaAlpha
        maxTrail: root.mainProfileVars.hitDistributionMaxTrail
        vertical: root.mainProfileVars.hitDistributionVertical
        lineColor: root.mainProfileVars.hitDistributionLineColor
        centerLineColor: root.mainProfileVars.hitDistributionCenterLineColor
        lineWidth: root.mainProfileVars.hitDistributionLineWidth
        centerLineWidth: root.mainProfileVars.hitDistributionCenterLineWidth
        backgroundOpacity: root.mainProfileVars.hitDistributionBackgroundOpacity

        timingWindows: root.chartData.timingWindows
        score: root.player1.score

        onXChanged: root.mainProfileVars.hitDistributionX = x
        onYChanged: root.mainProfileVars.hitDistributionY = y
        onWidthChanged: root.mainProfileVars.hitDistributionWidth = width
        onHeightChanged: root.mainProfileVars.hitDistributionHeight = height

        TemplateDragBorder {
            anchors.fill: parent
            anchors.margins: -borderMargin
            color: "transparent"
            visible: root.customizeMode
        }

        MouseArea {
            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            z: -1
            enabled: root.customizeMode
            onClicked: mouse => {
                let point = mapToItem(Overlay.overlay, mouse.x, mouse.y);
                hitDistributionPopupDp.setPosition(point);
                hitDistributionPopupDp.open();
                root.popup = hitDistributionPopupDp;
            }
        }
    }
    StandardArenaGameplayOverlay {
        id: arenaOverlay

        gameplay: root.gameplay
        customizeMode: root.customizeMode
        themeVars: root.mainProfileVars
        viewport: root
        z: 2000000
    }
    StandardGameplayFlow {
        id: gameplayInput

        gameplay: root.gameplay
        onStageActivated: {
            root.cancelScoreDbReply();
            scoreReplayer1.resetPoints();
            bestScoreReplayer1.resetPoints();
            root.trackScoreDbReply(root.player1.profile.scoreDb.getScoresForMd5(root.chartData.md5)).then(scores => {
                root.scores1 = scores.scores[root.chartData.md5] || [];
            });
        }
        onClosing: root.closeActivePopup()
        onRetryChoosingChanged: if (retryChoosing) root.closeActivePopup()
    }
    Shortcut {
        sequence: "F2"
        enabled: root.enabled && !gameplayInput.retryChoosing

        onActivated: {
            root.customizeMode = !root.customizeMode;
            root.closeActivePopup();
        }
    }

    TransientInputFocusDismissLayer {}
}
