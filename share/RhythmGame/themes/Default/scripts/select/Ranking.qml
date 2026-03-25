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
    property var provider: OnlineRankingModel.LR2IR
    property bool loading: onlineRankingModel.loading
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
    readonly property string keymode: {
        // convert keymode to string for tachi provider
        switch (ranking.chartData.keymode) {
            case 5:
            case 7:
                return "7K";
            case 10:
            case 14:
                return "14K";
        }
        return "";
    }
    readonly property string chartId: onlineRankingModel.chartId

    component RankingModel: OnlineRankingModel {
        md5: ranking.enabled ? ranking.md5 : "" // disable loading when in gameplay
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
        webApiUrl: profile.vars.generalVars.webApiUrl
        property var entries: {
            let entries = onlineRankingModel.rankingEntries;
            let ourUserId = ranking.profile.onlineUserData?.userId;
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
                    bestPointsGuid: ranking.bestPointsScore.result.guid
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

    RankingModel {
        id: tachi
        provider: OnlineRankingModel.Tachi
    }
    RankingModel {
        id: lr2ir
        provider: OnlineRankingModel.LR2IR
    }
    RankingModel {
        id: rhythmGame
        provider: OnlineRankingModel.RhythmGame
    }

    property var onlineRankingModel: {
        switch (ranking.provider) {
            case OnlineRankingModel.RhythmGame:
                return rhythmGame;
            case OnlineRankingModel.LR2IR:
                return lr2ir;
            case OnlineRankingModel.Tachi:
                return tachi;
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
        running: (onlineRankingModel.loading && ranking.page === 1) ||
            (onlineRankingModel.loading && !ranking.entries.length)
        visible: running
    }

    Loader {
        anchors.fill: parent
        sourceComponent: rankingPages
        active: !!ranking.md5 && onlineRankingModel.entries.length
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
                bestPointsScore: ranking.bestPointsScore
                bestClearTypeScore: ranking.bestClearTypeScore
                keymode: ranking.keymode
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