pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQml
import "../common"
import "./options"

FocusScope {
    id: selectScreen

    Image {
        id: root

        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
        readonly property string commonImagesUrl: Qt.resolvedUrl("../common/") + "images/"
        readonly property bool selectShortcutEnabled: root.enabled && !searchEdit.activeFocus && !options.visible
        readonly property bool selectOverlayShortcutEnabled: root.enabled && !searchEdit.activeFocus
        readonly property bool arenaSeated: Rg.arenaSession.state === ArenaSession.InRoom
            || Rg.arenaSession.state === ArenaSession.Reconnecting
        readonly property real contentScale: Math.min(width / 1920,
                                                       height / 1080)
        readonly property real contentLeft:
            (width - 1920 * contentScale) / 2
        readonly property real contentTop:
            (height - 1080 * contentScale) / 2
        readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars
        readonly property var themeVars: (Rg.profileList.mainProfile.vars.themeVars.select || {})[QmlUtils.themeName] || ({})

        ThemeFont {
            id: scoreInfoFont
            fileName: root.themeVars.scoreInfoFont
            fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
        }

        ThemeFont {
            id: songInfoFont
            fileName: root.themeVars.songInfoFont
        }

        function openReplay(type, button) {
            if (arenaSeated) {
                return false;
            }
            if (button === Qt.LeftButton) {
                return songList.controller.openReplay(type);
            }
            const score = songList.controller.resultForReplayType(type, false);
            if (!score) {
                return false;
            }
            if (button === Qt.RightButton) {
                return songList.controller.openResult(songList.current, score);
            }
            return songList.controller.openPlayable(songList.current, false, false, score);
        }

        function focusSongList() {
            songList.forceActiveFocus();
        }

        function openSelectedReadme() {
            return songList.controller.openSelectedReadme();
        }

        function showAllChartsForCurrentSong() {
            return songList.controller.showAllChartsForCurrentSong();
        }

        function toggleDetailOptions() {
            options.togglePlayOptions();
            return true;
        }

        function openKeyConfig() {
            globalRoot.openSettings("keys");
            return true;
        }

        function handleSelectDigitShortcut(digit) {
            switch (digit) {
            case 1:
                keymodeButton.cycle(1);
                return true;
            case 2:
                return songList.controller.cycleSortMode(1);
            case 3:
                return false;
            case 4:
                return songList.controller.cycleReplayType();
            case 5:
                return toggleDetailOptions();
            case 6:
                return openKeyConfig();
            case 8:
                return showAllChartsForCurrentSong();
            case 9:
                return openSelectedReadme();
            default:
                return false;
            }
        }

        StandardSelectController {
            id: selection

            inputEnabled: songList.enabled
            shortcutsEnabled: songList.enabled
            feedbackEnabled: songList.enabled
            minimumEntryCount: songList.pathItemCount
            preview.enabled: songList.enabled
            preview.moving: songList.movingInAnyWay || vbar.pressed

            onOpenedFolder: songList.openedFolder()
            onFocusRequested: index => {
                songList.positionViewAtIndex(Math.max(0, index), PathView.Center);
                songList.resetNavigation();
            }
            onMoveRequested: (steps, repeated, analog) => {
                songList.moveSelection(steps, repeated, analog);
            }
        }

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + "bg.png"
        width: parent.width

        Item {
            anchors.centerIn: parent
            height: 1080
            scale: root.contentScale
            width: 1920
            clip: true

            enabled: !options.visible

            TapHandler {
                acceptedButtons: Qt.LeftButton
                gesturePolicy: TapHandler.WithinBounds

                onTapped: (eventPoint, button) => {
                    if (!searchEdit.activeFocus) {
                        return;
                    }
                    let searchPoint = search.mapFromItem(search.parent,
                                                         eventPoint.position.x,
                                                         eventPoint.position.y);
                    if (searchPoint.x >= 0 && searchPoint.x <= search.width
                            && searchPoint.y >= 0 && searchPoint.y <= search.height) {
                        return;
                    }
                    root.focusSongList();
                }
            }

            StageFile {
                id: stageFile
                anchors {
                    left: parent.left
                    top: parent.top
                    leftMargin: 80
                    topMargin: 120
                }
                fillMode: Image.Stretch
                height: 480
                width: 640
            }
            StandardArenaSelectOverlay {
                parent: root
                defaultPixelRectHint: Qt.rect(root.contentLeft + 728 * root.contentScale,
                                              root.contentTop + 120 * root.contentScale,
                                              520 * root.contentScale,
                                              480 * root.contentScale)
                navigationFocusTarget: songList
                themeVars: root.themeVars

                z: options.visible ? 0 : 3
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
                currentItem: songList.currentItem
                selection: songList.controller
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
                        songList.offset = songList.count * (1 - vbar.position);
                        songList.resetNavigation();
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
                    delayed: true
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

                controller: selection
                anchors.right: parent.right
                anchors.rightMargin: 43
                anchors.top: parent.top
                anchors.topMargin: -40
                focus: true
                height: parent.height
                width: parent.width / 2
            }
            Loader {
                id: graphLoader
                active: songList.current instanceof ChartData
                anchors.horizontalCenter: songList.horizontalCenter
                anchors.horizontalCenterOffset: 19
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 75
                sourceComponent: Graph {
                    bpms: songList.current ? songList.current.bpmChanges : []
                    histogramData: songList.current ? songList.current.histogramData : [[], [], [], []]
                    mainBpm: songList.current ? songList.current.mainBpm : 0
                    maxBpm: songList.current ? songList.current.maxBpm : 0
                    minBpm: songList.current ? songList.current.minBpm : 0
                    length: songList.current ? songList.current.length : 0
                    normalCount: songList.current ? songList.current.normalNoteCount : 0
                    scratchCount: songList.current ? songList.current.scratchCount : 0
                    lnCount: songList.current ? songList.current.lnCount : 0
                    bssCount: songList.current ? songList.current.bssCount : 0
                    gapsEnabled: Rg.profileList.mainProfile.vars.themeVars.select[QmlUtils.themeName].densityGraphGapsEnabled
                    bpmConnectorOpacity: Rg.profileList.mainProfile.vars.themeVars.select[QmlUtils.themeName].densityGraphBpmConnectorOpacity
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

                active: songList.currentItem?.scoreWithBestPoints || false
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -10
                anchors.verticalCenterOffset: 150
                width: 350
                height: 400
                sourceComponent: Grade {
                    scoreWithBestPoints: songList.currentItem?.scoreWithBestPoints || null
                }
            }
            RankingPosition {
                id: rankingPosition

                anchors.left: grade.left
                width: 276
                anchors.top: grade.bottom
                anchors.topMargin: -146
                ranking: songList.controller.ranking
                visible: songList.controller.ranking.available
            }
            Text {
                anchors.bottom: scoreInfo.top
                anchors.horizontalCenter: scoreInfo.horizontalCenter
                font: scoreInfoFont.uiFont({
                    weight: scoreInfoFont.fontWeight,
                    variableAxes: scoreInfoFont.variableAxes,
                    italic: scoreInfoFont.italic,
                    pixelSize: 19
                })
                clip: true
                elide: Text.ElideRight
                fontSizeMode: Text.Fit
                height: 24
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                minimumPixelSize: 5
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.NoWrap
                anchors.horizontalCenterOffset: scoreInfo.lineWidth / 2 + scoreInfo.spacing / 2
                anchors.bottomMargin: 18
                text: qsTr("Score Details")
                    + (scoreInfo.scoreWithBestPoints?.imported && scoreInfo.scoreWithBestPoints?.sourceName
                       ? " · " + scoreInfo.scoreWithBestPoints.sourceName
                       : "")
                width: scoreInfo.lineWidth - 48
            }
            ScoreInfo {
                id: scoreInfo
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 80
                anchors.left: parent.left
                anchors.leftMargin: 80
                spacing: 12

                current: songList.current
                scoreWithBestPoints: songList.currentItem?.scoreWithBestPoints || null
                bestStats: songList.currentItem?.bestStats || null
                fontFile: root.themeVars.scoreInfoFont
            }
            Image {
                id: search

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.left: parent.left
                anchors.leftMargin: 45
                source: root.iniImagesUrl + "parts.png/search"

                TextEdit {
                    id: searchEdit

                    property bool showingSearchResultText: false

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20
                    anchors.left: parent.left
                    anchors.leftMargin: 51
                    color: "white"
                    font: songInfoFont.uiFont({
                        weight: songInfoFont.fontWeight,
                        variableAxes: songInfoFont.variableAxes,
                        italic: songInfoFont.italic,
                        pixelSize: 20
                    })
                    height: 25
                    width: 540
                    wrapMode: TextEdit.NoWrap

                    function searchResultText(count) {
                        return qsTr("%n chart(s) found", "", count);
                    }

                    function submitOrOpenCurrent() {
                        let query = searchEdit.text.trim();
                        if (query.length > 0) {
                            let count = songList.controller.search(query);
                            searchEdit.text = searchResultText(count);
                            showingSearchResultText = true;
                        } else {
                            songList.controller.goForward(songList.current);
                        }
                        root.focusSongList();
                    }

                    onActiveFocusChanged: {
                        if (activeFocus && showingSearchResultText) {
                            searchEdit.text = "";
                            showingSearchResultText = false;
                        }
                    }

                    Keys.onReturnPressed: event => {
                        submitOrOpenCurrent();
                        event.accepted = true;
                    }
                    Keys.onEnterPressed: event => {
                        submitOrOpenCurrent();
                        event.accepted = true;
                    }
                    Keys.onUpPressed: event => {
                        songList.decrementViewIndex(event.isAutoRepeat);
                        event.accepted = true;
                    }
                    Keys.onDownPressed: event => {
                        songList.incrementViewIndex(event.isAutoRepeat);
                        event.accepted = true;
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton

                    onWheel: wheel => {
                        wheel.accepted = selection.handleWheel(wheel);
                    }
                }
            }
            WindowBg {
                anchors.left: ranking.left
                anchors.verticalCenter: search.verticalCenter
                width: 400
                height: search.height

                ArenaActivityLabel {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    horizontalAlignment: Text.AlignHCenter
                    font: songInfoFont.uiFont({
                        weight: songInfoFont.fontWeight,
                        variableAxes: songInfoFont.variableAxes,
                        italic: songInfoFont.italic,
                        pixelSize: 22
                    })
                }
            }
            Ranking {
                id: ranking
                rankingState: songList.controller.ranking
                anchors.left: search.right
                anchors.leftMargin: -9
                anchors.bottom: search.top
                anchors.bottomMargin: -11
            }
            KeymodeButton {
                id: keymodeButton

                anchors.left: parent.left
                anchors.leftMargin: 900
                anchors.top: parent.top
                anchors.topMargin: 20
                controller: selection
            }
            SortButton {
                id: sortButton

                anchors.left: keymodeButton.left
                anchors.top: keymodeButton.bottom
                anchors.topMargin: 20
                selection: songList.controller
            }
            MouseArea {
                id: focusList

                anchors.fill: parent
                z: -1

                onClicked: {
                    root.focusSongList();
                }
            }
        }
        Options {
            id: options
            anchors.fill: parent
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "1"
            onActivated: root.handleSelectDigitShortcut(1)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "2"
            onActivated: root.handleSelectDigitShortcut(2)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "3"
            onActivated: root.handleSelectDigitShortcut(3)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "4"
            onActivated: root.handleSelectDigitShortcut(4)
        }
        Shortcut {
            enabled: root.selectOverlayShortcutEnabled
            sequence: "5"
            onActivated: root.handleSelectDigitShortcut(5)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "6"
            onActivated: root.handleSelectDigitShortcut(6)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "8"
            onActivated: root.handleSelectDigitShortcut(8)
        }
        Shortcut {
            enabled: root.selectShortcutEnabled
            sequence: "9"
            onActivated: root.handleSelectDigitShortcut(9)
        }
    }

    TransientInputFocusDismissLayer {}
}
