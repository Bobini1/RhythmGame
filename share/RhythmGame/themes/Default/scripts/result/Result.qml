import RhythmGameQml
import QtQuick
import QtQuick.Shapes
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

Item {
    id: root

    // course
    property var course
    property list<ChartData> chartDatas: []
    // single chart
    property ChartData chartData

    required property var scores
    required property list<Profile> profiles

    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    readonly property var score1: scores[0]
    readonly property var score2: scores[1] || null
    readonly property Profile profile1: profiles[0]
    readonly property Profile profile2: profiles[1] || null
    readonly property bool isBattle: score1 && score2

    Timer {
        interval: 100
        running: true
        repeat: false
        onTriggered: {
            print(root.enabled)
        }
    }
    Input.onButtonPressed: (key) => {
        if ([BmsKey.Col11, BmsKey.Col12, BmsKey.Col13, BmsKey.Col14, BmsKey.Col15, BmsKey.Col16, BmsKey.Col17,
            BmsKey.Col21, BmsKey.Col22, BmsKey.Col23, BmsKey.Col24, BmsKey.Col25, BmsKey.Col26, BmsKey.Col27].includes(key)) {
            sceneStack.pop();
        }
    }

    Image {
        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (root.score1.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        Shortcut {
            enabled: root.enabled
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
            }
        }
        Shortcut {
            enabled: root.enabled
            sequence: "Return"

            onActivated: {
                sceneStack.pop();
            }
        }
        Input.onStart1Pressed: () => {
            sceneStack.pop();
        }
        Input.onStart2Pressed: () => {
            sceneStack.pop();
        }
        Item {
            id: scaledRoot

            height: 1080
            scale: Math.min(parent.width / width, parent.height / height)
            width: 1920
            anchors.centerIn: parent


            Row {
                id: chartInfoRow
                anchors.horizontalCenter: parent.horizontalCenter

                StageFile {
                    chartDirectory: chartData?.chartDirectory || ""
                    stageFileName: chartData?.stageFile || ""
                }
                TitleArtist {
                    id: titleArtist

                    title: chartData?.title || course?.name || ""
                    artist: root.chartData?.artist || ""
                    subtitle: root.chartData?.subtitle || ""
                    subartist: root.chartData?.subartist || ""

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: 124
                    width: 1214
                }
                ChartInfo {
                    difficulty: root.chartData?.difficulty
                    total: root.chartData?.total
                    noteCount: root.score1.result.normalNoteCount + root.score1.result.lnCount

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: titleArtist.height
                    width: 280
                }
            }

            Side {
                score: root.score1
                isBattle: root.isBattle
                profile: root.profile1
                width: parent.width
                anchors.top: chartInfoRow.bottom
            }

            Loader {
                active: root.isBattle
                width: parent.width
                anchors.top: chartInfoRow.bottom
                sourceComponent: Side {
                    score: root.score2
                    isBattle: root.isBattle
                    profile: root.profile2
                    mirrored: true
                }
            }

            CourseSongList {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                width: parent.width
                chartDatas: root.chartDatas
            }

        }
    }
}