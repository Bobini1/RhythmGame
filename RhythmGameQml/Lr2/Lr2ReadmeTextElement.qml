pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

Item {
    id: readmeElement

    required property var screenRoot
    property var dsts: []
    property var srcData: null
    property var activeOptionsState: null
    property var activeOptions: []
    property var chart: null
    property int skinTime: 0
    property int timerFire: 0
    property real skinScale: 1

    readonly property var root: screenRoot

    clip: true

    Component.onCompleted: {
        if (srcData && srcData.readme && srcData.readmeId === 0) {
            root.lr2ReadmeLineSpacing = Math.max(1, srcData.readmeLineSpacing || 18);
            root.clampReadmeOffsets();
        }
    }

    Repeater {
        model: readmeElement.root.readmeLinesForSource(readmeElement.srcData)

        Lr2TextRenderer {
            anchors.fill: parent
            dsts: readmeElement.dsts
            srcData: readmeElement.srcData
            skinTime: readmeElement.skinTime
            activeOptionsState: readmeElement.activeOptionsState
            activeOptions: readmeElement.activeOptions
            timerFire: readmeElement.timerFire
            scaleOverride: readmeElement.skinScale
            offsetX: readmeElement.root.lr2ReadmeOffsetX
            offsetY: readmeElement.root.lr2ReadmeOffsetY
                + index * (readmeElement.srcData
                    ? readmeElement.srcData.readmeLineSpacing
                    : readmeElement.root.lr2ReadmeLineSpacing)
            resolvedText: modelData
        }
    }
}

