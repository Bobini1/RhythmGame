import QtQuick
import QtQuick.Controls

WindowBg {
    id: rankingPosition

    required property int oldRankingPosition
    required property int newRankingPosition
    required property int totalEntries
    required property bool loading
    required property bool scoreSubmissionFailed

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
            anchors.topMargin: rankingText.height + 48
            BusyIndicator {
                anchors.centerIn: parent
                running: true
            }
        }
    }
    Component {
        id: loaded
        Item {
            opacity: scoreSubmissionFailed ? 0.25 : 1
            Text {
                id: oldRanking

                anchors.baseline: parent.bottom
                anchors.baselineOffset: -25
                anchors.left: parent.left
                anchors.leftMargin: 30
                font.pixelSize: 24

                text: {
                    let len = Math.max(rankingPosition.oldRankingPosition.toString().length, 4);
                    let zeroes = "";
                    for (let i = 0; i < len; i++) {
                        zeroes += "0";
                    }
                    return "#" + "<font color='lightgray'>" + zeroes.slice(0, Math.max(0, len - rankingPosition.oldRankingPosition.toString().length)) + "</font>" + rankingPosition.oldRankingPosition;
                }
            }
            Image {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 24
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: -36
                source: root.iniImagesUrl + "parts.png/arrow"
            }
            Text {
                id: newRanking

                anchors.baseline: parent.bottom
                anchors.baselineOffset: -25
                anchors.right: parent.right
                anchors.rightMargin: 90
                font.pixelSize: 30

                text: {
                    let len = Math.max(rankingPosition.newRankingPosition.toString().length, 4);
                    let zeroes = "";
                    for (let i = 0; i < len; i++) {
                        zeroes += "0";
                    }
                    return "#" + "<font color='lightgray'>" + zeroes.slice(0, Math.max(0, len - rankingPosition.newRankingPosition.toString().length)) + "</font>" + rankingPosition.newRankingPosition;
                }
            }
            Text {
                id: totalEntries

                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.baseline: parent.bottom
                anchors.baselineOffset: -25
                font.pixelSize: 24

                text: {
                    let len = Math.max(rankingPosition.totalEntries.toString().length, 4);
                    let zeroes = "";
                    for (let i = 0; i < len; i++) {
                        zeroes += "0";
                    }
                    return "/" + "<font color='lightgray'>" + zeroes.slice(0, Math.max(0, len - rankingPosition.totalEntries.toString().length)) + "</font>" + rankingPosition.totalEntries;
                }
            }
        }
    }
}

