pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQuick.Controls.Basic
import QtMultimedia
import QtQml
import "../common/helpers.js" as Helpers
import "./playOptions"

FocusScope {
    Image {
        id: root

        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)

        function openReplay(type) {
            let path = songList.current instanceof course ? songList.current : songList.current.path;
            let func = songList.current instanceof course ? globalRoot.openCourse : globalRoot.openChart;
            switch (type) {
                case 0:
                    func(path, Rg.profileList.mainProfile, false, songList.currentItem.scores[0], null, false, null);
                    break;
                case 1:
                    func(path, Rg.profileList.mainProfile, false, songList.currentItem.scoreWithBestPoints, null, false, null);
                    break;
                case 2:
                    let clearType = Helpers.getClearType(songList.currentItem?.scores);
                    let score = songList.currentItem.scores.find((score) => {
                        return score.result.clearType === clearType;
                    });
                    if (score) {
                        func(path, Rg.profileList.mainProfile, false, score, null, false, null);
                    }
                    break;
                case 3:
                    let bestScore = songList.currentItem.scores.reduce((prev, curr) => {
                        return prev.result.maxCombo > curr.result.maxCombo ? prev : curr;
                    });
                    if (bestScore) {
                        func(path, Rg.profileList.mainProfile, false, bestScore, null, false, null);
                    }
                    break;
            }
        }

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + "bg.png"
        width: parent.width

        onEnabledChanged: {
            if (enabled) {
                previewDelayTimer.restart();
                songList.refresh();
            } else {
                playMusic.stop();
                previewDelayTimer.stop();
                playMusic.waitingForStop = false;
            }
        }

        Item {
            anchors.centerIn: parent
            height: 1080
            scale: Math.min(parent.width / 1920, parent.height / 1080)
            width: 1920

            enabled: !playOptions.enabled && !login.enabled

            StageFile {
                id: stageFile
                anchors {
                    left: parent.left
                    top: parent.top
                    leftMargin: 80
                    topMargin: 120
                }
                height: 480
                width: 640
            }
            Banner {
                id: banner
                anchors {
                    left: stageFile.right
                    top: parent.top
                    leftMargin: 60
                    topMargin: 200
                }
                height: 80
                width: 300
            }
            Loader {
                id: courseSongs
                active: songList.current instanceof course
                anchors {
                    left: parent.left
                    right: parent.right
                    top: stageFile.top
                }
                source: "CourseSongs.qml"
            }
            ReplayAutoplay {
                z: 1
                anchors.horizontalCenter: banner.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -10
                spacing: 5
                current: songList.current
            }
            Selector {
                anchors.right: parent.right
                anchors.rightMargin: 80
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -50
                currentItem: songList.current
                scrollingText: songList.scrollingText
                z: 2
            }
            Image {
                anchors.left: parent.left
                anchors.leftMargin: -20
                anchors.top: parent.top
                height: 1080
                source: root.iniImagesUrl + "parts.png/containers"
                width: 1920
            }
            ScrollBar {
                id: vbar

                active: true
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 152
                anchors.right: parent.right
                anchors.rightMargin: 25
                anchors.top: parent.top
                anchors.topMargin: 152
                hoverEnabled: true
                orientation: Qt.Vertical
                onPositionChanged: {
                    if (pressed) {
                        previewDelayTimer.restart();
                        songList.offset = songList.count * (1 - vbar.position);
                    }
                }
                snapMode: ScrollBar.SnapOnRelease
                stepSize: 1 / songList.count
                background: Item {
                }
                contentItem: Image {
                    source: root.iniImagesUrl + "parts.png/slider"
                }
                Binding {
                    vbar.position: {
                        let pos = 1 - (songList.offset / songList.count);
                        // some trickery to get the scrollbar to appear at the top at the beginning
                        return pos === 1 ? 0 : pos;
                    }
                }
            }
            Image {
                id: stageFileFrame

                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -22
                source: root.imagesUrl + "stageFileFrame.png"
            }
            List {
                id: songList

                anchors.right: parent.right
                anchors.rightMargin: 43
                anchors.top: parent.top
                anchors.topMargin: -40
                focus: true
                height: parent.height
                width: parent.width / 2
            }
            Shortcut {
                sequence: "Esc"
                enabled: root.enabled

                onActivated: {
                    sceneStack.pop();
                }
            }
            AudioPlayer {
                id: playMusic

                property bool waitingForStop: false

                looping: true
                source: songList.current instanceof ChartData ? songList.previewFiles[songList.current.chartDirectory] : undefined

                onSourceChanged: {
                    playMusic.stop();
                    previewDelayTimer.stop();
                    waitingForStop = playMusic.source !== "";
                }
            }
            Difficulty {
                anchors.left: parent.left
                anchors.leftMargin: 80
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 160
                current: songList.current
                spacing: 30
            }
            Loader {
                id: grade

                active: songList.currentItem
                ?.
                    scoreWithBestPoints || false
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -10
                anchors.verticalCenterOffset: 150
                sourceComponent: Grade {
                    scoreWithBestPoints: songList.currentItem
                    ?.
                        scoreWithBestPoints || null
                }
            }
            Connections {
                function onMovingInAnyWayChanged() {
                    if (playMusic.waitingForStop) {
                        previewDelayTimer.restart();
                    }
                }

                function onOpenedFolder() {
                    previewDelayTimer.restart()
                }

                target: songList
            }
            Timer {
                id: previewDelayTimer

                interval: 300

                onTriggered: {
                    playMusic.play();
                }
            }
            ScoreInfo {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 80
                anchors.left: parent.left
                anchors.leftMargin: 80
                spacing: 40

                current: songList.current
                scoreWithBestPoints: songList.currentItem
                ?.
                    scoreWithBestPoints || null
                bestStats: songList.currentItem
                ?.
                    bestStats || null
            }
            Image {
                id: search

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5
                anchors.left: parent.left
                anchors.leftMargin: 48
                source: root.iniImagesUrl + "parts.png/search"

                TextEdit {
                    id: searchEdit

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 17
                    anchors.left: parent.left
                    anchors.leftMargin: 48
                    color: "white"
                    font.pixelSize: 20
                    height: 25
                    width: 540

                    Keys.onReturnPressed: {
                        songList.search(searchEdit.text);
                        searchEdit.text = "";
                        songList.focus = true;
                    }
                }
            }
            KeymodeButton {
                id: keymodeButton

                anchors.left: parent.left
                anchors.leftMargin: 900
                anchors.top: parent.top
                anchors.topMargin: 20
            }
            SortButton {
                anchors.left: keymodeButton.left
                anchors.top: keymodeButton.bottom
                anchors.topMargin: 20
            }
            MouseArea {
                id: focusList

                anchors.fill: parent
                z: -1

                onClicked: {
                    songList.focus = true;
                }
            }
        }


        Rectangle {
            anchors.fill: parent

            color: {
                let c = Qt.color("black");
                c.a = 0.5;
                return c;
            }

            MouseArea {
                anchors.fill: parent
                enabled: parent.visible
                onClicked: (event) => {
                    login.enabled = false;
                }
                onWheel: (wheel) => {
                    wheel.accepted = true;
                }
            }

            visible: playOptions.enabled || login.enabled

            Item {
                id: options
                anchors.centerIn: parent
                height: 1080
                scale: Math.min(parent.width / 1920, parent.height / 1080)
                width: 1920

                property bool startPressed: Input.start1 || Input.start2

                Loader {
                    id: playOptions
                    active: enabled
                    enabled: options.startPressed && !login.enabled
                    anchors.centerIn: parent

                    source: Rg.profileList.battleActive ? "playOptions/PlayOptionsBattle.qml" : "playOptions/PlayOptionsSingle.qml"
                }

                Login {
                    id: login

                    anchors.centerIn: parent
                    enabled: false
                }
                Timer {
                    id: p1StartTimer
                    interval: 500
                }
                property bool start1Pressed: Input.start1
                onStart1PressedChanged: {
                    if (start1Pressed) {
                        if (p1StartTimer.running && !login.enabled || login.enabled) {
                            login.enabled = !login.enabled;
                        } else {
                            p1StartTimer.restart();
                        }
                    }
                }
                Timer {
                    id: p2StartTimer
                    interval: 500
                }
                property bool start2Pressed: Input.start2
                onStart2PressedChanged: {
                    if (start2Pressed) {
                        if (p2StartTimer.running && !login.enabled || login.enabled) {
                            login.enabled = !login.enabled;
                        } else {
                            p2StartTimer.restart();
                        }
                    }
                }
            }
        }
    }
}