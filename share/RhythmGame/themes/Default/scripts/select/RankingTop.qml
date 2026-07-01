pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import "../common"

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
    property string tachiGameId

    ThemeFont {
        id: rankingTopFont
        fileName: root.themeVars.rankingFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    Repeater {
        id: rankingRepeater
        model: ranking.entries
        delegate: Row {
            id: userEntry
            spacing: 8
            anchors.left: parent.left
            anchors.right: parent.right
            height: rankImage.height
            readonly property real percentLaneWidth: 58
            readonly property real nameLaneWidth: Math.min(132, Math.max(72, width * 0.34))
            readonly property real pointsLaneWidth: Math.max(
                36,
                width - rankImage.width - nameLaneWidth - clearTypeImage.width
                    - percentLaneWidth - spacing * 4)
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
            Item {
                width: userEntry.nameLaneWidth
                height: userEntry.height
                Text {
                    id: userNameText
                    width: parent.width
                    height: parent.height
                    text: modelData.userName
                    color: {
                        if (userEntry.isCurrentUser) {
                            return "#ff0066";
                        }
                        return "black";
                    }
                    font.pixelSize: 24
                    font.family: rankingTopFont.fontFamily
                    font.weight: rankingTopFont.fontWeight
                    font.variableAxes: rankingTopFont.variableAxes
                    font.italic: rankingTopFont.italic
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 8
                }
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: Math.max(0, userNameText.width - userNameText.implicitWidth)
                    cursorShape: enabled ? Qt.PointingHandCursor : undefined
                    enabled: !(userEntry.isCurrentUser && !profile.onlineUserData)
                    onClicked: {
                        let url = ranking.profile.vars.generalVars.websiteBaseUrl + "/charts/" +
                            ranking.chartData.md5 + "/players/" + modelData.userId + "/scores"
                        if (ranking.provider === OnlineRankingModel.LR2IR && !userEntry.isCurrentUser) {
                            url = "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=mypage&playerid=" + modelData.userId;
                        } else if (ranking.provider === OnlineRankingModel.Tachi) {
                            url = "https://boku.tachi.ac/u/" + encodeURIComponent(modelData.userName) +
                                "/games/" + ranking.tachiGameId
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
            Item {
                width: clearTypeImage.implicitWidth
                height: userEntry.height
                Image {
                    id: clearTypeImage
                    source: root.iniImagesUrl + "parts.png/ranking_" + modelData.bestClearType
                    property bool loading: false
                    anchors.verticalCenter: parent.verticalCenter
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
            }

            Item {
                width: userEntry.pointsLaneWidth
                height: userEntry.height
                Text {
                    id: pointsText
                    width: parent.width
                    height: parent.height
                    text: modelData.bestPoints
                    font.pixelSize: 24
                    font.family: rankingTopFont.fontFamily
                    font.weight: rankingTopFont.fontWeight
                    font.variableAxes: rankingTopFont.variableAxes
                    font.italic: rankingTopFont.italic
                    color: "#ff0066"
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 8
                    property bool loading: false
                    opacity: loading ? 0.5 : 1
                }
                MouseArea {
                    anchors.fill: parent
                    anchors.leftMargin: Math.max(0, pointsText.width - pointsText.implicitWidth)
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
            Item {
                width: userEntry.percentLaneWidth
                height: userEntry.height
                Text {
                    id: percentageText
                    anchors.fill: parent
                    text: (Math.floor(modelData.bestPoints / modelData.maxPoints * 1000) / 10).toFixed(1) + "%"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.family: rankingTopFont.fontFamily
                    font.weight: rankingTopFont.boldFontWeight
                    font.variableAxes: rankingTopFont.boldVariableAxes
                    font.italic: rankingTopFont.italic
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 7
                }
            }
        }
    }
}
