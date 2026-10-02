pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls
import "../common"

Image {
    id: ranking

    required property StandardRanking rankingState
    readonly property var chartData: rankingState.chartData
    readonly property string md5: rankingState.md5
    visible: rankingState.available
    property int page: 0
    readonly property var profile: rankingState.profile
    readonly property int provider: rankingState.provider
    readonly property bool loading: rankingState.loading
    readonly property var bestPointsResult: rankingState.bestPointsResult
    readonly property var bestClearTypeResult: rankingState.bestClearTypeResult
    readonly property var entries: rankingState.entries
    readonly property int playerCount: rankingState.playerCount
    readonly property var clearCounts: rankingState.clearCounts
    readonly property string tachiGameId: rankingState.tachiGameId
    readonly property int tabTitleInset: 32
    readonly property int tabTitleWidth: rankingBack.width - tabTitleInset * 2

    ThemeFont {
        id: rankingFont
        fileName: root.themeVars.rankingFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
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
        clip: true
        elide: Text.ElideRight
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: rankingFont.uiFont({
            weight: rankingFont.fontWeight,
            variableAxes: rankingFont.variableAxes,
            italic: rankingFont.italic,
            pixelSize: 16
        })
        maximumLineCount: 1
        minimumPixelSize: 5
        wrapMode: Text.NoWrap
        width: ranking.tabTitleWidth
        x: 42 + ranking.tabTitleInset
        anchors.top: rankingBack.top
        anchors.topMargin: 15
        anchors.bottom: rankingBack.bottom
    }

    Text {
        text: qsTr("Stats")
        clip: true
        elide: Text.ElideRight
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: rankingFont.uiFont({
            weight: rankingFont.fontWeight,
            variableAxes: rankingFont.variableAxes,
            italic: rankingFont.italic,
            pixelSize: 16
        })
        maximumLineCount: 1
        minimumPixelSize: 5
        wrapMode: Text.NoWrap
        width: ranking.tabTitleWidth
        x: parent.width - rankingBack.width - 42 + ranking.tabTitleInset
        anchors.top: rankingBack.top
        anchors.topMargin: 15
        anchors.bottom: rankingBack.bottom
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: (ranking.loading && ranking.page === 1) ||
            (ranking.loading && !ranking.entries.length) ||
            (ranking.loading && ranking.entries.length === 1 && ranking.provider === OnlineRankingModel.LR2IR)
        visible: running
    }

    Loader {
        anchors.fill: parent
        sourceComponent: rankingPages
        active: !!ranking.md5 && ranking.entries.length && !(ranking.loading && ranking.entries.length === 1 && ranking.provider === OnlineRankingModel.LR2IR)
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
                provider: ranking.provider
                entries: {
                    let entries = ranking.entries.slice(0, 7)
                    if (ranking.loading && entries.length === 1 && ranking.provider === OnlineRankingModel.LR2IR && !(entries[0] instanceof rankingEntry)) {
                        return [];
                    }
                    return entries;
                }
                bestPointsResult: ranking.bestPointsResult
                bestClearTypeResult: ranking.bestClearTypeResult
                tachiGameId: ranking.tachiGameId
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
