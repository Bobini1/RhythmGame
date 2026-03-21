import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls

Image {
    id: ranking

    required property var chartData
    readonly property var md5: chartData.md5
    visible: !!md5
    property int page: 0
    property var profile: Rg.profileList.mainProfile
    readonly property var ranking: onlineRankingModel
    property var bestPointsScore
    property var bestClearTypeScore
    readonly property var entries: onlineRankingModel.entries
    readonly property var playerCount: entries.length
    readonly property var clearCounts: {
        let counts = {};
        for (let i = 0; i < entries.length; i++) {
            let clearType = entries[i].bestClearType;
            counts[clearType] = (counts[clearType] ?? 0) + 1;
        }
        return counts;
    }
    onEnabledChanged: {
        if (enabled) {
            onlineRankingModel.refresh();
        }
    }

    OnlineRankingModel {
        id: onlineRankingModel
        md5: ranking.md5
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
        webApiUrl: profile.vars.generalVars.webApiUrl
        provider: OnlineRankingModel.LR2IR
        property var entries: {
            let entries = onlineRankingModel.rankingEntries;
            let ourUserId = ranking.profile.onlineUserId;
            if (provider !== OnlineRankingModel.LR2IR) {
                return entries;
            }
            if (ranking.bestPointsScore && ranking.bestClearTypeScore) {
                let entry = {
                    rank: -1,
                    userName: ranking.profile.vars.generalVars.name,
                    userId: ourUserId,
                    bestClearType: ranking.bestClearTypeScore.result.clearType,
                    bestPoints: ranking.bestPointsScore.result.points,
                    maxPoints: ranking.bestPointsScore.result.maxPoints,
                    bestClearTypeGuid: ranking.bestClearTypeScore.result.guid,
                    bestPointsGuid: ranking.bestPointsScore.result.guid,
                    bestPointsScore: ranking.bestPointsScore,
                    bestClearTypeScore: ranking.bestClearTypeScore
                };
                // insert where it belongs based on bestPoints
                let newEntries = [];
                let inserted = false;
                for (let i = 0; i < entries.length; i++) {
                    if (!inserted && entry.bestPoints > entries[i].bestPoints) {
                        newEntries.push(entry);
                        inserted = true;
                    }
                    newEntries.push(entries[i]);
                }
                if (!inserted) {
                    newEntries.push(entry);
                }
                return newEntries;
            }
            return entries;
        }
    }

    source: root.iniImagesUrl + "ir.png/ir" + (ranking.page + 1)

    Image {
        id: rankingBack
        anchors.top: parent.top
        source: root.iniImagesUrl + "ir.png/irback"
        x: ranking.page === 0 ? parent.width - width - 42 : 42
        z: -1
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (ranking.page === 0) {
                    ranking.page = 1;
                } else {
                    ranking.page = 0;
                }
            }
        }
    }

    Text {
        text: qsTr("Ranking")
        fontSizeMode: Text.VerticalFit
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 16
        width: rankingBack.width
        x: 42
        anchors.top: rankingBack.top
        anchors.topMargin: 15
        anchors.bottom: rankingBack.bottom
    }

    Text {
        text: qsTr("Stats")
        fontSizeMode: Text.VerticalFit
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 16
        width: rankingBack.width
        x: parent.width - rankingBack.width - 42
        anchors.top: rankingBack.top
        anchors.topMargin: 15
        anchors.bottom: rankingBack.bottom
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: onlineRankingModel.loading
        visible: running
    }

    Loader {
        anchors.fill: parent
        sourceComponent: rankingPages
        active: !!ranking.md5 && !onlineRankingModel.loading
    }
    Component {
        id: rankingPages
        Item {
            RankingTop {
                visible: ranking.page === 0
                anchors.fill: parent
                anchors.margins: 28
                anchors.topMargin: 54
                profile: ranking.profile
                chartData: ranking.chartData
                provider: onlineRankingModel.provider
                entries: onlineRankingModel.entries.slice(0, 7)
            }

            RankingStats {
                visible: ranking.page === 1
                anchors.fill: parent
                anchors.margins: 28
                anchors.topMargin: 54
                clearCounts: ranking.clearCounts
                playerCount: ranking.playerCount
            }
        }
    }
}