pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: root

    required property var host
    required property var selectContext
    required property var optionOpenSound
    required property var optionCloseSound

    readonly property var selectChart: {
        if (root.host.effectiveScreenKey !== "select") {
            return null;
        }

        let selectedChart = root.selectContext.selectedStateChartData;
        if (selectedChart) {
            return selectedChart;
        }
        if (root.selectContext.rankingMode && root.selectContext.rankingBaseItem) {
            return root.selectContext.rankingBaseItem;
        }

        let item = root.selectContext.focusedItem;
        return item instanceof ChartData || item instanceof entry ? item : null;
    }
    readonly property var chart: root.host.resultScreenActive
        ? root.host.resultChartData()
        : root.selectChart
    readonly property string md5: root.chart && root.chart.md5 ? String(root.chart.md5) : ""
    readonly property string loadedMd5: root.rankingModel.md5 ? String(root.rankingModel.md5) : ""
    readonly property bool requestMatchesCurrentChart: {
        let targetMd5 = root.md5.length > 0 ? root.md5.toLowerCase() : "";
        let requestMd5 = root.requestMd5.length > 0 ? root.requestMd5.toLowerCase() : "";
        return targetMd5.length > 0 && targetMd5 === requestMd5;
    }
    readonly property bool modelMatchesCurrentChart: {
        let targetMd5 = root.md5.length > 0 ? root.md5.toLowerCase() : "";
        let modelMd5 = root.loadedMd5.length > 0 ? root.loadedMd5.toLowerCase() : "";
        return targetMd5.length > 0 && targetMd5 === modelMd5;
    }
    readonly property bool currentRequestCompleted: {
        let targetMd5 = root.md5.length > 0 ? root.md5.toLowerCase() : "";
        let completedMd5 = root.completedMd5.length > 0 ? root.completedMd5.toLowerCase() : "";
        return targetMd5.length > 0
            && targetMd5 === completedMd5
            && root.modelMatchesCurrentChart;
    }
    readonly property int currentPlayerCount: root.computedPlayerCount()
    readonly property int currentStatusOption: {
        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return 600;
        }
        if (!root.requestMatchesCurrentChart) {
            return 600;
        }
        if (root.rankingModel.loading || !root.currentRequestCompleted) {
            return 601;
        }
        return 602;
    }
    property string requestMd5: ""
    property string completedMd5: ""
    property bool openWhenReady: false
    property bool internetOpenWhenReady: false
    property bool closeWhenReady: false
    property int transitionPhase: 0 // 175 before list swap, 176 after list swap.
    property string transitionAction: ""
    property int transitionStartSkinTime: 0
    property var cachedSnapshot: null
    property var cachedReplayScoresByGuid: ({})
    property string cachedSnapshotTargetMd5: ""
    property string cachedSnapshotLoadedMd5: ""
    property var cachedSnapshotEntrySource: null
    property var emptyRankingEntries: []
    property int cachedSnapshotPlayerCount: -1
    property int cachedSnapshotScoreCount: -1
    property int cachedSnapshotScoreGeneration: -1
    property int cachedSnapshotClearCountsGeneration: -1
    property int cachedSnapshotProvider: -1
    property int cachedSnapshotProfileUserId: -1
    property int clearCountsGeneration: 0
    readonly property int transitionDuration: 120
    readonly property int transitionElapsed: root.transitionPhase !== 0
        ? Math.max(0, Math.min(root.transitionDuration,
            root.host.selectLiveSkinTime - root.transitionStartSkinTime))
        : 0

    property OnlineRankingModel rankingModel: OnlineRankingModel {
        md5: root.requestMd5
        limit: 999
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
        provider: root.providerEnum()
        webApiUrl: root.host.mainGeneralVarsRef ? root.host.mainGeneralVarsRef.webApiUrl : ""

        onMd5Changed: {
            root.completedMd5 = "";
            root.handleModelChanged(false, false);
        }
        onWebApiUrlChanged: {
            root.cachedReplayScoresByGuid = {};
        }
        onLoadingChanged: {
            if (loading) {
                root.completedMd5 = "";
            } else if (root.requestMatchesCurrentChart && root.modelMatchesCurrentChart) {
                root.completedMd5 = root.loadedMd5;
            }
            root.handleModelChanged(true, true);
        }
        onProviderChanged: {
            root.completedMd5 = "";
            root.cachedReplayScoresByGuid = {};
            root.handleModelChanged(false, false);
        }
        onRankingEntriesChanged: root.handleModelChanged(true, false)
        onPlayerCountChanged: root.handleModelChanged(false, false)
        onScoreCountChanged: root.handleModelChanged(false, false)
        onClearCountsChanged: {
            root.clearCountsGeneration += 1;
            root.handleModelChanged(true, false);
        }
        onChartIdChanged: {
            if (root.internetOpenWhenReady && !loading) {
                Qt.callLater(root.finishOpenInternetRanking);
            }
        }
    }

    onCurrentStatusOptionChanged: {
        root.refreshHostRankingStatusOptions();
    }
    onCurrentPlayerCountChanged: {
        root.refreshHostRankingStatusOptions();
    }

    property Timer transitionTimer: Timer {
        interval: root.transitionDuration
        repeat: false
        onTriggered: root.advanceTransition()
    }

    property Timer autoStatsFetchTimer: Timer {
        interval: 600
        repeat: false
        onTriggered: root.refreshCurrentChartStats()
    }

    onMd5Changed: {
        if (root.host.effectiveScreenKey !== "select") {
            root.autoStatsFetchTimer.stop();
            return;
        }
        if (root.md5.length <= 0) {
            root.autoStatsFetchTimer.stop();
            return;
        }
        root.autoStatsFetchTimer.restart();
    }

    Component.onCompleted: {
        if (root.host.effectiveScreenKey === "select" && root.md5.length > 0) {
            root.autoStatsFetchTimer.restart();
        }
    }

    function refreshHostRankingStatusOptions() : void {
        if (root.host
                && root.host.effectiveScreenKey === "select"
                && root.host.queueSelectRuntimeActiveOptionsRefresh) {
            root.host.queueSelectRuntimeActiveOptionsRefresh(
                true,
                false,
                root.host.selectRankingResolvedTextIds);
        } else if (root.host
                && root.host.effectiveScreenKey === "select"
                && root.host.refreshSelectRuntimeActiveOptions) {
            root.host.refreshSelectRuntimeActiveOptions();
        }
    }

    function md5ForChart(chart: var) : var {
        return chart && chart.md5 ? String(chart.md5) : "";
    }

    function commitRequest(chart: var) : var {
        let nextMd5 = chart === undefined ? root.md5 : root.md5ForChart(chart);
        if (root.requestMd5 !== nextMd5) {
            root.requestMd5 = nextMd5;
            return true;
        }
        return false;
    }

    function providerEnum() : var {
        let vars = root.host.mainGeneralVarsRef;
        return vars ? vars.rankingProvider : OnlineRankingModel.RhythmGame;
    }

    function isRhythmGameRankingEntry(item: var) : var {
        return root.selectContext.isRankingEntry(item)
            && Number(item.rankingProvider || 0) === OnlineRankingModel.RhythmGame;
    }

    function firstReplayGuid(candidates: var) : var {
        for (let value of candidates || []) {
            let guid = value ? String(value).trim() : "";
            if (guid.length > 0) {
                return guid;
            }
        }
        return "";
    }

    function replayGuidForType(entry: var, replayType: var) : var {
        if (!root.isRhythmGameRankingEntry(entry)) {
            return "";
        }
        switch (Math.floor(Number(replayType || 0))) {
        case 0:
            return root.firstReplayGuid([
                entry.latestDateGuid,
                entry.bestPointsGuid,
                entry.bestClearTypeGuid,
                entry.bestComboGuid,
                entry.bestComboBreaksGuid
            ]);
        case 1:
            return root.firstReplayGuid([
                entry.bestPointsGuid,
                entry.latestDateGuid,
                entry.bestClearTypeGuid,
                entry.bestComboGuid,
                entry.bestComboBreaksGuid
            ]);
        case 2:
            return root.firstReplayGuid([
                entry.bestClearTypeGuid,
                entry.bestPointsGuid,
                entry.latestDateGuid,
                entry.bestComboGuid,
                entry.bestComboBreaksGuid
            ]);
        case 3:
            return root.firstReplayGuid([
                entry.bestComboGuid,
                entry.bestPointsGuid,
                entry.latestDateGuid,
                entry.bestClearTypeGuid,
                entry.bestComboBreaksGuid
            ]);
        default:
            return "";
        }
    }

    function launchReplayScore(entry: var, score: var, mouseButton: var) : void {
        let button = mouseButton === undefined ? Qt.LeftButton : mouseButton;
        if (button === Qt.RightButton) {
            root.selectContext.openReplayResult(entry, score);
            return;
        }
        root.host.selectGoForward(entry, false, button !== Qt.MiddleButton, score);
    }

    function loadReplayScore(guid: var, onReady: var) : var {
        let key = guid ? String(guid).trim() : "";
        if (key.length <= 0) {
            return false;
        }
        let webApiUrl = root.rankingModel.webApiUrl
            ? String(root.rankingModel.webApiUrl).trim()
            : "";
        if (webApiUrl.length <= 0) {
            return false;
        }
        if (root.cachedReplayScoresByGuid[key]) {
            onReady(root.cachedReplayScoresByGuid[key]);
            return true;
        }

        if (!Rg.onlineScores) {
            return false;
        }
        let pending = Rg.onlineScores.getScoreByGuid(webApiUrl, key);
        if (!pending || !pending.valid) {
            return false;
        }
        if (pending.resultAvailable && !pending.success) {
            return false;
        }
        pending.then(function(score) {
            if (!score) {
                return;
            }
            let nextCache = Object.assign({}, root.cachedReplayScoresByGuid);
            nextCache[key] = score;
            root.cachedReplayScoresByGuid = nextCache;
            onReady(score);
        });
        return true;
    }

    function launchEntryReplay(entry: var, replayType: var, mouseButton: var) : var {
        let guid = root.replayGuidForType(entry, replayType);
        if (guid.length <= 0) {
            return false;
        }
        let button = mouseButton === undefined ? Qt.LeftButton : mouseButton;
        return root.loadReplayScore(guid, function(score) {
            root.launchReplayScore(entry, score, button);
        });
    }

    function launchReplayType(replayType: var, mouseButton: var) : var {
        return root.launchEntryReplay(
            root.selectContext.activationItem(),
            replayType,
            mouseButton);
    }

    function launchSelectedScoreAction(mouseButton: var) : var {
        return root.launchEntryReplay(
            root.selectContext.activationItem(),
            1,
            mouseButton);
    }

    function refreshCurrentChartStats() : void {
        if (root.host.effectiveScreenKey !== "select") {
            return;
        }
        let targetChart = root.chart;
        let targetMd5 = root.md5ForChart(targetChart);
        if (targetMd5.length <= 0) {
            return;
        }
        if (root.requestMd5 === targetMd5 && root.modelMatchesCurrentChart) {
            let entries = root.rankingModel.rankingEntries || [];
            if (root.currentRequestCompleted
                    || Number(root.rankingModel.playerCount || 0) > 0
                    || entries.length > 0) {
                root.applyStatsToSelectContext();
            } else if (!root.rankingModel.loading) {
                root.rankingModel.refresh();
            }
            return;
        }

        root.commitRequest(targetChart);
    }

    function localEntry() : var {
        root.selectContext.scoreGeneration;
        let targetChart = root.chart;
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        if (!targetChart || !profile) {
            return null;
        }

        let scoreList = root.selectContext.entryScores(targetChart);
        let bestPointsScore = root.selectContext.bestScoreByPoints(scoreList);
        let bestClearScore = root.selectContext.scoreWithBestClear(scoreList);
        if (!bestPointsScore || !bestPointsScore.result || !bestClearScore || !bestClearScore.result) {
            return null;
        }

        let stats = root.selectContext.statsForScore(bestPointsScore);
        return {
            __lr2Local: true,
            userId: profile.onlineUserData ? profile.onlineUserData.userId : 0,
            userName: profile.vars.generalVars.name,
            bestClearType: bestClearScore.result.clearType || "NOPLAY",
            bestPoints: bestPointsScore.result.points || 0,
            maxPoints: bestPointsScore.result.maxPoints || 0,
            bestCombo: bestPointsScore.result.maxCombo || 0,
            bestComboBreaks: stats ? stats.badPoor : 0,
            bestPerfect: stats ? stats.pg : 0,
            bestGreat: stats ? stats.gr : 0,
            bestGood: stats ? stats.gd : 0,
            bestBad: stats ? stats.bd : 0,
            bestPoor: stats ? stats.poor : 0,
            bestEmptyPoor: stats ? stats.miss : 0,
            scoreCount: scoreList.length
        };
    }

    function buildEntries(source: var, provider: var) : var {
        if (!root.modelMatchesCurrentChart) {
            return [];
        }

        let result = [];
        for (let i = 0; i < (source || []).length; ++i) {
            result.push(source[i]);
        }

        if (provider !== OnlineRankingModel.LR2IR) {
            return result;
        }

        let local = root.localEntry();
        if (!local) {
            return result;
        }

        let withLocal = [];
        let inserted = false;
        for (let entry of result) {
            if (!inserted && local.bestPoints > (entry.bestPoints || 0)) {
                withLocal.push(local);
                inserted = true;
            }
            withLocal.push(entry);
        }
        if (!inserted) {
            withLocal.push(local);
        }
        return withLocal;
    }

    function entries() : var {
        return root.snapshot().entries;
    }

    function staleLocalEntryAt(index: var) : var {
        if (root.modelMatchesCurrentChart
                || root.providerEnum() !== OnlineRankingModel.LR2IR
                || Number(index) !== 0) {
            return null;
        }
        return root.localEntry();
    }

    function clearCounts(entries: var) : var {
        let counts = {};
        for (let entry of entries || []) {
            let clearType = root.selectContext.skinCompatibleClearType(entry.bestClearType || "NOPLAY");
            counts[clearType] = (counts[clearType] || 0) + 1;
        }
        return counts;
    }

    function modelClearCounts() : var {
        let source = root.rankingModel.clearCounts || {};
        let counts = {};
        let hasCounts = false;
        for (let key in source) {
            let count = Number(source[key] || 0);
            if (count <= 0) {
                continue;
            }
            let clearType = root.selectContext.skinCompatibleClearType(key);
            counts[clearType] = (counts[clearType] || 0) + count;
            hasCounts = true;
        }
        return hasCounts ? counts : null;
    }

    function computedPlayerCount(entries: var) : var {
        if (!root.modelMatchesCurrentChart) {
            return 0;
        }
        let count = Number(root.rankingModel.playerCount || 0);
        if (entries && entries.length > 0) {
            return Math.max(count, entries.length);
        }
        if (count <= 0) {
            let source = root.rankingModel.rankingEntries || [];
            count = source.length || 0;
        }
        if (root.providerEnum() === OnlineRankingModel.LR2IR && root.localEntry()) {
            count += 1;
        }
        return count;
    }

    function playerCount(entries: var) : var {
        return entries === undefined
            ? root.currentPlayerCount
            : root.computedPlayerCount(entries);
    }

    function totalPlayCount(entries: var) : var {
        if (entries === undefined) {
            return root.snapshot().totalPlayCount;
        }
        let modelCount = root.rankingModel.scoreCount || 0;
        let total = 0;
        for (let entry of entries || []) {
            total += entry.scoreCount || 0;
        }
        return Math.max(modelCount, total);
    }

    function profileUserId() : var {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        if (!profile) {
            return 0;
        }
        if (root.providerEnum() === OnlineRankingModel.Tachi) {
            return profile.tachiData ? Number(profile.tachiData.userId || 0) : 0;
        }
        return profile.onlineUserData ? Number(profile.onlineUserData.userId || 0) : 0;
    }

    function playerRank(entries: var) : var {
        if (entries === undefined) {
            return root.snapshot().playerRank;
        }
        let userId = root.profileUserId();
        for (let i = 0; i < (entries || []).length; ++i) {
            let entry = entries[i];
            if (entry.__lr2Local || (userId > 0 && Number(entry.userId || 0) === userId)) {
                return i + 1;
            }
        }
        return 0;
    }

    function snapshot() : var {
        let targetMd5 = root.md5;
        let loadedMd5 = root.rankingModel.md5 ? String(root.rankingModel.md5) : "";
        let source = root.rankingModel.rankingEntries || root.emptyRankingEntries;
        let playerCountValue = Number(root.rankingModel.playerCount || 0);
        let scoreCountValue = Number(root.rankingModel.scoreCount || 0);
        let scoreGeneration = root.selectContext.scoreGeneration;
        let provider = root.providerEnum();
        let userId = root.profileUserId();

        if (root.cachedSnapshot
                && root.cachedSnapshotTargetMd5 === targetMd5
                && root.cachedSnapshotLoadedMd5 === loadedMd5
                && root.cachedSnapshotEntrySource === source
                && root.cachedSnapshotPlayerCount === playerCountValue
                && root.cachedSnapshotScoreCount === scoreCountValue
                && root.cachedSnapshotScoreGeneration === scoreGeneration
                && root.cachedSnapshotClearCountsGeneration === root.clearCountsGeneration
                && root.cachedSnapshotProvider === provider
                && root.cachedSnapshotProfileUserId === userId) {
            return root.cachedSnapshot;
        }

        let currentEntries = root.buildEntries(source, provider);
        let currentClearCounts = root.modelClearCounts() || root.clearCounts(currentEntries);
        root.cachedSnapshot = {
            entries: currentEntries,
            clearCounts: currentClearCounts,
            playerCount: root.playerCount(currentEntries),
            totalPlayCount: root.totalPlayCount(currentEntries),
            playerRank: root.playerRank(currentEntries)
        };
        root.cachedSnapshotTargetMd5 = targetMd5;
        root.cachedSnapshotLoadedMd5 = loadedMd5;
        root.cachedSnapshotEntrySource = source;
        root.cachedSnapshotPlayerCount = playerCountValue;
        root.cachedSnapshotScoreCount = scoreCountValue;
        root.cachedSnapshotScoreGeneration = scoreGeneration;
        root.cachedSnapshotClearCountsGeneration = root.clearCountsGeneration;
        root.cachedSnapshotProvider = provider;
        root.cachedSnapshotProfileUserId = userId;
        return root.cachedSnapshot;
    }

    function entryAt(index: var) : var {
        let local = root.staleLocalEntryAt(index);
        if (local) {
            return local;
        }
        let currentEntries = root.entries();
        return index >= 0 && index < currentEntries.length ? currentEntries[index] : null;
    }

    function entryName(index: var) : var {
        let entry = root.entryAt(index);
        if (!entry) {
            return "";
        }
        let name = entry.userName || entry.owner || entry.player || "";
        return name.length > 0 ? name : "YOU";
    }

    function entryClearValue(index: var) : var {
        let entry = root.entryAt(index);
        return entry ? root.host.clearTypeValue(entry.bestClearType || "NOPLAY") : 0;
    }

    function entryExScore(index: var) : var {
        let entry = root.entryAt(index);
        return entry ? Math.floor(entry.bestPoints || 0) : 0;
    }

    function clearCount() : var {
        let counts = root.snapshot().clearCounts;
        let total = 0;
        for (let i = 0; i < arguments.length; ++i) {
            total += counts[arguments[i]] || 0;
        }
        return total;
    }

    function clearPercentValue(afterDot: var) : var {
        let players = root.playerCount();
        if (players <= 0) {
            return 0;
        }
        let count = 0;
        for (let i = 1; i < arguments.length; ++i) {
            count += root.clearCount(arguments[i]);
        }
        return afterDot
            ? Math.floor(count * 1000 / players) % 10
            : Math.floor(count * 100 / players);
    }

    function applyStatsToSelectContext() : var {
        let targetChart = root.chart;
        if (root.host.effectiveScreenKey !== "select"
                || !targetChart || !targetChart.md5
                || !root.modelMatchesCurrentChart) {
            return;
        }
        let currentSnapshot = root.snapshot();
        root.selectContext.setRankingStats(
            targetChart.md5,
            currentSnapshot.clearCounts,
            currentSnapshot.playerCount,
            currentSnapshot.totalPlayCount,
            currentSnapshot.playerRank);
    }

    function handleModelChanged(tryOpenRanking: var, tryOpenInternetRanking: var) : void {
        root.applyStatsToSelectContext();
        root.refreshHostRankingStatusOptions();
        if (tryOpenRanking && root.openWhenReady && !root.rankingModel.loading) {
            Qt.callLater(root.finishOpenRanking);
        }
        if (tryOpenInternetRanking && root.internetOpenWhenReady && !root.rankingModel.loading) {
            Qt.callLater(() => {
                if (!root.finishOpenInternetRanking()) {
                    root.internetOpenWhenReady = false;
                }
            });
        }
    }

    function startTransition(action: var) : var {
        if (root.transitionPhase !== 0) {
            return false;
        }
        root.closeWhenReady = false;
        root.transitionAction = action;
        root.transitionStartSkinTime = root.host.selectLiveSkinTime;
        root.transitionPhase = 175;
        root.transitionTimer.restart();
        return true;
    }

    function clearTransition() : void {
        root.transitionPhase = 0;
        root.transitionAction = "";
        root.transitionTimer.stop();
    }

    function finishTransition() : void {
        let shouldClose = root.closeWhenReady && root.selectContext.rankingMode;
        root.clearTransition();
        root.closeWhenReady = false;
        if (shouldClose) {
            root.startTransition("close");
        }
    }

    function enterPostSwapTimer() : void {
        root.transitionStartSkinTime = root.host.selectLiveSkinTime;
        root.transitionPhase = 176;
        root.transitionTimer.restart();
    }

    function performOpen() : var {
        if (!root.modelMatchesCurrentChart || root.rankingModel.loading) {
            return false;
        }
        let currentSnapshot = root.snapshot();
        if (currentSnapshot.entries.length === 0) {
            let targetChart = root.chart;
            if (targetChart && targetChart.md5) {
                root.selectContext.setRankingStats(targetChart.md5, {}, 0, 0, 0);
            }
            return false;
        }
        let opened = root.selectContext.showRanking(
            currentSnapshot.entries,
            currentSnapshot.clearCounts,
            currentSnapshot.playerCount,
            currentSnapshot.totalPlayCount,
            currentSnapshot.playerRank,
            root.providerEnum());
        if (opened) {
            root.host.playOneShot(root.optionOpenSound);
        }
        return opened;
    }

    function performClose() : var {
        if (!root.selectContext.rankingMode) {
            return false;
        }
        let closed = root.selectContext.hideRanking();
        if (closed) {
            root.host.playOneShot(root.optionCloseSound);
        }
        return closed;
    }

    function advanceTransition() : var {
        if (root.transitionPhase === 175) {
            let ok = root.transitionAction === "close"
                ? root.performClose()
                : root.performOpen();
            if (!ok) {
                root.closeWhenReady = false;
                root.clearTransition();
                return;
            }
            root.enterPostSwapTimer();
            return;
        }
        root.finishTransition();
    }

    function requestFetch(chart: var) : var {
        let targetMd5 = chart && chart.md5 ? String(chart.md5) : "";
        if (targetMd5.length === 0) {
            return false;
        }

        let changed = root.commitRequest(chart);
        root.openWhenReady = true;
        if (!changed && !root.rankingModel.loading) {
            root.rankingModel.refresh();
        }
        return true;
    }

    function tachiGameId(chart: var) : var {
        switch (chart ? (chart.keymode || 0) : 0) {
        case 5:
        case 7:
            return "bms-7k";
        case 10:
        case 14:
            return "bms-14k";
        default:
            return "";
        }
    }

    function internetRankingUrl(chart: var) : var {
        let targetMd5 = chart && chart.md5 ? String(chart.md5) : "";
        if (targetMd5.length === 0) {
            return "";
        }

        switch (root.providerEnum()) {
        case OnlineRankingModel.RhythmGame: {
            let vars = root.host.mainGeneralVarsRef;
            let baseUrl = vars && vars.websiteBaseUrl
                ? String(vars.websiteBaseUrl)
                : "https://rhythmgame.eu";
            while (baseUrl.length > 0 && baseUrl.charAt(baseUrl.length - 1) === "/") {
                baseUrl = baseUrl.substring(0, baseUrl.length - 1);
            }
            return baseUrl + "/charts/" + targetMd5;
        }
        case OnlineRankingModel.Tachi: {
            if (!root.modelMatchesCurrentChart || !root.rankingModel.chartId) {
                return "";
            }
            let tachiGame = root.tachiGameId(chart);
            return tachiGame.length > 0
                ? "https://boku.tachi.ac/games/" + tachiGame + "/charts/" + root.rankingModel.chartId
                : "";
        }
        case OnlineRankingModel.LR2IR:
        default:
            return "https://lr2ir.com/charts/" + targetMd5 + "#status";
        }
    }

    function finishOpenInternetRanking() : var {
        let url = root.internetRankingUrl(root.chart);
        if (url.length === 0) {
            return false;
        }
        root.internetOpenWhenReady = false;
        let opened = Qt.openUrlExternally(url);
        if (opened === false) {
            console.warn("[LR2] Failed to open internet ranking URL: " + url);
        }
        return opened !== false;
    }

    function openInternetRanking() : var {
        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return false;
        }

        let changed = root.commitRequest(targetChart);
        if (root.providerEnum() === OnlineRankingModel.Tachi
                && (!root.modelMatchesCurrentChart
                    || !root.rankingModel.chartId
                    || root.rankingModel.loading)) {
            root.internetOpenWhenReady = true;
            if (!changed && !root.rankingModel.loading) {
                root.rankingModel.refresh();
            }
            return true;
        }

        return root.finishOpenInternetRanking();
    }

    function finishOpenRanking() : var {
        if (root.transitionPhase !== 0) {
            return true;
        }
        if (!root.modelMatchesCurrentChart || root.rankingModel.loading) {
            return false;
        }
        let currentSnapshot = root.snapshot();
        if (currentSnapshot.entries.length === 0) {
            root.openWhenReady = false;
            let targetChart = root.chart;
            if (targetChart && targetChart.md5) {
                root.selectContext.setRankingStats(targetChart.md5, {}, 0, 0, 0);
            }
            return false;
        }
        root.openWhenReady = false;
        return root.startTransition("open");
    }

    function openRanking() : var {
        if (!root.host.selectNavigationReady() || root.host.selectPanel === 1) {
            return false;
        }
        if (root.transitionPhase !== 0) {
            return true;
        }

        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return false;
        }
        root.commitRequest(targetChart);
        if (!root.modelMatchesCurrentChart
            || root.rankingModel.loading
            || root.currentPlayerCount === 0) {
            return root.requestFetch(targetChart);
        }
        return root.finishOpenRanking();
    }

    function closeRanking() : var {
        root.openWhenReady = false;
        if (root.transitionPhase !== 0) {
            if (root.transitionAction === "open") {
                if (root.transitionPhase === 175 && !root.selectContext.rankingMode) {
                    root.closeWhenReady = false;
                    root.clearTransition();
                    return true;
                }
                root.closeWhenReady = true;
            }
            return true;
        }
        if (!root.selectContext.rankingMode) {
            return false;
        }
        return root.startTransition("close");
    }

    function pauseActivity() : void {
        root.transitionTimer.stop();
        root.autoStatsFetchTimer.stop();
    }
}
