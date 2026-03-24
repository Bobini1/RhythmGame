import RhythmGameQml
import QtQuick

OnlineRankingModel {
    id: rankingModel
    sortBy: OnlineRankingModel.ScorePct
    sortDir: OnlineRankingModel.Desc
    required property var userId
    property int size: {
        if (provider === OnlineRankingModel.LR2IR) {
            return rankingEntries.length + 1;
        }
        return rankingEntries.length;
    }
    property bool positionLoading: provider !== OnlineRankingModel.LR2IR

    property int position: {
        let entries = rankingEntries;
        if (provider === OnlineRankingModel.LR2IR) {
            let points = Math.max(side.score.result.points, side.oldBestPointsScore?.result?.points || 0);
            for (let i = 0; i < entries.length; i++) {
                if (points > entries[i].bestPoints) {
                    return i + 1;
                }
            }
            return entries.length + 1;
        } else if (provider === OnlineRankingModel.RhythmGame) {
            for (let i = 0; i < entries.length; i++) {
                if (entries[i].userId === rankingModel.userId) {
                    return i + 1;
                }
            }
            return 0;
        } if (provider === OnlineRankingModel.Tachi) {
            let points = Math.max(side.score.result.points, side.oldBestPointsScore?.result?.points || 0);
            for (let i = 0; i < entries.length; i++) {
                if (entries[i].userId === rankingModel.userId)
                {
                    return i + 1;
                }
            }
            return 0;
        }
    }

    property int oldPosition: {
        let entries = rankingEntries;
        if (provider === OnlineRankingModel.LR2IR) {
            if (side.oldBestPointsScore?.result?.points === undefined) {
                return 0;
            }
            let points = side.oldBestPointsScore?.result?.points || 0;
            for (let i = 0; i < entries.length; i++) {
                if (points > entries[i].bestPoints) {
                    return i + 1;
                }
            }
            return entries.length + 1;
        }
        return 0;
    }

    function fetchPosition() {
        if (provider === OnlineRankingModel.LR2IR) {
            return;
        }
        if (!userId) {
            return;
        }
        Rg.onlineScores.getRankingEntryAtTimestamp(side.profile.vars.generalVars.webApiUrl, userId, side.score.result.md5, Date.now() / 1000 - 5000, provider).then((entry) => {
            if (entry) {
                // go through entries to find the position we would have based on bestPoints
                let points = entry.bestPoints;
                for (let i = 0; i < rankingEntries.length; i++) {
                    if (points > rankingEntries[i].bestPoints) {
                        if (position !== 0 && i + 1 < position) {
                            rankingModel.oldPosition = i + 1;
                        } else {
                            rankingModel.oldPosition = i;
                        }
                        positionLoading = false;
                        return;
                    }
                }
                rankingModel.oldPosition = rankingEntries.length;
            }
            positionLoading = false;
        }, () => {
            console.error("Failed to get old ranking position");
            positionLoading = false;
        });
    }

    onRankingEntriesChanged: fetchPosition()
    onUserIdChanged: fetchPosition()
    onProviderChanged: fetchPosition()
    onWebApiUrlChanged: fetchPosition()
    onLoadingChanged: {
        if (!loading) {
            fetchPosition();
        }
    }
}