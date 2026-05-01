pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    required property var selectContext
    required property var optionOpenSound
    required property var optionCloseSound

    readonly property var chart: root.currentChart()
    readonly property string md5: root.chart && root.chart.md5 ? String(root.chart.md5) : ""
    property string requestMd5: ""
    property bool openWhenReady: false
    property bool internetOpenWhenReady: false
    property int transitionPhase: 0 // 175 before list swap, 176 after list swap.
    property string transitionAction: ""
    property int transitionStartSkinTime: 0
    property var cachedSnapshot: null
    property string cachedSnapshotTargetMd5: ""
    property string cachedSnapshotLoadedMd5: ""
    property var cachedSnapshotEntrySource: null
    property var emptyRankingEntries: []
    property int cachedSnapshotPlayerCount: -1
    property int cachedSnapshotScoreCount: -1
    property int cachedSnapshotScoreRevision: -1
    property int cachedSnapshotProvider: -1
    property int cachedSnapshotProfileUserId: -1
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
        webApiUrl: root.host.mainGeneralVars() ? root.host.mainGeneralVars().webApiUrl : ""

        onMd5Changed: root.handleModelChanged(false, false)
        onLoadingChanged: root.handleModelChanged(true, true)
        onRankingEntriesChanged: root.handleModelChanged(true, false)
        onPlayerCountChanged: root.handleModelChanged(false, false)
        onScoreCountChanged: root.handleModelChanged(false, false)
        onClearCountsChanged: root.handleModelChanged(true, false)
        onChartIdChanged: {
            if (root.internetOpenWhenReady && !loading) {
                Qt.callLater(root.finishOpenInternetRanking);
            }
        }
    }

    property Timer transitionTimer: Timer {
        interval: root.transitionDuration
        repeat: false
        onTriggered: root.advanceTransition()
    }

    function currentChart() {
        if (root.host.isResultScreen()) {
            return root.host.resultChartData();
        }
        if (root.host.effectiveScreenKey !== "select") {
            return null;
        }
        if (root.selectContext.cachedSelectedChartData) {
            return root.selectContext.cachedSelectedChartData;
        }
        if (root.selectContext.rankingMode && root.selectContext.rankingBaseItem) {
            return root.selectContext.rankingBaseItem;
        }

        let item = root.selectContext.focusedItem;
        return root.selectContext.isChart(item) || root.selectContext.isEntry(item) ? item : null;
    }

    function md5ForChart(chart) {
        return chart && chart.md5 ? String(chart.md5) : "";
    }

    function commitRequest(chart) {
        let nextMd5 = chart === undefined ? root.md5 : root.md5ForChart(chart);
        if (root.requestMd5 !== nextMd5) {
            root.requestMd5 = nextMd5;
            return true;
        }
        return false;
    }

    function providerEnum() {
        let vars = root.host.mainGeneralVars();
        return vars ? vars.rankingProvider : OnlineRankingModel.RhythmGame;
    }

    function matchesCurrentChart() {
        let targetMd5 = root.md5.length > 0 ? root.md5.toLowerCase() : "";
        let loadedMd5 = root.rankingModel.md5 ? String(root.rankingModel.md5).toLowerCase() : "";
        return targetMd5.length > 0 && targetMd5 === loadedMd5;
    }

    function localEntry() {
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
            scoreCount: scoreList.length
        };
    }

    function buildEntries(source, provider) {
        if (!root.matchesCurrentChart()) {
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

    function entries() {
        return root.snapshot().entries;
    }

    function clearCounts(entries) {
        let counts = {};
        for (let entry of entries || []) {
            let clearType = entry.bestClearType || "NOPLAY";
            counts[clearType] = (counts[clearType] || 0) + 1;
        }
        return counts;
    }

    function playerCount(entries) {
        if (entries === undefined) {
            return root.snapshot().playerCount;
        }
        if (!root.matchesCurrentChart()) {
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

    function totalPlayCount(entries) {
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

    function profileUserId() {
        let profile = Rg.profileList ? Rg.profileList.mainProfile : null;
        if (!profile) {
            return 0;
        }
        if (root.providerEnum() === OnlineRankingModel.Tachi) {
            return profile.tachiData ? Number(profile.tachiData.userId || 0) : 0;
        }
        return profile.onlineUserData ? Number(profile.onlineUserData.userId || 0) : 0;
    }

    function playerRank(entries) {
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

    function snapshot() {
        let targetMd5 = root.md5;
        let loadedMd5 = root.rankingModel.md5 ? String(root.rankingModel.md5) : "";
        let source = root.rankingModel.rankingEntries || root.emptyRankingEntries;
        let playerCountValue = Number(root.rankingModel.playerCount || 0);
        let scoreCountValue = Number(root.rankingModel.scoreCount || 0);
        let scoreRevision = root.selectContext.scoreRevision;
        let provider = root.providerEnum();
        let userId = root.profileUserId();

        if (root.cachedSnapshot
                && root.cachedSnapshotTargetMd5 === targetMd5
                && root.cachedSnapshotLoadedMd5 === loadedMd5
                && root.cachedSnapshotEntrySource === source
                && root.cachedSnapshotPlayerCount === playerCountValue
                && root.cachedSnapshotScoreCount === scoreCountValue
                && root.cachedSnapshotScoreRevision === scoreRevision
                && root.cachedSnapshotProvider === provider
                && root.cachedSnapshotProfileUserId === userId) {
            return root.cachedSnapshot;
        }

        let currentEntries = root.buildEntries(source, provider);
        root.cachedSnapshot = {
            entries: currentEntries,
            clearCounts: root.clearCounts(currentEntries),
            playerCount: root.playerCount(currentEntries),
            totalPlayCount: root.totalPlayCount(currentEntries),
            playerRank: root.playerRank(currentEntries)
        };
        root.cachedSnapshotTargetMd5 = targetMd5;
        root.cachedSnapshotLoadedMd5 = loadedMd5;
        root.cachedSnapshotEntrySource = source;
        root.cachedSnapshotPlayerCount = playerCountValue;
        root.cachedSnapshotScoreCount = scoreCountValue;
        root.cachedSnapshotScoreRevision = scoreRevision;
        root.cachedSnapshotProvider = provider;
        root.cachedSnapshotProfileUserId = userId;
        return root.cachedSnapshot;
    }

    function entryAt(index) {
        let currentEntries = root.entries();
        return index >= 0 && index < currentEntries.length ? currentEntries[index] : null;
    }

    function entryName(index) {
        let entry = root.entryAt(index);
        if (!entry) {
            return "";
        }
        let name = entry.userName || entry.owner || entry.player || "";
        return name.length > 0 ? name : "YOU";
    }

    function entryClearValue(index) {
        let entry = root.entryAt(index);
        return entry ? root.host.clearTypeValue(entry.bestClearType || "NOPLAY") : 0;
    }

    function entryExScore(index) {
        let entry = root.entryAt(index);
        return entry ? Math.floor(entry.bestPoints || 0) : 0;
    }

    function clearCount() {
        let counts = root.snapshot().clearCounts;
        let total = 0;
        for (let i = 0; i < arguments.length; ++i) {
            total += counts[arguments[i]] || 0;
        }
        return total;
    }

    function clearPercentValue(afterDot) {
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

    function applyStatsToSelectContext() {
        let targetChart = root.chart;
        if (root.host.effectiveScreenKey !== "select"
                || !targetChart || !targetChart.md5
                || !root.matchesCurrentChart()
                || root.rankingModel.loading) {
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

    function handleModelChanged(tryOpenRanking, tryOpenInternetRanking) {
        root.applyStatsToSelectContext();
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

    function statusOption() {
        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return 600;
        }
        if (!root.matchesCurrentChart()) {
            return root.rankingModel.loading ? 601 : 600;
        }
        if (root.rankingModel.loading) {
            return 601;
        }
        return root.playerCount() > 0 ? 602 : 603;
    }

    function startTransition(action) {
        if (root.transitionPhase !== 0) {
            return false;
        }
        root.transitionAction = action;
        root.transitionStartSkinTime = root.host.selectLiveSkinTime;
        root.transitionPhase = 175;
        root.transitionTimer.restart();
        return true;
    }

    function clearTransition() {
        root.transitionPhase = 0;
        root.transitionAction = "";
        root.transitionTimer.stop();
    }

    function enterPostSwapTimer() {
        root.transitionStartSkinTime = root.host.selectLiveSkinTime;
        root.transitionPhase = 176;
        root.transitionTimer.restart();
    }

    function performOpen() {
        if (!root.matchesCurrentChart() || root.rankingModel.loading) {
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
            currentSnapshot.playerRank);
        if (opened) {
            root.host.playOneShot(root.optionOpenSound);
        }
        return opened;
    }

    function performClose() {
        if (!root.selectContext.rankingMode) {
            return false;
        }
        let closed = root.selectContext.hideRanking();
        if (closed) {
            root.host.playOneShot(root.optionCloseSound);
        }
        return closed;
    }

    function advanceTransition() {
        if (root.transitionPhase === 175) {
            let ok = root.transitionAction === "close"
                ? root.performClose()
                : root.performOpen();
            if (!ok) {
                root.clearTransition();
                return;
            }
            root.enterPostSwapTimer();
            return;
        }
        root.clearTransition();
    }

    function requestFetch(chart) {
        let targetMd5 = chart && chart.md5 ? String(chart.md5) : "";
        if (targetMd5.length === 0) {
            return false;
        }

        let changed = root.commitRequest(chart);
        root.openWhenReady = true;
        if (!changed && !root.rankingModel.loading) {
            root.rankingModel.refresh();
        }
        return false;
    }

    function tachiKeymode(chart) {
        switch (chart ? (chart.keymode || 0) : 0) {
        case 5:
        case 7:
            return "7K";
        case 10:
        case 14:
            return "14K";
        default:
            return "";
        }
    }

    function internetRankingUrl(chart) {
        let targetMd5 = chart && chart.md5 ? String(chart.md5) : "";
        if (targetMd5.length === 0) {
            return "";
        }

        switch (root.providerEnum()) {
        case OnlineRankingModel.RhythmGame: {
            let vars = root.host.mainGeneralVars();
            let baseUrl = vars && vars.websiteBaseUrl
                ? String(vars.websiteBaseUrl)
                : "https://rhythmgame.eu";
            while (baseUrl.length > 0 && baseUrl.charAt(baseUrl.length - 1) === "/") {
                baseUrl = baseUrl.substring(0, baseUrl.length - 1);
            }
            return baseUrl + "/charts/" + targetMd5;
        }
        case OnlineRankingModel.Tachi: {
            if (!root.matchesCurrentChart() || !root.rankingModel.chartId) {
                return "";
            }
            let keymode = root.tachiKeymode(chart);
            return keymode.length > 0
                ? "https://boku.tachi.ac/games/bms/" + keymode + "/charts/" + root.rankingModel.chartId
                : "";
        }
        case OnlineRankingModel.LR2IR:
        default:
            return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5="
                + targetMd5 + "#status";
        }
    }

    function finishOpenInternetRanking() {
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

    function openInternetRanking() {
        root.host.updateDisplayedSelectChart();
        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return false;
        }

        let changed = root.commitRequest(targetChart);
        if (root.providerEnum() === OnlineRankingModel.Tachi
                && (!root.matchesCurrentChart()
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

    function finishOpenRanking() {
        if (root.transitionPhase !== 0) {
            return true;
        }
        if (!root.matchesCurrentChart() || root.rankingModel.loading) {
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

    function openRanking() {
        if (!root.host.selectNavigationReady() || root.host.selectPanel === 1) {
            return false;
        }
        if (root.transitionPhase !== 0) {
            return true;
        }

        root.host.updateDisplayedSelectChart();
        let targetChart = root.chart;
        if (!targetChart || !targetChart.md5) {
            return false;
        }
        root.commitRequest(targetChart);
        if (!root.matchesCurrentChart()
            || root.rankingModel.loading
            || root.playerCount() === 0) {
            return root.requestFetch(targetChart);
        }
        return root.finishOpenRanking();
    }

    function closeRanking() {
        root.openWhenReady = false;
        if (root.transitionPhase !== 0) {
            return true;
        }
        if (!root.selectContext.rankingMode) {
            return false;
        }
        return root.startTransition("close");
    }

    function pauseActivity() {
        root.transitionTimer.stop();
    }
}
