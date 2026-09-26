pragma Translator: "Result"

import RhythmGameQml
import QtQuick
import QtQuick.Shapes
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers
import "../common"

Item {
    id: root

    // Private shared presentation for the two independently typed screen roots.
    required property var resultData
    readonly property var course: resultData instanceof CourseResultContext ? resultData.course : null
    readonly property list<ChartData> chartDatas: resultData instanceof CourseResultContext ? resultData.charts : []
    readonly property ChartData chartData: resultData instanceof ResultContext ? resultData.chartData : null
    readonly property var scores: resultData.players.map(player => player.score)
    readonly property list<Profile> profiles: resultData.players.map(player => player.profile)
    readonly property bool arenaResultMatches: root.resultData instanceof ResultContext && root.resultData.arenaActive
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    readonly property string commonImagesUrl: Qt.resolvedUrl("../common/") + "images/"
    readonly property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    readonly property var score1: scores[0]
    readonly property var score2: scores[1] || null
    readonly property Profile profile1: profiles[0]
    readonly property Profile profile2: profiles[1] || null
    readonly property bool isBattle: score1 && score2
    readonly property var themeVars: (Rg.profileList.mainProfile.vars.themeVars.result || {})[QmlUtils.themeName] || ({})
    readonly property var chartKeymode: chartData ? chartData.keymode : chartDatas[0].keymode

    ThemeFont {
        id: resultStatsFont
        fileName: root.themeVars.resultStatsFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    ThemeFont {
        id: resultTitleFont
        fileName: root.themeVars.resultTitleFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    function cycleGaugeForKey(key) {
        if (key === BmsKey.Col16) {
            return side1.cycleGauge();
        }
        if (key === BmsKey.Col26) {
            if (side2Loader.item) {
                return side2Loader.item.cycleGauge();
            }
            return side1.cycleGauge();
        }
        return false;
    }

    StandardResultInput {
        result: root.resultData
        enabled: root.enabled
        tryHandleButtonAction: key => root.cycleGaugeForKey(key)
        confirmEnabled: !(root.arenaResultMatches
                          && Rg.arenaSession.chatOpen)
    }

    Image {
        id: resultBackground

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + (root.score1.result.clearType === "FAILED" ? "failed.png" : "clear.png")
        width: parent.width

        AudioPlayer {
            source: {
                let clear = root.score1.result.clearType !== "FAILED";
                if (root.course) {
                    if (clear) {
                        return Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "course_clear";
                    }
                    return Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "course_fail";
                }
                if (clear) {
                    return Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "clear";
                }
                return Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "fail";
            }
            playing: true
        }

        Shortcut {
            enabled: root.enabled
            sequence: "F6"

            onActivated: {
                let date = new Date();
                let timestamp = Qt.formatDateTime(date, "yyyyMMdd_HHmmss");

                let g = Helpers.getGrade(root.score1.result.points, root.score1.result.maxPoints).toUpperCase();
                let clearType = root.score1.result.clearType;

                let prefix = "";
                if (root.chartData) {
                    let info = Rg.tables.search(root.chartData.md5);
                    if (info.length > 0) {
                        prefix = info[0].symbol + info[0].levelName + " ";
                    } else {
                        let diff = Helpers.difficultyName(root.chartData.difficulty);
                        let level = root.chartData.playLevel;
                        prefix = (diff ? diff + " " : "") + level + " ";
                    }
                }

                let rawTitle = root.chartData ? root.chartData.title + (root.chartData.subtitle ? " " + root.chartData.subtitle : "") : root.course?.name ?? "";

                let title = Helpers.sanitizeFilename(rawTitle);

                let filename = timestamp + "_" + prefix + title + " " + clearType + " " + g + ".png";
                root.grabToImage(function (grabResult) {
                    let filepath = Rg.programSettings.screenshotsFolder + "/" + filename;
                    if (grabResult.saveToFile(filepath)) {
                        Rg.programSettings.copyImageToClipboard(filepath);
                        screenshotMessage.show(qsTr("Screenshot saved to %1 and clipboard.").arg(filepath));
                    } else {
                        screenshotMessage.show(qsTr("Failed to save screenshot."));
                    }
                });
            }
        }
        Item {
            id: scaledRoot

            height: 1080
            scale: Math.min(parent.width / width, parent.height / height)
            width: 1920
            anchors.centerIn: parent

            Text {
                id: screenshotMessage

                anchors.top: parent.top
                anchors.topMargin: 16
                anchors.horizontalCenter: parent.horizontalCenter
                width: titleArtist.width
                z: 10
                opacity: 0
                color: "white"
                font: resultStatsFont.uiFont({
                    weight: resultStatsFont.boldFontWeight,
                    variableAxes: resultStatsFont.boldVariableAxes,
                    italic: resultStatsFont.italic,
                    pixelSize: 28
                })
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                style: Text.Outline
                styleColor: "black"

                function show(msg) {
                    text = msg;
                    fadeAnim.restart();
                }

                SequentialAnimation {
                    id: fadeAnim
                    NumberAnimation {
                        target: screenshotMessage
                        property: "opacity"
                        to: 1.0
                        duration: 150
                    }
                    PauseAnimation {
                        duration: 3000
                    }
                    NumberAnimation {
                        target: screenshotMessage
                        property: "opacity"
                        to: 0.0
                        duration: 600
                    }
                }
            }

            Row {
                id: chartInfoRow
                anchors.left: parent.left
                anchors.leftMargin: 14

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
                    width: 1286
                }
                ChartInfo {
                    difficulty: root.chartData?.difficulty
                    total: root.chartData?.total
                    noteCount: root.score1.result.normalNoteCount + root.score1.result.lnCount + root.score1.result.bssCount + root.score1.result.scratchCount

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 24
                    height: titleArtist.height
                    width: 280
                }
            }

            Side {
                id: side1

                score: root.score1
                isBattle: root.isBattle
                profile: root.profile1
                width: parent.width
                anchors.top: chartInfoRow.bottom
                chartKeymode: root.chartKeymode
                arenaResultActive: root.arenaResultMatches
            }

            Loader {
                id: side2Loader

                active: root.isBattle
                width: parent.width
                anchors.top: chartInfoRow.bottom
                sourceComponent: Side {
                    score: root.score2
                    isBattle: root.isBattle
                    profile: root.profile2
                    mirrored: true
                    chartKeymode: root.chartKeymode
                    arenaResultActive: root.arenaResultMatches
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

        StandardArenaResultOverlay {
            id: arenaResultOverlay

            parent: resultBackground
            result: root.resultData instanceof ResultContext ? root.resultData : null
            visible: arenaResultOverlay.active && !root.isBattle
                && arenaResultOverlay.overlayVisible
            minimumPixelSize: Qt.size(360, 240)
            themeVars: root.themeVars
            viewport: resultBackground
            z: 20

            panelComponent: ArenaResultPanel {
                id: arenaResultPanel

                anchors.fill: parent
                localMemberId: String(Rg.arenaSession.selfMemberId || "")
                result: root.arenaResult
                session: Rg.arenaSession
                statsFontFamily: resultStatsFont.fontFamily
                textFontFamily: resultTitleFont.fontFamily
                expanded: arenaResultOverlay.expanded

                onChatSelected: chat => {
                    arenaResultOverlay.setChatSelected(chat);
                }
                onExpandedChanged: {
                    arenaResultOverlay.setExpanded(
                                arenaResultPanel.expanded);
                }
            }
        }
    }

    TransientInputFocusDismissLayer {}
}
