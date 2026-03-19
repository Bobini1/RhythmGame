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
    onEnabledChanged: {
        if (enabled) {
            onlineRankingModel.refresh();
        }
    }

    OnlineRankingModel {
        id: onlineRankingModel
        md5: ranking.md5
        limit: 7
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
        webApiUrl: profile.vars.generalVars.webApiUrl
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
                rankingModel: onlineRankingModel
                profile: ranking.profile
                chartData: ranking.chartData
            }

            RankingStats {
                visible: ranking.page === 1
                anchors.fill: parent
                anchors.margins: 28
                anchors.topMargin: 54
                clearCounts: onlineRankingModel.clearCounts
                playerCount: onlineRankingModel.playerCount
            }
        }
    }
}