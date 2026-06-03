pragma ValueTypeBehavior: Addressable
pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: sceneRoot

    required property var screenRoot
    required property var skinModel
    required property var playContext
    required property var selectContext

    readonly property var root: screenRoot
    readonly property bool rootReady: root !== undefined && root !== null
    readonly property real skinW: rootReady ? root.skinW : 0
    readonly property real skinH: rootReady ? root.skinH : 0
    readonly property real skinScale: rootReady ? root.skinScale : 1
    readonly property var skinRuntime: rootReady ? root.skinRuntimeRef : null
    readonly property int runtimeRevision: skinRuntime ? skinRuntime.revision : 0
    readonly property bool screenUpdatesActive: rootReady && root.screenUpdatesActive
    readonly property bool selectScreen: rootReady && root.effectiveScreenKey === "select"
    readonly property bool selectScreenActive: screenUpdatesActive && selectScreen
    readonly property var gameplayFrameState: rootReady ? root.gameplayFrameStateRef : null

    Lr2SelectPointerController {
        id: selectPointer
        screenRoot: sceneRoot.root
        selectContext: sceneRoot.selectContext
        skinRuntime: sceneRoot.skinRuntime
        skinScale: sceneRoot.skinScale
    }

    Item {
        id: skinViewport
        anchors.fill: parent

        // Children stay in skin coordinates; this transform performs the same
        // global non-uniform stretch LR2 applies at presentation time.
        Item {
            id: skinContainer
            width: sceneRoot.skinW
            height: sceneRoot.skinH
            transform: Scale {
                origin.x: 0
                origin.y: 0
                xScale: sceneRoot.root.skinVisualScaleX
                yScale: sceneRoot.root.skinVisualScaleY
            }

            Lr2SelectPointerSurface {
                anchors.fill: parent
                z: 100240
                screenRoot: sceneRoot.root
                selectContext: sceneRoot.selectContext
                skinModel: sceneRoot.skinModel
                pointerController: selectPointer
                active: sceneRoot.selectScreenActive
            }

            Repeater {
                id: skinRepeater
                model: sceneRoot.skinModel

                delegate: Lr2SkinElementDelegate {
                    readonly property bool usesChartAssetSource: Lr2SkinUtils.sourceTreeUsesChartAsset(src)
                    screenRoot: sceneRoot.root
                    skinModel: sceneRoot.skinModel
                    selectContext: sceneRoot.selectContext
                    playContext: sceneRoot.playContext
                    pointerController: selectPointer
                    skinRuntime: sceneRoot.skinRuntime
                    runtimeRevision: sceneRoot.runtimeRevision
                    gameplayFrameState: sceneRoot.gameplayFrameState
                    screenUpdatesActive: sceneRoot.screenUpdatesActive
                    stageFileSource: usesChartAssetSource && sceneRoot.rootReady ? selectContext.visualStageFileSource : ""
                    backBmpSource: usesChartAssetSource && sceneRoot.rootReady ? selectContext.visualBackBmpSource : ""
                    bannerSource: usesChartAssetSource && sceneRoot.rootReady ? selectContext.visualBannerSource : ""
                    skinW: sceneRoot.skinW
                    skinH: sceneRoot.skinH
                    skinScale: sceneRoot.skinScale
                }
            }

            Lr2SkinCursor {
                anchors.fill: parent
                screenRoot: sceneRoot.root
                skinModel: sceneRoot.skinModel
            }

            Lr2ReadmePointerArea {
                anchors.fill: parent
                screenRoot: sceneRoot.root
                skinItem: skinContainer
                skinScale: sceneRoot.skinScale
            }
        }
    }
}
