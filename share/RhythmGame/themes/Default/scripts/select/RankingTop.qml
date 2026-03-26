pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

Column {
    id: ranking
    spacing: 6
    property var entries
    property var profile
    property var chartData
    property var provider
    readonly property var md5: chartData?.md5
    readonly property var path: chartData?.path
    property var bestPointsScore
    property var bestClearTypeScore
    property string keymode

    Repeater {
        id: rankingRepeater
        model: ranking.entries
        delegate: Row {
            id: userEntry
            spacing: 10
            anchors.left: parent.left
            anchors.right: parent.right
            height: rankImage.height

            readonly property bool isCurrentUser: {
                switch (ranking.provider) {
                    case OnlineRankingModel.RhythmGame:
                        return modelData.userId === ranking.profile.onlineUserData?.userId;
                    case OnlineRankingModel.Tachi:
                        return modelData.userId === ranking.profile.tachiData?.userId;
                    case OnlineRankingModel.LR2IR:
                        return !(modelData instanceof rankingEntry);
                }
            }

            Image {
                id: rankImage
                source: root.iniImagesUrl + "ir.png/rank_" + (index + 1)
            }
            Text {
                id: userNameText
                text: modelData.userName
                color: {
                    if (userEntry.isCurrentUser) {
                        return "#ff0066";
                    }
                    return "black";
                }
                font.pixelSize: 24
                elide: Text.ElideRight
                anchors.baseline: parent.bottom
                anchors.baselineOffset: -1
                fontSizeMode: Text.VerticalFit
                width: 132
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: userNameText.width - userNameText.implicitWidth
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    enabled: !(userEntry.isCurrentUser && !profile.onlineUserData)
                    onClicked: {
                        let url = ranking.profile.vars.generalVars.websiteBaseUrl + "/charts/" +
                            ranking.chartData.md5 + "/players/" + modelData.userId + "/scores"
                        if (ranking.provider === OnlineRankingModel.LR2IR && !userEntry.isCurrentUser) {
                            url = "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=mypage&playerid=" + modelData.userId;
                        } else if (ranking.provider === OnlineRankingModel.Tachi) {
                            url = "https://boku.tachi.ac/u/" + modelData.userName + "/games/bms/" + ranking.keymode
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
                source: root.iniImagesUrl + "parts.png/ranking_" + modelData.bestClearType
                property bool loading: false
                anchors.bottom: rankImage.bottom
                anchors.bottomMargin: -1
                MouseArea {
                    anchors.fill: parent
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    enabled: (userEntry.isCurrentUser && ranking.provider === OnlineRankingModel.LR2IR)
                        || ranking.provider === OnlineRankingModel.RhythmGame
                    onClicked: (event) => {
                        if (ranking.provider === OnlineRankingModel.LR2IR) {
                            if (ranking.bestPointsScore) {
                                onScoreLoaded(event, ranking.bestPointsScore);
                            }
                            return;
                        }
                        clearTypeImage.loading = true;
                        Rg.onlineScores.getScoreByGuid(
                            ranking.profile.vars.generalVars.webApiUrl,
                            modelData.bestClearTypeGuid).then(
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
                text: modelData.bestPoints
                font.pixelSize: 24
                color: "#ff0066"
                verticalAlignment: Text.AlignBottom
                horizontalAlignment: Text.AlignRight
                anchors.baseline: rankImage.bottom
                anchors.baselineOffset: -1
                width: 160 - clearTypeImage.width - 10
                property bool loading: false
                opacity: loading ? 0.5 : 1
                MouseArea {
                    anchors.fill: parent
                    anchors.leftMargin: pointsText.width - pointsText.implicitWidth
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    enabled: (userEntry.isCurrentUser && ranking.provider === OnlineRankingModel.LR2IR)
                        || ranking.provider === OnlineRankingModel.RhythmGame
                    onClicked: (event) => {
                        if (ranking.provider === OnlineRankingModel.LR2IR) {
                            if (ranking.bestPointsScore) {
                                onScoreLoaded(event, ranking.bestPointsScore);
                            }
                            return;
                        }
                        pointsText.loading = true;
                        Rg.onlineScores.getScoreByGuid(
                            ranking.profile.vars.generalVars.webApiUrl,
                            modelData.bestPointsGuid).then(
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
                text: (Math.floor(modelData.bestPoints / modelData.maxPoints * 1000) / 10).toFixed(1) + "%"
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