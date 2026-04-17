import QtQuick
import RhythmGameQml 1.0

Item {
    id: root
    property string csvPath
    property var chart
    property var skinSettings

    Shortcut {
        enabled: root.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
    
    readonly property var activeOptions: {
        let result = [52, 900];
        let settings = root.skinSettings || {};

        let stagefileMode = settings.stagefile;
        if (stagefileMode === "IGNORE") {
            result = [52, 901];
        }

        let hasStageFile = !!(root.chart && root.chart.chartData && root.chart.chartData.stageFile);
        result.push(hasStageFile ? 191 : 190);

        let difficulty = root.chart && root.chart.chartData ? root.chart.chartData.difficulty : 0;
        if (difficulty >= 1 && difficulty <= 5) {
            result.push(150 + difficulty);
        }

        return result;
    }

    function resolveText(st) {
        if (!root.chart) {
            return "";
        }
        let chartData = root.chart.chartData;
        switch (st) {
        case 10:
            if (chartData) {
                let title = chartData.title || "";
                if (chartData.subtitle) {
                    title += " " + chartData.subtitle;
                }
                return title.replace(/\r\n|\n|\r/g, " ");
            }
            return root.chart.course ? root.chart.course.name : "";
        case 11:
            return chartData ? (chartData.subtitle || "") : "";
        case 13:
            return chartData ? (chartData.genre || "") : "Course";
        case 14:
            return chartData ? (chartData.artist || "") : "";
        case 15:
            return chartData ? (chartData.subartist || "") : "";
        default:
            return "";
        }
    }

    // Monotonic scene clock. Anchored to the moment the screen loads so that
    // animations don't drift if frames are skipped.
    readonly property double sceneStartMs: Date.now()
    property int globalSkinTime: 0
    Timer {
        id: skinStopwatch
        interval: 16
        running: true
        repeat: true
        onTriggered: {
            root.globalSkinTime = Date.now() - root.sceneStartMs;
        }
    }

    // Timer fire times (ms since scene start). For the decide scene every
    // timer reference is SCENE_START (index 0), which fires immediately.
    // Other scenes will add more entries (e.g. ready-to-start, fade-in, etc.).
    readonly property var timers: ({ 0: 0 })

    Lr2SkinModel {
        id: skinModel
        csvPath: root.csvPath
        settingValues: root.skinSettings || {}
        activeOptions: root.activeOptions
    }

    readonly property real skinW: 640
    readonly property real skinH: 480
    readonly property real skinScale: 1.0

    // Outer container: visual size after scaling, centered in the screen
    Item {
        id: skinViewport
        width: skinW * skinScale
        height: skinH * skinScale
        anchors.centerIn: parent

        // Inner canvas: exact viewport size - DST coords in 640x480 space
        // are scaled up to this larger size uniformly
        Item {
            id: skinContainer
            width: skinW * skinScale
            height: skinH * skinScale

            Repeater {
                id: skinRepeater
                model: skinModel

                delegate: Loader {
                    id: elemLoader
                    x: 0; y: 0
                    width: skinW * skinScale
                    height: skinH * skinScale
                    z: index

                    sourceComponent: {
                        if (model.type === 0) {
                            return imageComponent;
                        } else if (model.type === 2) {
                            return textComponent;
                        }
                        return undefined;
                    }

                    Component {
                        id: imageComponent
                        Lr2SpriteRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.globalSkinTime
                            activeOptions: root.activeOptions
                            timers: root.timers
                            chart: root.chart
                            scaleOverride: skinScale
                        }
                    }

                    Component {
                        id: textComponent
                        Lr2TextRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.globalSkinTime
                            activeOptions: root.activeOptions
                            timers: root.timers
                            chart: root.chart
                            scaleOverride: skinScale
                            resolvedText: root.resolveText(model.src ? model.src.st : -1)
                        }
                    }
                }
            }
        }
    }
}
