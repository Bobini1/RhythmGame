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

    Component.onDestruction: {
        if (rankingModel) {
            rankingModel.cancelPending();
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
        running: rankingModel ? rankingModel.loading : false
        visible: running
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 42
        anchors.topMargin: 60
        spacing: 4

        Repeater {
            model: ranking.rankingModel
            delegate: RowLayout {
                id: rankingEntry
                spacing: 10
                required property var rank
                required property var userName
                required property var bestClearType
                required property var bestPoints

                Text {
                    text: "#" + rankingEntry.rank
                    font.pixelSize: 14
                    Layout.preferredWidth: 30
                }
                Text {
                    text: rankingEntry.userName
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }
                Text {
                    text: rankingEntry.bestClearType
                    font.pixelSize: 14
                }
                Text {
                    text: rankingEntry.bestPoints
                    font.pixelSize: 14
                }
            }
        }
    }
}