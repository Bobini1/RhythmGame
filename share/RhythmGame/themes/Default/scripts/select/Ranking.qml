import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls

Image {
    id: ranking

    required property var md5
    required property var path
    visible: !!md5
    property int page: 0
    property var profile: Rg.profileList.mainProfile

    OnlineRankingModel {
        id: rankingModel
        md5: ranking.md5
        limit: 7
        sortBy: OnlineRankingModel.ScorePct
        sortDir: OnlineRankingModel.Desc
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
        running: rankingModel ? rankingModel.loading : false
        visible: running
    }

    Loader {
        anchors.fill: parent
        active: !!ranking.md5 && !rankingModel.loading
        sourceComponent: ranking.page === 0 ? rankingDelegate : statsDelegate
    }

    component StatLineItem: Item {
        id: statLineItem
        property alias labelSource: label.source
        property alias text1: textOne.text
        property alias text2: textTwo.text
        height: label.height
        width: 188
        Image {
            id: label
        }
        Text {
            id: textOne
            anchors.right: textTwo.left
            anchors.rightMargin: 5
            anchors.baseline: label.bottom
            font.pixelSize: 21
            anchors.baselineOffset: -1
        }
        Text {
            id: textTwo
            font.pixelSize: 12
            width: 37
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignBottom
            anchors.right: parent.right
            anchors.baseline: label.bottom
            font.bold: true
            anchors.baselineOffset: -1
        }
    }

    Component {
        id: statsDelegate

        RowLayout {
            anchors.fill: parent
            anchors.margins: 28
            anchors.topMargin: 54
            spacing: 10
            Column {
                spacing: 7

                Repeater {
                    model: ["MAX", "PERFECT", "FC", "EXHARD", "HARD", "NORMAL", "EASY"]
                    delegate: StatLineItem {
                        labelSource: root.iniImagesUrl + "parts.png/ranking_" + modelData
                        text1: rankingModel.clearCounts[modelData] ?? 0
                        text2: rankingModel.playerCount > 0 ? ((rankingModel.clearCounts[modelData] ?? 0) / rankingModel.playerCount * 100).toFixed(1) + "%" : "0.0%"
                    }
                }
            }
            Column {
                spacing: 7

                StatLineItem {
                    id: playerCountItem
                    labelSource: root.iniImagesUrl + "parts.png/ranking_PLAYER"
                    text1: rankingModel.playerCount
                }
                StatLineItem {
                    labelSource: root.iniImagesUrl + "parts.png/ranking_FULLCOMBO"
                    readonly property int count: (rankingModel.clearCounts["FC"] ?? 0) + (rankingModel.clearCounts["PERFECT"] ?? 0) + (rankingModel.clearCounts["MAX"] ?? 0)
                    text1: count
                    text2: rankingModel.playerCount > 0 ? (count / rankingModel.playerCount * 100).toFixed(1) + "%" : "0.0%"
                }
                StatLineItem {
                    labelSource: root.iniImagesUrl + "parts.png/ranking_CLEAR"
                    readonly property int count: {
                        let total = 0;
                        for (let key in rankingModel.clearCounts) {
                            if (key !== "NOPLAY" && key !== "FAILED") {
                                total += rankingModel.clearCounts[key] ?? 0;
                            }
                        }
                        return total;
                    }
                    text1: count
                    text2: rankingModel.playerCount > 0 ? (count / rankingModel.playerCount * 100).toFixed(1) + "%" : "0.0%"
                }
                StatLineItem {
                    height: playerCountItem.height
                }
                Repeater {
                    model: ["AEASY", "FAILED", "NOPLAY"]
                    delegate: StatLineItem {
                        labelSource: root.iniImagesUrl + "parts.png/ranking_" + modelData
                        text1: rankingModel.clearCounts[modelData] ?? 0
                        text2: rankingModel.playerCount > 0 ? ((rankingModel.clearCounts[modelData] ?? 0) / rankingModel.playerCount * 100).toFixed(1) + "%" : "0.0%"
                    }
                }
            }
        }
    }

    Component {
        id: rankingDelegate
        Column {
            anchors.fill: parent
            anchors.margins: 28
            anchors.topMargin: 54
            spacing: 6

            Repeater {
                model: rankingModel
                delegate: Row {
                    id: rankingEntry
                    spacing: 10
                    required property int rank
                    required property var userName
                    required property var bestClearType
                    required property var bestPoints
                    required property var maxPoints
                    required property var userId
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
                        font.pixelSize: 24
                        elide: Text.ElideRight
                        anchors.baseline: parent.bottom
                        anchors.baselineOffset: -1
                        fontSizeMode: Text.VerticalFit
                        width: 132
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally(
                                Rg.onlineLinks.scoresByUserOnChart(
                                    ranking.profile.vars.generalVars.websiteBaseUrl,
                                    ranking.md5,
                                    rankingEntry.userId))
                        }
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
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                clearTypeImage.loading = true;
                                Rg.onlineScores.getScoreByGuid(
                                    ranking.profile.vars.generalVars.webApiUrl,
                                    rankingEntry.bestClearTypeGuid).then(
                                    (score) => {
                                        clearTypeImage.loading = false;
                                        globalRoot.openChart(ranking.path, Rg.profileList.mainProfile, false, score, null, false, null);
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
                        elide: Text.ElideMiddle
                        property bool loading: false
                        opacity: loading ? 0.5 : 1
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                pointsText.loading = true;
                                Rg.onlineScores.getScoreByGuid(
                                    ranking.profile.vars.generalVars.webApiUrl,
                                    rankingEntry.bestPointsGuid).then(
                                    (score) => {
                                        pointsText.loading = false;
                                        globalRoot.openChart(ranking.path, Rg.profileList.mainProfile, false, score, null, false, null);
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
    }
}