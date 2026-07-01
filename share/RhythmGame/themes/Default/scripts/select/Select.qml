pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQml
import "../common/helpers.js" as Helpers
import "../common"
import "./options"

FocusScope {
    id: selectScreen

    function reloadCurrentFolderOrTable() {
        return root.reloadCurrentFolderOrTable();
    }

    function openSelectedFolder() {
        return root.openSelectedFolder();
    }

    function openSelectedInternetRanking() {
        return root.openSelectedInternetRanking();
    }

    Image {
        id: root

        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
        readonly property string commonImagesUrl: Qt.resolvedUrl("../common/") + "images/"
        readonly property bool selectShortcutEnabled: root.enabled && !searchEdit.activeFocus && !options.visible
        readonly property bool selectOverlayShortcutEnabled: root.enabled && !searchEdit.activeFocus
        readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars
        readonly property var themeVars: (Rg.profileList.mainProfile.vars.themeVars.select || {})[QmlUtils.themeName] || ({})
        readonly property int replayType: replayTypeIndex(generalVars ? generalVars.replayType : 0)

        ThemeFont {
            id: scoreInfoFont
            fileName: root.themeVars.scoreInfoFont
            fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
        }

        ThemeFont {
            id: songInfoFont
            fileName: root.themeVars.songInfoFont
        }

        function getScore(type) {
            let scoreList = songList.currentItem?.scores || [];
            if (scoreList.length === 0) {
                return null;
            }
            switch (type) {
                case 0:
                    return scoreList[scoreList.length - 1];
                    break;
                case 1:
                    return songList.currentItem.scoreWithBestPoints;
                    break;
                case 2:
                    let clearType = Helpers.getClearType(scoreList);
                    let score = scoreList.find((score) => {
                        return score.result.clearType === clearType;
                    });
                    if (score) {
                        return score;
                    }
                    break;
                case 3:
                    let bestScore = scoreList.reduce((prev, curr) => {
                        return prev.result.maxCombo > curr.result.maxCombo ? prev : curr;
                    });
                    if (bestScore) {
                        return bestScore;
                    }
                    break;
            }
            return null;
        }

        function openReplay(type, button) {
            let score = getScore(type);
            if (!score) {
                return false;
            }
            if (button === Qt.RightButton) {
                globalRoot.openResult([score], [Rg.profileList.mainProfile], songList.current);
                return true;
            }
            let replay = true;
            if (button === Qt.MiddleButton) {
                replay = false;
            }
            let path = songList.current instanceof course ? songList.current : songList.current.path;
            let func = songList.current instanceof course ? globalRoot.openCourse : globalRoot.openChart;
            func(path, Rg.profileList.mainProfile, false, replay, score, null, false, false, null);
            return true;
        }

        function focusSongList() {
            songList.forceActiveFocus();
        }

        function selectedReplayType() {
            if (Keys.digit2Pressed) {
                return 1;
            }
            if (Keys.digit3Pressed) {
                return 2;
            }
            if (Keys.digit4Pressed) {
                return 3;
            }
            return replayType;
        }

        function cycleReplayType() {
            setReplayType(replayType + 1);
            return true;
        }

        function replayTypeIndex(value) {
            let numeric = Math.floor(Number(value || 0));
            if (isNaN(numeric)) {
                numeric = 0;
            }
            return ((numeric % 4) + 4) % 4;
        }

        function setReplayType(value) {
            if (generalVars) {
                generalVars.replayType = replayTypeIndex(value);
            }
        }

        function openSelectedAutoplay() {
            return songList.openPlayable(songList.current, true, false, null);
        }

        function openSelectedReplay(button) {
            let target = songList.current;
            if (!(target instanceof ChartData || target instanceof course)) {
                return false;
            }
            return openReplay(selectedReplayType(), button || Qt.LeftButton);
        }

        function reloadCurrentFolderOrTable() {
            if (globalRoot.reloadTableForItem(songList.current)) {
                return true;
            }
            return songList.reloadCurrentFolderOrTable();
        }

        function openSelectedFolder() {
            let target = songList.current;
            if (target instanceof ChartData && target.chartDirectory) {
                return globalRoot.openLocalFolder(target.chartDirectory);
            }
            if (typeof target === "string") {
                return globalRoot.openLocalFolder(target);
            }
            return false;
        }

        function openSelectedInternetRanking() {
            return rankingPosition.rankingLink.length > 0
                && Qt.openUrlExternally(rankingPosition.rankingLink);
        }

        function openSelectedReadme() {
            let target = songList.current;
            if (!(target instanceof ChartData) || !target.chartDirectory) {
                return false;
            }
            let paths = Rg.songDirectoryFilePathFetcher.getReadmeFilePaths([target.chartDirectory]);
            let path = paths[target.chartDirectory] || "";
            return path.length > 0 && Qt.openUrlExternally(globalRoot.localFileUrl(path));
        }

        function showAllChartsForCurrentSong() {
            let target = songList.current;
            return target instanceof ChartData
                && !!target.chartDirectory
                && songList.openChartDirectory(target.chartDirectory, target);
        }

        function cycleSortMode(delta) {
            sortButton.cycle(delta === undefined ? 1 : delta);
            return true;
        }

        function toggleDetailOptions() {
            options.togglePlayOptions();
            return true;
        }

        function openKeyConfig() {
            globalRoot.openSettings(5);
            return true;
        }

        function handleSelectDigitShortcut(digit) {
            switch (digit) {
            case 1:
                keymodeButton.cycle(1);
                return true;
            case 2:
                return cycleSortMode(1);
            case 3:
                return false;
            case 4:
                return cycleReplayType();
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

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + "bg.png"
        width: parent.width

        onEnabledChanged: {
            if (enabled) {
                previewDelayTimer.restart();
                songList.refresh();
            } else {
                previewDelayTimer.stop();
                playMusic.stop();
                bgm.stop();
                playMusic.waitingForStop = false;
            }
        }

        Item {
            anchors.centerIn: parent
            height: 1080
            scale: Math.min(parent.width / 1920, parent.height / 1080)
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
                currentItem: songList.currentItem
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
                fadeInMillis: 1000
                looping: true
                source: songList.current instanceof ChartData ? songList.previewFiles[songList.current.chartDirectory] : undefined
                Component.onCompleted: {
                    playing = true;
                }

                onSourceChanged: {
                    previewDelayTimer.stop();
                    playMusic.stop();
                    waitingForStop = playMusic.source !== "";
                }
            }
            AudioPlayer {
                id: bgm
                looping: true
                source: Rg.profileList.mainProfile.vars.generalVars.bgmPath + "select";
                fadeInMillis: 1000
                property bool canPlay: (!playMusic.playing || playMusic.source === "") && root.enabled
                onCanPlayChanged: {
                    if (!canPlay) {
                        bgm.stop();
                    }
                }
            }
            Timer {
                id: bgmDelayTimer

                interval: 500
                running: bgm.canPlay

                onTriggered: {
                    bgm.play();
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
                loading: ranking.loading
                rankingTotalEntries: ranking.playerCount
                visible: ranking.visible
                rankingLink: {
                    if (!rankingTotalEntries || (ranking.provider === OnlineRankingModel.LR2IR && rankingTotalEntries <= 1)) {
                        return "";
                    }
                    switch (ranking.provider) {
                        case OnlineRankingModel.RhythmGame:
                            return Rg.profileList.mainProfile.vars.generalVars.websiteBaseUrl + "/charts/" + songList.current.md5;
                        case OnlineRankingModel.LR2IR:
                            return "http://www.dream-pro.info/~lavalse/LR2IR/search.cgi?mode=ranking&bmsmd5=" + songList.current.md5;
                        case OnlineRankingModel.Tachi:
                            if (!ranking.chartId || !ranking.tachiGameId) {
                                return "";
                            }
                            return "https://boku.tachi.ac/games/" + ranking.tachiGameId +
                                "/charts/" + ranking.chartId;
                    }
                }
                rankingPosition: {
                    let entries = ranking.entries;
                    switch (ranking.provider) {
                        case OnlineRankingModel.RhythmGame:
                            for (let i = 0; i < entries.length; i++) {
                                if (entries[i].userId === Rg.profileList.mainProfile.onlineUserData?.userId) {
                                    return i + 1;
                                }
                            }
                            break;
                        case OnlineRankingModel.LR2IR:
                            for (let i = 0; i < entries.length; i++) {
                                if (!(entries[i] instanceof rankingEntry)) {
                                    return i + 1;
                                }
                            }
                            break;
                        case OnlineRankingModel.Tachi:
                            for (let i = 0; i < entries.length; i++) {
                                if (entries[i].userId === Rg.profileList.mainProfile.tachiData?.userId) {
                                    return i + 1;
                                }
                            }
                            break;
                    }
                    return 0;
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
            Text {
                anchors.bottom: scoreInfo.top
                anchors.horizontalCenter: scoreInfo.horizontalCenter
                font.family: scoreInfoFont.fontFamily
                font.weight: scoreInfoFont.fontWeight
                font.italic: scoreInfoFont.italic
                font.pixelSize: 19
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
                    font.family: songInfoFont.fontFamily
                    font.weight: songInfoFont.fontWeight
                    font.italic: songInfoFont.italic
                    font.pixelSize: 20
                    height: 25
                    width: 540
                    wrapMode: TextEdit.NoWrap

                    function searchResultText(count) {
                        return qsTr("%n chart(s) found", "", count);
                    }

                    function submitOrOpenCurrent() {
                        let query = searchEdit.text.trim();
                        if (query.length > 0) {
                            let count = songList.search(query);
                            searchEdit.text = searchResultText(count);
                            showingSearchResultText = true;
                        } else {
                            songList.goForward(songList.current);
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
                        songList.handleWheel(wheel);
                    }
                }
            }
            Ranking {
                id: ranking
                chartData: songList.current
                anchors.left: search.right
                anchors.leftMargin: -9
                anchors.bottom: search.top
                anchors.bottomMargin: -11
                provider: rankingPosition.provider
                bestPointsScore: songList.currentItem?.scoreWithBestPoints || null
                bestClearTypeScore: songList.currentItem?.scoreWithBestClear || null
            }
            KeymodeButton {
                id: keymodeButton

                anchors.left: parent.left
                anchors.leftMargin: 900
                anchors.top: parent.top
                anchors.topMargin: 20
                generalVars: Rg.profileList.mainProfile.vars.generalVars
            }
            SortButton {
                id: sortButton

                anchors.left: keymodeButton.left
                anchors.top: keymodeButton.bottom
                anchors.topMargin: 20
                generalVars: Rg.profileList.mainProfile.vars.generalVars
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
}
