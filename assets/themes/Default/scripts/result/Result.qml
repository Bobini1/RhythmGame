import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia
import QtQml
import "../common/helpers.js" as Helpers

FocusScope {
    focus: StackView.status === StackView.Active

    Image {
        id: root

        readonly property bool active: parent.focus
        readonly property var earlyLate: Helpers.getEarlyLate(result.replayData, result.result.judgementCounts[Judgement.Poor])
        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        readonly property var oldBestClearScore: Helpers.getClearType(scores)
        readonly property var oldBestPointsScore: Helpers.getScoreWithBestPoints(scores)
        readonly property var oldBestStats: Helpers.getBestStats(scores)
        property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
        readonly property var scores: {
            let scores = ScoreDb.getScoresForChart(chartData.sha256);
            // remove the score with the same id
            scores = scores.filter(function (score) {
                    return score.id !== result.id;
                });
            return scores;
        }

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (result.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: resultFocusScope.active
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
                sceneStack.pop();
            }
        }
        Item {
            id: scaledRoot

            height: 1080
            scale: Math.min(parent.width / width, parent.height / height)
            width: 1920

            Column {
                width: parent.width

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        id: stageFile

                        asynchronous: true
                        height: sourceSize.height
                        source: {
                            let stageFile = "file://" + chartData.directory + chartData.stageFile;
                            return FileValidator.exists(stageFile) ? stageFile : "";
                        }
                        sourceSize.height: 192
                        sourceSize.width: 256
                        width: sourceSize.width

                        Rectangle {
                            anchors.fill: parent
                            color: "black"
                            opacity: 0.5
                            z: -1
                        }
                    }
                    WindowBg {
                        id: title

                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: 124
                        width: 1214

                        Text {
                            id: titleText

                            anchors.bottom: artistText.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: 40
                            text: chartData.title + (chartData.subtitle ? (" " + chartData.subtitle) : "")
                        }
                        Text {
                            id: artistText

                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 18
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: 30
                            text: chartData.artist + (chartData.subartist ? (" " + chartData.subartist) : "")
                        }
                    }
                    WindowBg {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 24
                        height: title.height
                        width: 280

                        Image {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            source: {
                                switch (chartData.difficulty) {
                                case 1:
                                    return root.iniImagesUrl + "parts.png/beginner";
                                case 2:
                                    return root.iniImagesUrl + "parts.png/normal";
                                case 3:
                                    return root.iniImagesUrl + "parts.png/hyper";
                                case 4:
                                    return root.iniImagesUrl + "parts.png/another";
                                case 5:
                                    return root.iniImagesUrl + "parts.png/insane";
                                default:
                                    return root.iniImagesUrl + "parts.png/unknown";
                                }
                            }
                        }
                        Text {
                            id: totalLabel

                            anchors.baseline: parent.top
                            anchors.baselineOffset: 60
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.horizontalCenterOffset: -60
                            color: "white"
                            font.pixelSize: 20
                            text: qsTr("TOTAL")

                            Rectangle {
                                anchors.centerIn: parent
                                color: "#4b4b4b"
                                height: 20
                                radius: 10
                                width: 90
                                z: -1
                            }
                        }
                        Text {
                            anchors.baseline: totalLabel.baseline
                            anchors.right: parent.right
                            anchors.rightMargin: 36
                            font.pixelSize: 20
                            text: chartData.total
                        }
                        Text {
                            id: noteCountLabel

                            anchors.baseline: parent.top
                            anchors.baselineOffset: 90
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.horizontalCenterOffset: -60
                            color: "white"
                            font.pixelSize: 20
                            text: qsTr("NOTES")

                            Rectangle {
                                anchors.centerIn: parent
                                color: "#4b4b4b"
                                height: 20
                                radius: 10
                                width: 90
                                z: -1
                            }
                        }
                        Text {
                            anchors.baseline: noteCountLabel.baseline
                            anchors.right: parent.right
                            anchors.rightMargin: 36
                            font.pixelSize: 20
                            text: {
                                let counts = result.result.judgementCounts;
                                return counts[Judgement.Perfect] + counts[Judgement.Great] + counts[Judgement.Good] + counts[Judgement.Bad] + counts[Judgement.Poor];
                            }
                        }
                    }
                }
                Item {
                    height: childrenRect.height
                    width: parent.width

                    Image {
                        id: lifeGraph

                        anchors.right: parent.right
                        anchors.rightMargin: 90
                        anchors.top: scoreColumn.top
                        source: root.iniImagesUrl + "parts.png/life_graph_frame"
                    }
                    Column {
                        id: scoreColumn

                        WindowBg {
                            id: score

                            height: 150
                            width: 668

                            Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 36
                                anchors.verticalCenter: parent.verticalCenter

                                Column {
                                    spacing: 20
                                    width: 460

                                    Item {
                                        height: exScoreImg.sourceSize.height
                                        width: parent.width

                                        Image {
                                            id: exScoreImg

                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            source: root.iniImagesUrl + "parts.png/ex_score"
                                        }
                                        Text {
                                            id: exScoreText

                                            anchors.baseline: parent.bottom
                                            anchors.right: parent.right
                                            anchors.rightMargin: 120
                                            color: "lightgray"
                                            font.pixelSize: 41
                                            horizontalAlignment: Text.AlignRight
                                            text: {
                                                let points = result.result.points;
                                                return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                                            }
                                        }
                                        Text {
                                            id: scoreRate

                                            anchors.baseline: parent.bottom
                                            anchors.leftMargin: 30
                                            anchors.right: parent.right
                                            font.pixelSize: 25
                                            horizontalAlignment: Text.AlignRight
                                            text: (result.result.maxPoints ? (result.result.points / result.result.maxPoints * 100).toFixed(2) : "0.00") + "%"
                                        }
                                    }
                                    Item {
                                        height: hiScoreImg.sourceSize.height
                                        width: parent.width

                                        Image {
                                            id: hiScoreImg

                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            source: root.iniImagesUrl + "parts.png/hi_score"
                                        }
                                        Text {
                                            id: hiScoreText

                                            anchors.baseline: parent.bottom
                                            anchors.right: parent.right
                                            anchors.rightMargin: 120
                                            color: "lightgray"
                                            font.pixelSize: 34
                                            horizontalAlignment: Text.AlignRight
                                            text: {
                                                let points = root.oldBestPointsScore ? root.oldBestPointsScore.points : 0;
                                                return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                                            }
                                        }
                                        Text {
                                            id: hiScoreDelta

                                            anchors.baseline: parent.bottom
                                            anchors.left: hiScoreText.right
                                            anchors.leftMargin: 41
                                            color: {
                                                let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                                                return delta > 0 ? "darkgreen" : (delta < 0 ? "FireBrick" : "black");
                                            }
                                            font.pixelSize: 25
                                            horizontalAlignment: Text.AlignLeft
                                            text: {
                                                let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                                                if (delta > 0) {
                                                    return "+" + delta;
                                                } else if (delta < 0) {
                                                    return "–" + (-delta);
                                                } else {
                                                    return "";
                                                }
                                            }
                                        }
                                    }
                                }
                                Image {
                                    id: gradeImage

                                    anchors.verticalCenter: parent.verticalCenter
                                    source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(result.result.points, result.result.maxPoints)
                                }
                            }
                        }
                        WindowBg {
                            id: comboInfo

                            height: 180
                            width: 668

                            Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 36
                                anchors.verticalCenter: parent.verticalCenter

                                Column {
                                    spacing: 20
                                    width: 460

                                    StatLine {
                                        img: root.iniImagesUrl + "parts.png/miss_count"
                                        invertDeltaColor: true
                                        newVal: result.result.judgementCounts[Judgement.Poor] + result.result.judgementCounts[Judgement.EmptyPoor] + result.result.judgementCounts[Judgement.Bad]
                                        oldVal: root.oldBestStats ? root.oldBestStats.missCount : 0
                                        width: parent.width
                                    }
                                    StatLine {
                                        img: root.iniImagesUrl + "parts.png/combo"
                                        newVal: result.result.maxCombo
                                        oldVal: root.oldBestStats ? root.oldBestStats.maxCombo : 0
                                        width: parent.width
                                    }
                                    StatLine {
                                        img: root.iniImagesUrl + "parts.png/combo_break"
                                        invertDeltaColor: true
                                        newVal: result.result.judgementCounts[Judgement.Poor] + result.result.judgementCounts[Judgement.Bad]
                                        oldVal: root.oldBestStats ? root.oldBestStats.comboBreak : 0
                                        width: parent.width
                                    }
                                }
                            }
                        }
                        WindowBg {
                            id: hitStats

                            height: 364
                            width: 668

                            Item {
                                id: hitStatHeader

                                anchors.left: parent.left
                                anchors.leftMargin: 450
                                anchors.top: parent.top
                                anchors.topMargin: 36
                                height: childrenRect.height

                                Image {
                                    id: early

                                    anchors.left: parent.left
                                    source: root.iniImagesUrl + "parts.png/early"
                                }
                                Image {
                                    id: late

                                    anchors.left: early.right
                                    anchors.leftMargin: -155
                                    source: root.iniImagesUrl + "parts.png/late"
                                }
                            }
                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: hitStatHeader.bottom
                                spacing: 18

                                Repeater {
                                    model: [Judgement.Perfect, Judgement.Great, Judgement.Good, Judgement.Bad, Judgement.Poor, Judgement.EmptyPoor]

                                    HitStatLine {
                                        earlyCount: root.earlyLate.early[modelData]
                                        img: {
                                            switch (modelData) {
                                            case Judgement.Perfect:
                                                return root.iniImagesUrl + "parts.png/perfect";
                                            case Judgement.Great:
                                                return root.iniImagesUrl + "parts.png/great";
                                            case Judgement.Good:
                                                return root.iniImagesUrl + "parts.png/good";
                                            case Judgement.Bad:
                                                return root.iniImagesUrl + "parts.png/bad";
                                            case Judgement.Poor:
                                                return root.iniImagesUrl + "parts.png/poor";
                                            case Judgement.EmptyPoor:
                                                return root.iniImagesUrl + "parts.png/empty_poor";
                                            }
                                        }
                                        judgementCount: result.result.judgementCounts[modelData]
                                        lateCount: root.earlyLate.late[modelData]
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}