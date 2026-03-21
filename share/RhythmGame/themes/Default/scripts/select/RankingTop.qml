import QtQuick
import RhythmGameQml

Column {
    id: ranking
    spacing: 6
    property alias rankingModel: rankingRepeater.model
    property var profile
    property var chartData
    readonly property var md5: chartData.md5
    readonly property var path: chartData.path

    Repeater {
        id: rankingRepeater
        delegate: Row {
            id: rankingEntry
            spacing: 10
            required property int rank
            required property var userName
            required property var userId
            required property var bestClearType
            required property var bestPoints
            required property var maxPoints
            required property var bestClearTypeGuid
            required property var bestPointsGuid
            anchors.left: parent.left
            anchors.right: parent.right
            height: rankImage.height

            Image {
                id: rankImage
                source: root.iniImagesUrl + "ir.png/rank_" + rankingEntry.rank
            }
            Text {
                id: userNameText
                text: rankingEntry.userName
                color: rankingEntry.userId === ranking.profile.onlineUserId ? "#ff0066" : "black"
                font.pixelSize: 24
                elide: Text.ElideRight
                anchors.baseline: parent.bottom
                anchors.baselineOffset: -1
                fontSizeMode: Text.VerticalFit
                width: 132
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: userNameText.width - userNameText.implicitWidth
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        let url;
                        switch (ranking.rankingModel.provider) {
                            case OnlineRankingModel.RhythmGame:
                                url = Rg.onlineLinks.scoresByUserOnChart(
                                    ranking.profile.vars.generalVars.websiteBaseUrl,
                                    rankingEntry.userId,
                                    ranking.chartData.md5);
                                break;
                            case OnlineRankingModel.LR2IR:
                                url = "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=mypage&playerid=" + rankingEntry.userId;
                                break;
                        }
                        Qt.openUrlExternally(url);

                    }
                }
            }
            function onScoreLoaded(event, score) {
                let replay = true;
                if (event.button === Qt.RightButton) {
                    globalRoot.openResult([score], [Rg.profileList.mainProfile], ranking.chartData);
                    return;
                }
                if (event.button === Qt.MiddleButton) {
                    replay = false;
                }
                globalRoot.openChart(ranking.path, Rg.profileList.mainProfile, false, replay, score, null, false, false, null);
            }
            Image {
                id: clearTypeImage
                source: root.iniImagesUrl + "parts.png/ranking_" + rankingEntry.bestClearType
                property bool loading: false
                opacity: loading ? 0.5 : 1
                anchors.bottom: rankImage.bottom
                anchors.bottomMargin: -1
                MouseArea {
                    anchors.fill: parent
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    enabled: rankingEntry.bestClearTypeGuid
                    onClicked: (event) => {
                        clearTypeImage.loading = true;
                        Rg.onlineScores.getScoreByGuid(
                            ranking.profile.vars.generalVars.webApiUrl,
                            rankingEntry.bestClearTypeGuid).then(
                                (score) => {
                                clearTypeImage.loading = false;
                                onScoreLoaded(event, score);
                            },
                                () => {
                                clearTypeImage.loading = false;
                            });
                    }
                }
            }

            Text {
                id: pointsText
                text: rankingEntry.bestPoints
                font.pixelSize: 24
                color: "#ff0066"
                verticalAlignment: Text.AlignBottom
                horizontalAlignment: Text.AlignRight
                anchors.baseline: rankImage.bottom
                anchors.baselineOffset: -1
                width: 160 - clearTypeImage.width - rankingEntry.spacing
                property bool loading: false
                opacity: loading ? 0.5 : 1
                MouseArea {
                    anchors.fill: parent
                    anchors.leftMargin: pointsText.width - pointsText.implicitWidth
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    enabled: rankingEntry.bestPointsGuid
                    onClicked: (event) => {
                        pointsText.loading = true;
                        Rg.onlineScores.getScoreByGuid(
                            ranking.profile.vars.generalVars.webApiUrl,
                            rankingEntry.bestPointsGuid).then(
                            (score) => {
                                pointsText.loading = false;
                                onScoreLoaded(event, score);
                            },
                            () => {
                                pointsText.loading = false;
                            });
                    }
                }
            }
            Text {
                id: percentageText
                text: (rankingEntry.bestPoints / rankingEntry.maxPoints * 100).toFixed(1) + "%"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignBottom
                anchors.baseline: rankImage.bottom
                anchors.baselineOffset: -1
                width: 41
                font.bold: true
            }
        }
    }
}