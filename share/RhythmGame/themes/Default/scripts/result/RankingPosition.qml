import QtQuick
import QtQuick.Controls.Basic
import "../common"

WindowBg {
    id: rankingPosition

    required property int oldRankingPosition
    required property int newRankingPosition
    required property int totalEntries
    required property bool loading
    required property bool scoreSubmissionFailed
    required property bool arenaSelected
    required property bool arenaFinalized
    required property bool arenaLocalDnf
    required property int arenaLocalRank
    required property int arenaParticipantCount

    ThemeFont {
        id: resultRankingFont
        fileName: root.themeVars.resultStatsFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    Image {
        id: rankingText

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        source: root.iniImagesUrl + "parts.png/ranking"
    }

    Loader {
        anchors.fill: parent
        sourceComponent: loading ? loadingIndicator : loaded
    }
    Component {
        id: loadingIndicator
        Item {
            anchors.fill: parent
            anchors.topMargin: rankingText.height + 32
            anchors.bottomMargin: 24
            BusyIndicator {
                anchors.centerIn: parent
                running: true
                height: parent.height
                width: height
            }
        }
    }
    Component {
        id: loaded
        Item {
            id: loadedRanking
            opacity: rankingPosition.arenaSelected
                ? 1 : (scoreSubmissionFailed ? 0.25 : 1)
            function paddedRanking(prefix, value) {
                let len = Math.max(value.toString().length, 4);
                let zeroes = "";
                for (let i = 0; i < len; i++) {
                    zeroes += "0";
                }
                return prefix + "<font color='lightgray'>" + zeroes.slice(0, Math.max(0, len - value.toString().length)) + "</font>" + value;
            }
            readonly property string oldRankingLabel: paddedRanking("#", rankingPosition.oldRankingPosition)
            readonly property string newRankingLabel: paddedRanking("#", rankingPosition.newRankingPosition)
            readonly property string totalEntriesLabel: paddedRanking("/", rankingPosition.totalEntries)

            Item {
                visible: !rankingPosition.arenaSelected
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin: 18
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 22
                height: 34
                readonly property real arrowGap: 7
                readonly property real totalGap: 4
                readonly property real arrowWidth: resultRankingArrow.implicitWidth
                readonly property real naturalOldWidth: oldRankingSizer.implicitWidth
                readonly property real naturalNewWidth: newRankingSizer.implicitWidth
                readonly property real naturalTotalWidth: totalEntriesSizer.implicitWidth
                readonly property real naturalTextWidth: naturalOldWidth + naturalNewWidth + naturalTotalWidth
                readonly property real fixedContentWidth: arrowWidth + arrowGap * 2 + totalGap
                readonly property real textAvailableWidth: Math.max(0, width - fixedContentWidth)
                readonly property real textScale: naturalTextWidth > 0 && textAvailableWidth > 0 ? Math.min(1, textAvailableWidth / naturalTextWidth) : 1
                readonly property real oldTextWidth: naturalOldWidth * textScale
                readonly property real newTextWidth: naturalNewWidth * textScale
                readonly property real totalTextWidth: naturalTotalWidth * textScale
                readonly property real contentWidth: oldTextWidth + newTextWidth + totalTextWidth + fixedContentWidth
                readonly property real contentLeft: Math.max(0, (width - contentWidth) / 2)
                readonly property real baselineY: height - 3

                ResultNumberText {
                    id: oldRankingSizer

                    visible: false
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 24
                    })
                    text: loadedRanking.oldRankingLabel
                    textFormat: Text.RichText
                }
                ResultNumberText {
                    id: newRankingSizer

                    visible: false
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 30
                    })
                    text: loadedRanking.newRankingLabel
                    textFormat: Text.RichText
                }
                ResultNumberText {
                    id: totalEntriesSizer

                    visible: false
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 24
                    })
                    text: loadedRanking.totalEntriesLabel
                    textFormat: Text.RichText
                }

                ResultNumberText {
                    id: oldRankingText

                    anchors.baseline: parent.top
                    anchors.baselineOffset: parent.baselineY
                    anchors.left: parent.left
                    anchors.leftMargin: parent.contentLeft
                    color: "black"
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 24 * parent.textScale
                    })
                    horizontalAlignment: Text.AlignRight
                    text: loadedRanking.oldRankingLabel
                    textFormat: Text.RichText
                    width: parent.oldTextWidth
                }
                Image {
                    id: resultRankingArrow

                    anchors.left: oldRankingText.right
                    anchors.leftMargin: parent.arrowGap
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2
                    source: root.iniImagesUrl + "parts.png/arrow"
                }
                ResultNumberText {
                    id: newRankingText

                    anchors.baseline: parent.top
                    anchors.baselineOffset: parent.baselineY
                    anchors.left: resultRankingArrow.right
                    anchors.leftMargin: parent.arrowGap
                    color: "black"
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 30 * parent.textScale
                    })
                    horizontalAlignment: Text.AlignRight
                    text: loadedRanking.newRankingLabel
                    textFormat: Text.RichText
                    width: parent.newTextWidth
                }
                ResultNumberText {
                    anchors.baseline: parent.top
                    anchors.baselineOffset: parent.baselineY
                    anchors.left: newRankingText.right
                    anchors.leftMargin: parent.totalGap
                    color: "black"
                    font: resultRankingFont.uiFont({
                        weight: resultRankingFont.fontWeight,
                        variableAxes: resultRankingFont.variableAxes,
                        italic: resultRankingFont.italic,
                        pixelSize: 24 * parent.textScale
                    })
                    horizontalAlignment: Text.AlignLeft
                    text: loadedRanking.totalEntriesLabel
                    textFormat: Text.RichText
                    width: parent.totalTextWidth
                }
            }

            Text {
                anchors.baseline: parent.top
                anchors.baselineOffset: parent.height - 25
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#20242c"
                font: resultRankingFont.uiFont({
                    weight: resultRankingFont.fontWeight,
                    variableAxes: resultRankingFont.variableAxes,
                    italic: resultRankingFont.italic,
                    pixelSize: 30
                })
                text: rankingPosition.arenaLocalDnf
                    || !rankingPosition.arenaFinalized
                    || rankingPosition.arenaLocalRank <= 0
                    ? qsTr("— / %1").arg(
                          rankingPosition.arenaParticipantCount)
                    : qsTr("#%1 / %2")
                          .arg(rankingPosition.arenaLocalRank)
                          .arg(rankingPosition.arenaParticipantCount)
                textFormat: Text.PlainText
                visible: rankingPosition.arenaSelected
            }
        }
    }
}

