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
    required property var selectBarGeometry

    readonly property var root: screenRoot
    readonly property bool rootReady: root !== undefined && root !== null
    readonly property real skinW: rootReady ? root.skinW : 0
    readonly property real skinH: rootReady ? root.skinH : 0
    readonly property real skinScale: rootReady ? root.skinScale : 1
    readonly property var skinRuntime: rootReady ? root.skinRuntimeRef : null
    readonly property var skinTiming: rootReady ? root.skinTimingRef : null
    readonly property var valueResolver: rootReady ? root.skinValueResolverRef : null
    readonly property var resolvedTextRegistry: rootReady ? root.skinResolvedTextRegistryRef : null
    readonly property var sliderState: rootReady ? root.skinSliderStateRef : null
    readonly property var selectPanelController: rootReady ? root.selectPanelControllerRef : null
    readonly property var selectHoverState: rootReady ? root.selectHoverStateRef : null
    readonly property var selectSearchState: rootReady ? root.selectSearchStateRef : null
    readonly property var runtimeElementDescriptors: skinRuntime ? skinRuntime.elementDescriptors : []
    readonly property int runtimeDescriptorRevision: skinRuntime ? skinRuntime.descriptorRevision : 0
    readonly property bool screenUpdatesActive: rootReady && root.screenUpdatesActive
    readonly property bool selectScreen: rootReady && root.effectiveScreenKey === "select"
    readonly property bool selectScreenActive: screenUpdatesActive && selectScreen
    readonly property var gameplayFrameState: rootReady ? root.gameplayFrameStateRef : null
    Lr2SelectPointerController {
        id: selectPointer
        screenRoot: sceneRoot.root
        selectContext: sceneRoot.selectContext
        selectBarGeometry: sceneRoot.selectBarGeometry
        runtimeElementDescriptors: sceneRoot.runtimeElementDescriptors
        skinRuntime: sceneRoot.skinRuntime
        sliderState: sceneRoot.sliderState
        selectPanelController: sceneRoot.selectPanelController
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
                sliderState: sceneRoot.sliderState
                selectHoverState: sceneRoot.selectHoverState
                active: sceneRoot.selectScreenActive
            }

            // Event-driven sprites may not otherwise reach the scene graph
            // until the first hit. Keep one obscured node per source resident
            // so large textures are uploaded while play is being prepared.
            Repeater {
                model: sceneRoot.rootReady && sceneRoot.root.gameplayScreenActive
                    ? sceneRoot.skinModel.gameplayPreloadSources
                    : []

                delegate: Image {
                    required property string modelData

                    x: 0
                    y: 0
                    z: -1000000
                    width: 1
                    height: 1
                    opacity: 0.001
                    source: Lr2SkinUtils.fileUrlForPath(modelData)
                    cache: true
                    asynchronous: false
                    smooth: false
                    mipmap: false
                }
            }

            Repeater {
                id: skinRepeater
                model: sceneRoot.skinModel

                delegate: Lr2SkinElementDelegate {
                    readonly property int sceneDescriptorRevision: sceneRoot.runtimeDescriptorRevision
                    readonly property var sceneElementState: sceneRoot.skinRuntime && sceneDescriptorRevision >= 0
                        ? sceneRoot.skinRuntime.descriptor(index)
                        : ({})
                    readonly property bool usesChartAssetSource: sceneElementState.sourceTreeUsesChartAsset
                    screenRoot: sceneRoot.root
                    skinModel: sceneRoot.skinModel
                    selectContext: sceneRoot.selectContext
                    playContext: sceneRoot.playContext
                    pointerController: selectPointer
                    skinRuntime: sceneRoot.skinRuntime
                    skinTiming: sceneRoot.skinTiming
                    valueResolver: sceneRoot.valueResolver
                    resolvedTextRegistry: sceneRoot.resolvedTextRegistry
                    sliderState: sceneRoot.sliderState
                    selectPanelController: sceneRoot.selectPanelController
                    selectHoverState: sceneRoot.selectHoverState
                    selectSearchState: sceneRoot.selectSearchState
                    runtimeDescriptorRevision: sceneRoot.runtimeDescriptorRevision
                    gameplayFrameState: sceneRoot.gameplayFrameState
                    screenUpdatesActive: sceneRoot.screenUpdatesActive
                    stageFileSource: usesChartAssetSource && sceneRoot.rootReady ? sceneRoot.root.chartAssetStageFileSource : ""
                    backBmpSource: usesChartAssetSource && sceneRoot.rootReady ? sceneRoot.root.chartAssetBackBmpSource : ""
                    bannerSource: usesChartAssetSource && sceneRoot.rootReady ? sceneRoot.root.chartAssetBannerSource : ""
                    skinW: sceneRoot.skinW
                    skinH: sceneRoot.skinH
                    skinScale: sceneRoot.skinScale
                }
            }

            Lr2SkinCursor {
                anchors.fill: parent
                screenRoot: sceneRoot.root
                skinTiming: sceneRoot.skinTiming
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
