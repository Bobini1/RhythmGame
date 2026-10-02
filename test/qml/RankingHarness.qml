import QtQuick
import RhythmGameQml

Item {
    id: harness
    property alias ranking: rankingImpl
    property alias provider: settings.rankingProvider
    property var chart: ({ md5: "chart-A" })
    property bool active: true
    property bool loadEntries: false
    property bool showResult: false
    property var account: ({ userId: 42 })
    property var openedUrls: []
    property int failures: 0
    property alias resultScore: resultScore
    property alias savedResults: scoreDb.chartResults

    QtObject {
        id: settings
        property int rankingProvider: OnlineRankingModel.RhythmGame
        property string webApiUrl: "https://ranking.example/api/"
        property string websiteBaseUrl: "https://ranking.example/"
        property string name: "Player"
    }
    QtObject {
        id: testProfile
        property var vars: ({ generalVars: settings })
        property var onlineUserData: harness.account
        property var tachiData: ({ userId: 43 })
        property QtObject scoreDb: QtObject {
            id: scoreDb
            property var chartResults: ({})
        }
    }
    QtObject {
        id: resultScore
        property int submissionState: 0
        property var result: ({ md5: "chart-A", guid: "current", points: 180,
            maxPoints: 200, clearType: "NORMAL", unixTimestamp: 200 })
    }

    StandardRanking {
        id: rankingImpl
        enabled: harness.active
        entriesEnabled: harness.loadEntries
        chartData: harness.chart
        profile: testProfile
        score: harness.showResult ? resultScore : null
        openUrlAction: url => {
            harness.openedUrls = harness.openedUrls.concat([String(url)]);
            return true;
        }
        onOpenFailed: ++harness.failures
    }
}
