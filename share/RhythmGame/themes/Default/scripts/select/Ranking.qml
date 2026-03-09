import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls

Image {
    id: ranking

    required property var md5
    property int page: 0

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
                    onLinkActivated: Qt.openUrlExternally(link)
                    width: 132
                }
                Image {
                    id: clearTypeImage
                    source: root.iniImagesUrl + "parts.png/ranking_" + rankingEntry.bestClearType
                    anchors.bottom: rankImage.bottom
                    anchors.bottomMargin: -1
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