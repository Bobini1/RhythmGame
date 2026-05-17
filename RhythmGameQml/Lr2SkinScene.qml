pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml 1.0

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
    readonly property var renderChart: rootReady ? root.renderChart : null
    readonly property string stageFileSource: rootReady ? selectContext.visualStageFileSource : ""
    readonly property string backBmpSource: rootReady ? selectContext.visualBackBmpSource : ""
    readonly property string bannerSource: rootReady ? selectContext.visualBannerSource : ""
    readonly property bool screenUpdatesActive: rootReady && root.screenUpdatesActive
    readonly property bool selectScreenActive: screenUpdatesActive && root.effectiveScreenKey === "select"
    readonly property int valueRevision: !rootReady ? 0
        : (root.effectiveScreenKey === "select"
            ? root.selectRevision
                + selectContext.listRevision
                + selectContext.folderLampRevision
                + root.lr2SkinSettingsRevision
            : (root.resultScreenActive
                ? root.resultOldScoresRevision
                : root.gameplayRevision))
    readonly property int selectDetailValueRevision: !rootReady ? 0
        : root.selectDetailRevision
            + selectContext.listRevision
            + selectContext.folderLampRevision
            + root.lr2SkinSettingsRevision
    readonly property int selectStableBarGraphValueRevision: !rootReady ? 0
        : root.lr2SkinSettingsRevision
    readonly property int textSettingsRevisionKind: 0
    readonly property int textFocusedRevisionKind: 1
    readonly property int textListRevisionKind: 2
    readonly property int liveGameplayRhythmTimer: rootReady
        && root.gameplayScreenActive
        && root.gameplayFrameStateRef
            ? root.gameplayFrameStateRef.rhythmTimerSkinTime
            : -1

    function selectBarGraphUsesFocusedState(type: var) : var {
        return type === 101
            || (type >= 5 && type <= 9)
            || (type >= 40 && type <= 47)
            || (type >= 103 && type <= 115)
            || (type >= 140 && type <= 147);
    }

    function barGraphValueRevision(src: var) : var {
        if (!rootReady) {
            return 0;
        }
        if (root.effectiveScreenKey !== "select") {
            return valueRevision;
        }
        let type = src ? (src.graphType || 0) : 0;
        return selectBarGraphUsesFocusedState(type)
            ? selectDetailValueRevision
            : selectStableBarGraphValueRevision;
    }

    function sourceUsesChartAsset(src: var) : var {
        if (!src) {
            return false;
        }
        return src.specialType === 1 || src.specialType === 3 || src.specialType === 4;
    }

    function chartAssetSourceFor(src: var) : var {
        if (!src) {
            return "";
        }
        if (src.specialType === 1) {
            return stageFileSource;
        }
        if (src.specialType === 3) {
            return backBmpSource;
        }
        if (src.specialType === 4) {
            return bannerSource;
        }
        return "";
    }

    function sourceTreeUsesChartAsset(value: var, depth: var) : var {
        if (!value || depth > 4) {
            return false;
        }
        if (sourceUsesChartAsset(value)) {
            return true;
        }
        if (value.length !== undefined && typeof value !== "string") {
            for (let i = 0; i < value.length; ++i) {
                if (sourceTreeUsesChartAsset(value[i], depth + 1)) {
                    return true;
                }
            }
        }
        if (value.source && sourceTreeUsesChartAsset(value.source, depth + 1)) {
            return true;
        }
        if (value.sources && sourceTreeUsesChartAsset(value.sources, depth + 1)) {
            return true;
        }
        return false;
    }

    function hoverPointInSkinCoordinates() : var {
        return rootReady ? root.selectHoverPointInSkinCoordinates() : Qt.point(0, 0);
    }

    Lr2SelectPointerController {
        id: selectPointerController
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
            width: skinW
            height: skinH
            transform: Scale {
                origin.x: 0
                origin.y: 0
                xScale: root.skinVisualScaleX
                yScale: root.skinVisualScaleY
            }

            MouseArea {
                id: selectPointerMouseArea
                anchors.fill: parent
                enabled: sceneRoot.selectScreenActive
                visible: enabled
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                propagateComposedEvents: true
                preventStealing: true
                z: 100240
                property var pressedTarget: ({ kind: "none" })

                onPressed: (mouse) => {
                    pressedTarget = selectPointerController.targetAt(mouse.x, mouse.y, mouse.button);
                    if (pressedTarget.kind === "slider") {
                        root.clearSelectSearchFocus();
                        root.selectSliderFixedPoint = -1;
                        if (pressedTarget.element.selectScroll) {
                            root.selectScrollStartSkinTime = root.renderSkinTime;
                            selectContext.beginScrollFixedPointDrag();
                        }
                        selectPointerController.updateSlider(pressedTarget, mouse.x, mouse.y);
                        mouse.accepted = true;
                    } else if (pressedTarget.kind === "bar" || pressedTarget.kind === "blank") {
                        root.clearSelectSearchFocus();
                        mouse.accepted = pressedTarget.kind === "bar";
                    } else if (pressedTarget.kind === "button") {
                        mouse.accepted = true;
                    } else {
                        mouse.accepted = false;
                    }
                }
                onPositionChanged: (mouse) => {
                    if (pressedTarget.kind === "slider") {
                        selectPointerController.updateSlider(pressedTarget, mouse.x, mouse.y);
                        mouse.accepted = true;
                    }
                }
                onReleased: (mouse) => {
                    const releasedTarget = selectPointerController.targetAt(mouse.x, mouse.y, mouse.button);
                    const target = pressedTarget.kind !== "none"
                        ? pressedTarget
                        : releasedTarget;
                    if (target.kind === "slider") {
                        selectPointerController.finishSlider(target);
                        pressedTarget = { kind: "none" };
                        mouse.accepted = true;
                        return;
                    }
                    pressedTarget = { kind: "none" };
                    if (target.kind === "button") {
                        if (!selectPointerController.sameTarget(target, releasedTarget)) {
                            mouse.accepted = false;
                            return;
                        }
                        selectPointerController.clickButton(target, mouse);
                        return;
                    }
                    if (target.kind === "bar") {
                        root.handleBarRowClick(target.row, mouse);
                        mouse.accepted = true;
                        return;
                    }
                    if (target.kind === "blank") {
                        root.clearSelectSearchFocus();
                        mouse.accepted = false;
                    }
                }
                onClicked: (mouse) => {
                    const releasedTarget = selectPointerController.targetAt(mouse.x, mouse.y, mouse.button);
                    mouse.accepted = releasedTarget.kind !== "blank";
                }
                onDoubleClicked: (mouse) => {
                    const target = selectPointerController.targetAt(mouse.x, mouse.y, mouse.button);
                    pressedTarget = { kind: "none" };
                    if (target.kind !== "bar" || mouse.button !== Qt.LeftButton) {
                        mouse.accepted = false;
                        return;
                    }
                    selectContext.selectVisibleRow(target.row, skinModel.barCenter);
                    root.selectGoForward(selectContext.activationItem());
                    mouse.accepted = true;
                }
                onCanceled: {
                    selectPointerController.finishSlider(pressedTarget);
                    pressedTarget = { kind: "none" };
                }
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }

            HoverHandler {
                id: selectHoverHandler
                enabled: root.selectHoverTracking && root.selectHoverElementCount > 0
                target: skinContainer
                onHoveredChanged: {
                    if (hovered) {
                        if (root.updateSelectHoverPoint(point.position.x, point.position.y)) {
                            root.runSelectHoverRefresh();
                        }
                    } else {
                        root.clearSelectHoverPoint();
                    }
                }
            }

            FrameAnimation {
                id: selectHoverPointSampler
                running: selectHoverHandler.enabled && selectHoverHandler.hovered
                onTriggered: {
                    if (root.updateSelectHoverPoint(selectHoverHandler.point.position.x,
                                                    selectHoverHandler.point.position.y)) {
                        root.runSelectHoverRefresh();
                    }
                }
            }

            Repeater {
                id: skinRepeater
                model: skinModel

                delegate: Loader {
                    id: elemLoader
                    x: 0; y: 0
                    width: skinW * skinScale
                    height: skinH * skinScale
                    z: elementState.z || 0
                    readonly property int elementIndex: index
                    readonly property var elementData: ({ type: model.type, src: model.src, dsts: model.dsts })

                    readonly property var elementState: {
                        sceneRoot.runtimeRevision;
                        return sceneRoot.skinRuntime
                            ? sceneRoot.skinRuntime.descriptor(elementIndex)
                            : ({});
                    }

                    readonly property bool usesActiveOptions: elementState.usesActiveOptions
                    readonly property bool usesSkinTime: elementState.usesSkinTime
                    readonly property int dstTimer: elementState.dstTimer
                    readonly property int srcTimer: elementState.srcTimer
                    readonly property bool usesSelectHeldButtonTimer: elementState.usesSelectHeldButtonTimer
                    readonly property bool usesLiveDstClock: elementState.usesLiveDstClock
                    readonly property bool usesLiveSourceClock: elementState.usesLiveSourceClock
                    readonly property bool usesLiveSelectClock: elementState.usesLiveSelectClock
                    readonly property bool usesDynamicDstTimer: elementState.usesDynamicDstTimer
                    readonly property bool usesDynamicSrcTimer: elementState.usesDynamicSrcTimer
                    readonly property bool usesLiveGameplayDstTimer: sceneRoot.rootReady
                        && root.gameplayScreenActive
                        && elemLoader.dstTimer === 140
                    readonly property bool usesLiveGameplaySrcTimer: sceneRoot.rootReady
                        && root.gameplayScreenActive
                        && elemLoader.srcTimer === 140
                    readonly property int spriteStateOverrideKind: elementState.spriteStateOverrideKind
                    readonly property bool usesSpriteStateOverride: elementState.usesSpriteStateOverride
                    readonly property bool usesSpriteForceHidden: elementState.usesSpriteForceHidden
                    readonly property bool usesButtonFrameOverride: elementState.usesButtonFrameOverride
                    readonly property var elementActiveOptionsState: sceneRoot.skinRuntime
                        ? sceneRoot.skinRuntime.elementActiveOptionsState(elementIndex)
                        : null
                    readonly property var elementTimerState: {
                        sceneRoot.runtimeRevision;
                        return sceneRoot.skinRuntime
                            ? sceneRoot.skinRuntime.elementTimerState(elementIndex)
                            : null;
                    }
                    readonly property int dstTimerFire: {
                        if (!elemLoader.usesDynamicDstTimer) {
                            return elemLoader.dstTimer === 0 ? 0 : -1;
                        }
                        if (elemLoader.usesLiveGameplayDstTimer) {
                            return sceneRoot.liveGameplayRhythmTimer;
                        }
                        return elemLoader.elementTimerState
                            ? elemLoader.elementTimerState.dstTimerFire
                            : -1;
                    }
                    readonly property int srcTimerFire: {
                        if (!elemLoader.usesDynamicSrcTimer) {
                            return elemLoader.srcTimer === 0 ? 0 : -1;
                        }
                        if (elemLoader.usesLiveGameplaySrcTimer) {
                            return sceneRoot.liveGameplayRhythmTimer;
                        }
                        return elemLoader.elementTimerState
                            ? elemLoader.elementTimerState.srcTimerFire
                            : -1;
                    }
                    readonly property bool usesElementSkinTime: elementState.usesElementSkinTime
                    readonly property int manualClock: 0
                    readonly property int renderClock: 1
                    readonly property int selectSourceClock: 2
                    readonly property int selectInfoClock: 6
                    readonly property int elementSkinClockMode: elementState.elementSkinClockMode
                    readonly property bool useDirectElementSkinClock: elementState.useDirectElementSkinClock
                    readonly property bool needsManualElementSkinTime: elementState.needsManualElementSkinTime
                    readonly property int elementSkinTime: needsManualElementSkinTime
                        ? (elementSkinClockMode === selectInfoClock
                            ? root.selectInfoElapsed
                            : (usesLiveSelectClock ? root.selectSourceSkinTime : root.renderSkinTime))
                        : 0

                    Component.onCompleted: {
                        root.registerSelectHoverElement(elementIndex, elemLoader.elementData.src, usesSpriteForceHidden);
                        selectPointerController.registerElement(elementIndex, elemLoader.elementData.type, elemLoader.elementData.src, elementState.z || 0);
                    }
                    Component.onDestruction: {
                        root.unregisterSelectHoverElement(elementIndex);
                        selectPointerController.unregisterElement(elementIndex);
                    }

                    sourceComponent: {
                        if (elemLoader.elementData.type === 0) {
                            return elementState.sourceMouseCursor ? undefined : imageComponent;
                        } else if (elemLoader.elementData.type === 1) {
                            return numberComponent;
                        } else if (elemLoader.elementData.type === 2) {
                            return elemLoader.elementData.src && elemLoader.elementData.src.readme ? readmeTextComponent : textComponent;
                        } else if (elemLoader.elementData.type === 3) {
                            return barImageComponent;
                        } else if (elemLoader.elementData.type === 4) {
                            return barTextComponent;
                        } else if (elemLoader.elementData.type === 5) {
                            return barNumberComponent;
                        } else if (elemLoader.elementData.type === 6) {
                            return barGraphComponent;
                        } else if (elemLoader.elementData.type === 7) {
                            return bgaComponent;
                        } else if (elemLoader.elementData.type === 8) {
                            return playNotesComponent;
                        } else if (elemLoader.elementData.type === 9) {
                            return grooveGaugeComponent;
                        } else if (elemLoader.elementData.type === 10) {
                            return resultChartComponent;
                        } else if (elemLoader.elementData.type === 11) {
                            return noteChartComponent;
                        } else if (elemLoader.elementData.type === 12) {
                            return bpmChartComponent;
                        } else if (elemLoader.elementData.type === 13) {
                            return barDistributionGraphComponent;
                        }
                        return undefined;
                    }

                    Component {
                        id: imageComponent
                        Item {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property int manualClock: 0
                            readonly property int renderClock: 1
                            readonly property int selectSourceClock: 2
                            readonly property int selectInfoClock: 6
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            readonly property real nowJudgeOffsetX: root.nowJudgeOffsetX(elemLoader.elementData.src, elemLoader.elementData.dsts)
                            readonly property int scratchRotationSide: elementState.scratchRotationSide
                            readonly property bool useDirectSkinClock: elementState.spriteUsesDirectSkinClock
                            readonly property int spriteSkinClockMode: elementState.spriteSkinClockMode
                            readonly property int spriteSourceSkinClockMode: elementState.spriteSourceSkinClockMode
                            readonly property int selectHeldSkinClock: elemLoader.usesSelectHeldButtonTimer
                                ? (root.hasSelectHeldButtonTimers
                                    ? root.selectHeldButtonSkinTime
                                    : root.currentSelectHeldButtonSkinTime())
                                : 0
                            readonly property int spriteSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (spriteSkinClockMode === selectInfoClock
                                        ? root.selectInfoElapsed
                                        : (elemLoader.usesLiveDstClock
                                            ? root.selectSourceSkinTime
                                            : root.renderSkinTime)))
                                : 0
                            readonly property int spriteSourceSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (spriteSourceSkinClockMode === selectInfoClock
                                        ? root.selectInfoElapsed
                                        : (elemLoader.usesLiveSourceClock
                                        ? root.selectSourceSkinTime
                                        : spriteSkinClock)))
                                : 0
                            readonly property bool sliderTranslationEnabled: elemLoader.usesSpriteStateOverride
                            readonly property real sliderPosition: sliderTranslationEnabled
                                ? root.spriteSliderPositionForKind(elemLoader.spriteStateOverrideKind, elemLoader.elementData.src)
                                : 0
                            readonly property bool dstOffsetsEnabled: elementState.dstOffsetsEnabled
                            readonly property int dstOffsetSide: elementState.dstOffsetSide

                            Component.onCompleted: root.observeSelectSortButton(elemLoader.elementData.src)

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: elemLoader.elementData.dsts
                                srcData: root.imageSetSourceFor(elemLoader.elementData.src)
                                skinTime: parent.useDirectSkinClock ? 0 : parent.spriteSkinClock
                                sourceSkinTime: parent.useDirectSkinClock ? 0 : parent.spriteSourceSkinClock
                                skinClock: parent.useDirectSkinClock ? root.skinClockRef : null
                                skinClockMode: parent.spriteSkinClockMode
                                sourceSkinClockMode: parent.spriteSourceSkinClockMode
                                activeOptionsState: elemLoader.elementActiveOptionsState
                                timerFire: elemLoader.dstTimerFire
                                sourceTimerFire: elemLoader.srcTimerFire
                                chartAssetSource: sceneRoot.chartAssetSourceFor(elemLoader.elementData.src)
                                scaleOverride: skinScale
                                mediaActive: root.enabled
                                transColor: skinModel.transColor
                                colorKeyEnabled: skinModel.hasTransColor
                                screenRoot: sceneRoot.root
                                offsetX: parent.nowJudgeOffsetX
                                frameOverride: elemLoader.usesButtonFrameOverride ? root.buttonFrame(elemLoader.elementData.src) : -1
                                sliderTranslationEnabled: parent.sliderTranslationEnabled
                                sliderPosition: parent.sliderPosition
                                sliderRange: elemLoader.elementData.src ? elemLoader.elementData.src.sliderRange || 0 : 0
                                sliderDirection: elemLoader.elementData.src ? elemLoader.elementData.src.sliderDirection || 0 : 0
                                dstOffsetsEnabled: parent.dstOffsetsEnabled
                                dstOffsetLiftY: parent.dstOffsetSide === 2
                                    ? root.gameplayDstOffsetLiftY2
                                    : root.gameplayDstOffsetLiftY1
                                dstOffsetLaneCoverY: parent.dstOffsetSide === 2
                                    ? root.gameplayDstOffsetLaneCoverY2
                                    : root.gameplayDstOffsetLaneCoverY1
                                dstOffsetHiddenY: parent.dstOffsetSide === 2
                                    ? root.gameplayDstOffsetHiddenY2
                                    : root.gameplayDstOffsetHiddenY1
                                dstOffsetHiddenA: parent.dstOffsetSide === 2
                                    ? root.gameplayDstOffsetHiddenA2
                                    : root.gameplayDstOffsetHiddenA1
                                forceHidden: elemLoader.usesSpriteForceHidden
                                    ? root.spriteForceHidden(elemLoader.elementData.src, elemLoader.elementIndex)
                                    : false
                                scratchAngle1: parent.scratchRotationSide === 1 ? playContext.scratchAngle1 : 0
                                scratchAngle2: parent.scratchRotationSide === 2 ? playContext.scratchAngle2 : 0
                            }
                        }
                    }

                    Component {
                        id: bgaComponent
                        Lr2BgaRenderer {
                            dsts: elemLoader.elementData.dsts
                            skinTime: elemLoader.elementSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            chart: root.chart
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.gameplayScreenActive && root.lr2BgaEnabled()
                            poorVisible: root.gameplayPoorBgaVisible()
                        }
                    }

                    Component {
                        id: playNotesComponent
                        Lr2PlayNoteField {
                            anchors.fill: parent
                            screenRoot: sceneRoot.root
                            skinModel: sceneRoot.root.skinModelRef
                            skinScale: skinScale
                            renderSkinTime: sceneRoot.root.renderSkinTime
                            runtimeActiveOptions: sceneRoot.root.noteFieldUsesActiveOptions()
                                ? sceneRoot.root.runtimeActiveOptions
                                : sceneRoot.root.emptyActiveOptions
                            timers: null
                            transColor: sceneRoot.root.skinModelRef ? sceneRoot.root.skinModelRef.transColor : "black"
                            enabled: sceneRoot.root.enabled && sceneRoot.root.gameplayScreenActive
                        }
                    }

                    Component {
                        id: grooveGaugeComponent
                        Lr2GrooveGaugeRenderer {
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.renderSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            screenRoot: sceneRoot.root
                            scaleOverride: skinScale
                            mediaActive: sceneRoot.root.enabled && sceneRoot.root.gameplayScreenActive
                            transColor: sceneRoot.root.skinModelRef ? sceneRoot.root.skinModelRef.transColor : "black"
                        }
                    }

                    Component {
                        id: resultChartComponent
                        Lr2ResultChartRenderer {
                            anchors.fill: parent
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            scaleOverride: skinScale
                            screenRoot: sceneRoot.root
                        }
                    }

                    Component {
                        id: noteChartComponent
                        Lr2NoteChartRenderer {
                            anchors.fill: parent
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: bpmChartComponent
                        Lr2BpmChartRenderer {
                            anchors.fill: parent
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: numberComponent
                        Lr2NumberRenderer {
                            id: numberRenderer
                            readonly property var numberSrc: elemLoader.elementData.src
                            readonly property int numberId: numberSrc ? numberSrc.num || 0 : 0
                            readonly property bool numberNowCombo: numberSrc && numberSrc.nowCombo
                            readonly property int numberNowComboSide: numberNowCombo
                                ? (numberSrc.side || (numberSrc.timer === 47 ? 2 : 1))
                                : 0
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            readonly property int gameplayRevisionKind: root.gameplayNumberRevisionKind(numberSrc)
                            dsts: elemLoader.elementData.dsts
                            srcData: numberSrc
                            skinTime: elemLoader.useDirectElementSkinClock && !sourceAnimates ? 0 : elemLoader.elementSkinTime
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            scaleOverride: skinScale
                            value: {
                                if (!numberRenderer.hasCurrentState) {
                                    return 0;
                                }
                                if (root.effectiveScreenKey === "select") {
                                    let num = numberRenderer.numberId;
                                    if (!elementState.numberUsesFocusedSelectState) {
                                        return root.numberValue(numberRenderer.numberSrc, root.lr2SkinSettingsRevision);
                                    }
                                    sceneRoot.selectDetailValueRevision;
                                    if ((num >= 410 && num <= 419) || (num >= 421 && num <= 424)) {
                                        return root.numberValue(numberRenderer.numberSrc, sceneRoot.selectDetailValueRevision);
                                    }
                                    if (num >= 1312 && num <= 1327) {
                                        return root.numberValue(numberRenderer.numberSrc, sceneRoot.selectDetailValueRevision);
                                    }
                                    return selectContext.nativeState.numberValue(num);
                                }
                                if (root.gameplayScreenActive) {
                                    if (numberRenderer.numberNowCombo) {
                                        return numberRenderer.numberNowComboSide === 2
                                            ? root.gameplayJudgeCombo2
                                            : root.gameplayJudgeCombo1;
                                    }
                                    if (numberRenderer.numberId === 20) {
                                        return root.lr2CurrentFps;
                                    }
                                    if (numberRenderer.numberId === 160) {
                                        let p1 = root.gameplayPlayer(1);
                                        return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
                                    }
                                    if (numberRenderer.numberId === 161) {
                                        return Math.floor(root.gameplayTimeSeconds(1, false) / 60);
                                    }
                                    if (numberRenderer.numberId === 162) {
                                        return root.gameplayTimeSeconds(1, false) % 60;
                                    }
                                    if (numberRenderer.numberId === 163) {
                                        return Math.floor(root.gameplayTimeSeconds(1, true) / 60);
                                    }
                                    if (numberRenderer.numberId === 164) {
                                        return root.gameplayTimeSeconds(1, true) % 60;
                                    }
                                    return root.numberValue(
                                        numberRenderer.numberSrc,
                                        root.gameplayNumberRevisionForKind(numberRenderer.gameplayRevisionKind));
                                }
                                return root.numberValue(numberRenderer.numberSrc, sceneRoot.valueRevision);
                            }
                            forceHidden: root.numberForceHidden(numberSrc)
                            animationRevision: root.numberAnimationRevision(numberSrc)
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            screenRoot: sceneRoot.root
                        }
                    }

                    Component {
                        id: textComponent
                        Lr2TextElement {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            screenRoot: sceneRoot.root
                            selectContext: sceneRoot.selectContext
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            valueRevision: root.effectiveScreenKey !== "select"
                                ? sceneRoot.valueRevision
                                : (elementState.textSelectRevisionKind === sceneRoot.textFocusedRevisionKind
                                    ? sceneRoot.selectDetailValueRevision
                                    : (elementState.textSelectRevisionKind === sceneRoot.textListRevisionKind
                                        ? selectContext.listRevision + root.lr2SkinSettingsRevision
                                        : root.lr2SkinSettingsRevision))
                            selectRevisionKind: elementState.textSelectRevisionKind
                            skinScale: skinScale
                        }
                    }

                    Component {
                        id: readmeTextComponent
                        Lr2ReadmeTextElement {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            screenRoot: sceneRoot.root
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.renderSkinTime
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            skinScale: skinScale
                        }
                    }

                    Component {
                        id: barImageComponent
                        Lr2BarSpriteRenderer {
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.barSkinTime
                            sourceSkinTime: root.effectiveScreenKey === "select"
                                ? (sourceAnimates ? root.selectSourceSkinTime : 0)
                                : (sourceAnimates ? root.renderSkinTime : 0)
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: sceneRoot.root.selectContextRef
                            barRows: skinModel.barRows
                            barLampVariants: skinModel.barLampVariants
                            barBaseStates: root.cachedBarBaseStates
                            barPositionCache: root.cachedBarPositionCache
                            barCells: selectContext.visibleBarTextCells
                            barTextCells: selectContext.visibleBarTextCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                            stageFileSource: sceneRoot.sourceTreeUsesChartAsset(elemLoader.elementData.src, 0) ? sceneRoot.stageFileSource : ""
                            backBmpSource: sceneRoot.sourceTreeUsesChartAsset(elemLoader.elementData.src, 0) ? sceneRoot.backBmpSource : ""
                            bannerSource: sceneRoot.sourceTreeUsesChartAsset(elemLoader.elementData.src, 0) ? sceneRoot.bannerSource : ""
                            transColor: skinModel.transColor
                            colorKeyEnabled: skinModel.hasTransColor
                        }
                    }

                    Component {
                        id: barTextComponent
                        Lr2BarTextRenderer {
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: sceneRoot.root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barPositionCache: root.cachedBarPositionCache
                            barCells: selectContext.visibleBarTextCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barNumberComponent
                        Lr2BarNumberRenderer {
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: sceneRoot.root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barPositionCache: root.cachedBarPositionCache
                            barCells: selectContext.visibleBarTextCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                        }
                    }

                    Component {
                        id: barGraphComponent
                        Lr2BarGraphRenderer {
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
                            sourceSkinTime: elemLoader.useDirectElementSkinClock
                                ? 0
                                : (root.effectiveScreenKey === "select" && sourceAnimates
                                    ? root.selectSourceSkinTime
                                    : (sourceAnimates ? root.renderSkinTime : 0))
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            sourceSkinClockMode: elemLoader.useDirectElementSkinClock && sourceAnimates
                                ? (root.effectiveScreenKey === "select"
                                    ? elemLoader.selectSourceClock
                                    : elemLoader.renderClock)
                                : elemLoader.manualClock
                            activeOptionsState: elemLoader.elementActiveOptionsState
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            chartAssetSource: sceneRoot.chartAssetSourceFor(elemLoader.elementData.src)
                            scaleOverride: skinScale
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            value: {
                                sceneRoot.barGraphValueRevision(elemLoader.elementData.src);
                                let graphType = elemLoader.elementData.src ? elemLoader.elementData.src.graphType || 0 : 0;
                                return root.effectiveScreenKey === "select"
                                    ? selectContext.nativeBarGraphValue(graphType)
                                    : root.resolveBarGraph(graphType);
                            }
                            animateValue: root.effectiveScreenKey === "select"
                                && elemLoader.elementData.src
                                && elemLoader.elementData.src.graphType >= 5
                                && elemLoader.elementData.src.graphType <= 9
                        }
                    }

                    Component {
                        id: barDistributionGraphComponent
                        Lr2BarDistributionGraphRenderer {
                            readonly property bool barDistributionSourceAnimates: root.barDistributionGraphSourceAnimates(elemLoader.elementData.src)
                            dsts: elemLoader.elementData.dsts
                            srcData: elemLoader.elementData.src
                            skinTime: root.barSkinTime
                            sourceSkinTime: barDistributionSourceAnimates
                                ? (root.effectiveScreenKey === "select"
                                    ? root.selectSourceSkinTime
                                    : root.renderSkinTime)
                                : 0
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            scaleOverride: skinScale
                            selectContext: sceneRoot.root.selectContextRef
                            barRows: skinModel.barRows
                            barPositionCache: root.cachedBarPositionCache
                            barCells: selectContext.visibleBarTextCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            chartAssetSource: sceneRoot.chartAssetSourceFor(elemLoader.elementData.src)
                        }
                    }
                }
            }

            Text {
                visible: sceneRoot.selectScreenActive
                    && root.clearStatusIsBest()
                text: "BEST"
                x: 80 * skinScale
                y: 459 * skinScale
                z: 100200
                color: "white"
                font.family: "Arial"
                font.pixelSize: 6 * skinScale
                font.bold: true
                renderType: Text.NativeRendering
            }

            Lr2SkinCursor {
                anchors.fill: parent
                screenRoot: sceneRoot.root
                skinModel: skinModel
            }

            Lr2ReadmePointerArea {
                anchors.fill: parent
                screenRoot: sceneRoot.root
                skinItem: skinContainer
                skinScale: skinScale
            }
        }
    }
}
