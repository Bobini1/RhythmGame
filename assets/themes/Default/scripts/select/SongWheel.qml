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
            print(path)
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
                sourceComponent: Column {
                    id: courseSongsColumn
                    property var chartDatas: Rg.chartLoader.loadChartDataFromDb(songList.current.md5s)
                    property var canPlay: songList.current.md5s.every(md5 => chartDatas[md5.toUpperCase()] !== undefined)
                    Repeater {
                        model: {
                            let md5s = songList.current.md5s;
                            let chartDatas = courseSongsColumn.chartDatas;
                            let names = []
                            for (let md5 of md5s) {
                                let info = Rg.tables.search(md5);
                                let chartData = chartDatas[md5.toUpperCase()];
                                let red = (chartData === undefined);
                                if (info.length) {
                                    names.push({red, text: info[0].symbol + info[0].levelName + " " + info[0].entry.title + (info[0].entry.subtitle ? " " + info[0].entry.subtitle : "")});
                                } else {
                                    if (chartData !== undefined) {
                                        names.push({red, text: chartData.title + (chartData.subtitle ? " " + chartData.subtitle : "")});
                                    } else {
                                        names.push({red, text: before + md5 + after});
                                    }
                                }
                            }
                            return names;
                        }
                        delegate: Image {
                            source: root.iniImagesUrl + "parts.png/course_chart_bar"
                            Image {
                                source: root.iniImagesUrl + "parts.png/" + (index+1) + "th"
                                anchors.top: parent.top
                                anchors.topMargin: 4
                            }
                            NameLabel {
                                anchors.left: parent.left
                                anchors.leftMargin: 192
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                anchors.top: parent.top
                                anchors.topMargin: 10
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 10
                                color: modelData.red ? "red" : "black"
                                font.pixelSize: 25
                                text: modelData.text
                                scrolling: songList.scrollingText
                            }
                        }
                    }
                }
            }
            Row {
                z: 1
                anchors.horizontalCenter: banner.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -10
                spacing: 5
                Image {
                    id: auto
                    source: root.iniImagesUrl + "parts.png/auto"
                    enabled: songList.current?.path || (songList.current instanceof course && courseSongs.item?.canPlay) || false
                    opacity: enabled ? 1 : 0.5
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: enabled ? Qt.PointingHandCursor : undefined
                        onClicked: {
                            let func = songList.current instanceof course ? globalRoot.openCourse : globalRoot.openChart;
                            let path = songList.current instanceof course ? songList.current : songList.current.path;
                            if (Rg.profileList.battleActive) {
                                func(path, Rg.profileList.battleProfiles.player1Profile, true, null, Rg.profileList.battleProfiles.player2Profile, true, null);
                            } else {
                                func(path, Rg.profileList.mainProfile, true, null, null, true, null);
                            }
                        }
                    }
                }
                Repeater {
                    model: 4
                    delegate: Image {
                        id: replay
                        source: root.iniImagesUrl + "parts.png/replay"
                        opacity: enabled ? 1 : 0.5
                        enabled: ((songList.current?.path || (songList.current instanceof course && courseSongs.item?.canPlay)) && songList.currentItem?.scores?.length) || false
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: enabled ? Qt.PointingHandCursor : undefined
                            hoverEnabled: true
                            ToolTip.visible: containsMouse
                            ToolTip.text: {
                                switch (modelData) {
                                    case 0:
                                        return qsTr("NEWEST");
                                    case 1:
                                        return qsTr("BEST SCORE");
                                    case 2:
                                        return qsTr("BEST CLEAR");
                                    case 3:
                                        return qsTr("BEST COMBO");
                                }
                            }
                            ToolTip.delay: 500
                            onClicked: {
                                root.openReplay(modelData);
                            }
                        }
                    }
                }
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
            MediaPlayer {
                id: playMusic

                property bool waitingForStop: false

                loops: MediaPlayer.Infinite
                source: {
                    let base = songList.current instanceof ChartData ? Rg.previewFilePathFetcher.getPreviewFilePath(songList.current.chartDirectory) : ""
                    if (base === "") {
                        return base;
                    }
                    if (base[0] !== '/') {
                        base = '/' + base;
                    }
                    return "file://" + base;
                }

                audioOutput: AudioOutput {
                    id: audioOutput

                }

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

                active: root.scoreWithBestPoints !== null
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -10
                anchors.verticalCenterOffset: -10
                sourceComponent: Grade {
                    scoreWithBestPoints: songList.currentItem?.scoreWithBestPoints || null
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
                scoreWithBestPoints: songList.currentItem?.scoreWithBestPoints || null
                bestStats: songList.currentItem?.bestStats || null
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
                property bool startAndSelectPressed: Input.start1 && Input.select1 || Input.start2 && Input.select2

                onStartAndSelectPressedChanged: {
                    if (startAndSelectPressed) {
                        login.enabled = !login.enabled;
                    }
                }
            }
        }
    }
}