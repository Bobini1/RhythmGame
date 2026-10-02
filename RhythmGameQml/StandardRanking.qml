import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardRanking
    \inqmlmodule RhythmGameQml
    \brief Loads a chart's online ranking and opens its ranking page.

    Set \l chartData to a chart. The active profile supplies the default account,
    server and ranking provider. StandardSelectController supplies this component
    as its \c ranking property. It can also be used on results or other screens.

    For a result, also set \l score and \l profile for that player. The component
    refreshes after score submission and supplies \l previousPosition alongside
    \l position. Course summaries have no online ranking; individual course
    stages use their normal chart and score.

    Bind your display to \l entries, \l playerCount, \l clearCounts and \l loading.
    LR2IR entries include the profile's local best score, since LR2IR cannot
    identify the current profile's account. Local results come directly from
    the profile's cache and update after saves and imports. Positions are
    one-based, or zero when the player has no entry.

    Call \l open to open the provider's website. No ranking display is required.
    Set \l entriesEnabled to false to use only this action. Tachi needs a chart-ID
    lookup before its link is available; open handles that lookup without
    downloading the leaderboard. Changing the chart, provider or profile, or
    disabling the component, cancels a pending open request.

    Override \l provider or \l url for a different source, or \l openUrlAction
    to present the link yourself. Advanced themes can use OnlineRankingModel
    directly when they need different queries or sorting.
*/
Item {
    id: root
    visible: false

    /*! Chart metadata, or null when a folder or course is selected. */
    property var chartData: null
    /*! Profile whose account, saved scores and server settings are used. */
    property var profile: Rg.profileList.mainProfile
    /*! Normal chart score being shown on a result screen, or null on selection. */
    property var score: null
    /*! Ranking provider. Follows the profile's choice unless overridden. */
    property int provider: profile?.vars.generalVars.rankingProvider
        ?? OnlineRankingModel.RhythmGame
    /*! Controls leaderboard loading. The open action remains available when false. */
    property bool entriesEnabled: true
    /*! Called with the website URL. Return false if it could not be opened. */
    property var openUrlAction: websiteUrl => Qt.openUrlExternally(websiteUrl)

    /*! Chart hash used by the ranking. Empty for unsupported selections. */
    readonly property string md5: score instanceof BmsScoreCourse ? ""
        : String(chartData?.md5 || score?.result?.md5 || "")
    /*! Whether the current selection has a supported ranking provider. */
    readonly property bool available: md5.length > 0
        && [OnlineRankingModel.RhythmGame, OnlineRankingModel.LR2IR,
            OnlineRankingModel.Tachi].indexOf(provider) >= 0
    /*! Whether ranking data or the previous result position is being loaded. */
    readonly property bool loading: rankingModel.loading
        || (state.queriesMatch && state.queriesLoading)
    /*! Whether open is waiting for a website link. */
    readonly property bool opening: linkReplies.count > 0
    /*! Ranking rows, including the local best score for LR2IR. */
    readonly property var entries: {
        if (!enabled || !entriesEnabled || !available) return [];
        const rows = rankingModel.rankingEntries.slice();
        const local = state.localEntry;
        if (local) {
            let index = rows.findIndex(row => local.bestPoints > row.bestPoints);
            rows.splice(index < 0 ? rows.length : index, 0, local);
        }
        return rows;
    }
    /*! Number of rows, including the local LR2IR entry when present. */
    readonly property int playerCount: entries.length
    /*! Clear-type counts for the displayed entries. */
    readonly property var clearCounts: {
        const counts = {};
        for (const row of entries)
            counts[row.bestClearType] = (counts[row.bestClearType] || 0) + 1;
        return counts;
    }
    /*! The profile's position, or zero if it has no entry. */
    readonly property int position: {
        if (provider === OnlineRankingModel.LR2IR)
            return state.localEntry ? entries.indexOf(state.localEntry) + 1 : 0;
        return state.userId
            ? entries.findIndex(row => row.userId === state.userId) + 1 : 0;
    }
    /*!
        Position of the player's earlier best score among the current opponents.
        Zero means no earlier score is available. This is not a historical snapshot
        of all players' ranks.
    */
    readonly property int previousPosition: {
        if (!score || !enabled || !entriesEnabled || !available) return 0;
        const previous = provider === OnlineRankingModel.LR2IR
            ? state.previousLocalScore : state.previousEntry;
        if (!previous) return 0;
        const points = previous.points ?? previous.bestPoints;
        let rank = 1;
        for (let i = 0; i < entries.length; ++i) {
            if (i + 1 !== position && entries[i].bestPoints >= points) ++rank;
        }
        return rank;
    }
    /*! Best local points score, including score when supplied. */
    readonly property var bestPointsResult: state.bestPoints(state.scores)
    /*! Best local clear score, including score when supplied. */
    readonly property var bestClearTypeResult: state.bestClear(state.scores)
    /*! Resolved Tachi game ID, such as bms-7k. Empty before it is known. */
    readonly property string tachiGameId: rankingModel.gameId
        || (state.resolvedLinkKey === state.linkKey ? state.resolvedChart?.gameId : "") || ""
    /*! Website link. Tachi's link is empty until its chart ID is resolved. */
    property string url: {
        if (!available) return "";
        const hash = encodeURIComponent(md5);
        switch (provider) {
        case OnlineRankingModel.RhythmGame: {
            const base = profile?.vars.generalVars.websiteBaseUrl || "";
            return base ? base.replace(/\/+$/, "") + "/charts/" + hash : "";
        }
        case OnlineRankingModel.LR2IR:
            return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5=" + hash;
        case OnlineRankingModel.Tachi: {
            const chartId = rankingModel.chartId
                || (state.resolvedLinkKey === state.linkKey ? state.resolvedChart?.chartId : "");
            return chartId && tachiGameId
                ? "https://boku.tachi.ac/games/" + encodeURIComponent(tachiGameId)
                    + "/charts/" + encodeURIComponent(chartId) : "";
        }
        }
        return "";
    }

    /*! Emitted when a requested website lookup or open operation fails. */
    signal openFailed()

    /*! Opens the ranking page. True means the open request was accepted. */
    function open() {
        if (!enabled || !available) return false;
        if (root.url) return state.openUrl(root.url);
        if (provider !== OnlineRankingModel.Tachi) return false;
        if (opening) return true;
        const key = state.linkKey;
        const requestedProfile = profile;
        linkReplies.track(Rg.onlineScores.getTachiChart(md5)).then(chart => {
            if (!root.enabled || state.linkKey !== key || root.profile !== requestedProfile) return;
            state.resolvedLinkKey = key;
            state.resolvedChart = chart;
            state.openUrl(root.url);
        }, () => {
            if (root.enabled && state.linkKey === key && root.profile === requestedProfile)
                root.openFailed();
        });
        return true;
    }

    /*! Reloads ranking data for the current chart and profile. */
    function refresh() {
        if (!enabled || !entriesEnabled || !available) return;
        rankingModel.refresh();
        Qt.callLater(state.refreshQueries);
    }

    OnlineRankingModel {
        id: rankingModel
        md5: root.enabled && root.entriesEnabled && root.available ? root.md5 : ""
        provider: root.provider
        webApiUrl: root.profile?.vars.generalVars.webApiUrl || ""
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
    }

    PendingReplyGroup { id: scoreReplies }
    PendingReplyGroup { id: linkReplies }

    QtObject {
        id: state
        readonly property var userId: root.provider === OnlineRankingModel.Tachi
            ? root.profile?.tachiData?.userId : root.profile?.onlineUserData?.userId
        readonly property string linkKey: JSON.stringify([root.enabled, root.md5,
            root.provider, root.profile?.vars.generalVars.websiteBaseUrl])
        readonly property string queryKey: JSON.stringify([linkKey, root.entriesEnabled,
            userId, root.score?.result?.guid, root.score?.result?.unixTimestamp,
            root.profile?.vars.generalVars.webApiUrl])
        property string resolvedLinkKey: ""
        property var resolvedChart: null
        property string loadedQueryKey: ""
        property var loadedProfile: null
        readonly property bool queriesMatch: loadedQueryKey === queryKey && loadedProfile === root.profile
        readonly property bool queriesLoading: scoreReplies.count > 0
        property var loadedPreviousEntry: null
        readonly property var previousEntry: queriesMatch ? loadedPreviousEntry : null
        readonly property var savedScores: {
            if (!root.enabled || !root.entriesEnabled || !root.available
                    || !root.profile) return [];
            return root.profile.scoreDb.chartResults[root.md5.toUpperCase()] || [];
        }
        readonly property var scores: root.score
            ? savedScores.filter(item => item.guid !== root.score.result.guid).concat([root.score.result])
            : savedScores
        readonly property var previousLocalScore: bestPoints(savedScores.filter(item =>
            item.guid !== root.score?.result?.guid
            && (!root.score?.result?.unixTimestamp
                || item.unixTimestamp < root.score.result.unixTimestamp)))
        readonly property var localEntry: {
            if (root.provider !== OnlineRankingModel.LR2IR
                    || !root.bestPointsResult || !root.bestClearTypeResult) return null;
            const points = root.bestPointsResult;
            const clear = root.bestClearTypeResult;
            return { userId: userId, userName: root.profile?.vars.generalVars.name || "",
                bestPoints: points.points, maxPoints: points.maxPoints,
                bestClearType: clear.clearType, bestPointsGuid: points.guid,
                bestClearTypeGuid: clear.guid };
        }

        function bestPoints(scores) {
            let best = null;
            for (const item of scores) {
                if (item.maxPoints > 0 && (!best
                        || item.points / item.maxPoints
                            > best.points / best.maxPoints)) best = item;
            }
            return best;
        }

        function bestClear(scores) {
            const order = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL",
                "HARD", "EXHARD", "FC", "PERFECT", "MAX"];
            let best = null;
            for (const item of scores) {
                const rank = order.indexOf(item.clearType);
                const previous = best ? order.indexOf(best.clearType) : -1;
                if (!best || rank > previous || (rank === previous
                        && item.points / (item.maxPoints || 1)
                            >= best.points / (best.maxPoints || 1))) best = item;
            }
            return best;
        }

        function openUrl(websiteUrl) {
            if (websiteUrl && root.openUrlAction(websiteUrl) !== false) return true;
            root.openFailed();
            return false;
        }

        function refreshQueries() {
            scoreReplies.cancelAll();
            loadedPreviousEntry = null;
            loadedQueryKey = queryKey;
            loadedProfile = root.profile;
            if (!root.enabled || !root.entriesEnabled || !root.available || !root.profile) return;
            const key = queryKey;
            const requestedProfile = root.profile;
            if (root.score && userId && root.provider !== OnlineRankingModel.LR2IR) {
                const before = (root.score.result.unixTimestamp || Math.floor(Date.now() / 1000)) - 1;
                scoreReplies.track(Rg.onlineScores.getRankingEntryAtTimestamp(
                    root.profile.vars.generalVars.webApiUrl, userId,
                    root.md5, before, root.provider)).then(entry => {
                    if (state.queryKey === key && root.profile === requestedProfile)
                        state.loadedPreviousEntry = entry;
                });
            }
        }

        onQueryKeyChanged: Qt.callLater(refreshQueries)
        onLinkKeyChanged: {
            if (linkReplies) linkReplies.cancelAll();
        }
    }

    onProfileChanged: {
        Qt.callLater(state.refreshQueries);
        if (linkReplies) linkReplies.cancelAll();
    }
    Component.onCompleted: Qt.callLater(state.refreshQueries)

    Connections {
        target: root.score
        ignoreUnknownSignals: true
        function onSubmissionStateChanged() { root.refresh(); }
    }
}
