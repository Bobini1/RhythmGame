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
    readonly property bool screenUpdatesActive: rootReady && root.screenUpdatesActive
    readonly property bool selectScreenActive: screenUpdatesActive && root.effectiveScreenKey === "select"
    readonly property int valueRevision: !rootReady ? 0
        : (root.effectiveScreenKey === "select"
            ? root.selectRevision
                + selectContext.listRevision
                + selectContext.folderLampRevision
                + root.lr2SkinSettingsRevision
            : (root.isResultScreen()
                ? root.resultOldScoresRevision
                : root.gameplayRevision))
    readonly property int selectDetailValueRevision: !rootReady ? 0
        : root.selectRevision
            + selectContext.listRevision
            + selectContext.folderLampRevision
            + root.lr2SkinSettingsRevision
    readonly property int selectStableBarGraphValueRevision: !rootReady ? 0
        : root.lr2SkinSettingsRevision

    function selectNumberUsesFocusedState(num) {
        return num === 42 || num === 96
            || (num >= 45 && num <= 49)
            || (num >= 70 && num <= 116)
            || num === 128
            || num === 150 || num === 152 || num === 154
            || (num >= 179 && num <= 182)
            || (num >= 200 && num <= 242)
            || num === 290 || num === 291
            || num === 300 || (num >= 320 && num <= 330)
            || (num >= 350 && num <= 368)
            || (num >= 410 && num <= 427)
            || num === 1163 || num === 1164
            || (num >= 1312 && num <= 1327);
    }

    function numberValueRevision(src) {
        if (!rootReady) {
            return 0;
        }
        if (root.effectiveScreenKey !== "select") {
            return valueRevision;
        }
        let num = src ? (src.num || 0) : 0;
        return selectNumberUsesFocusedState(num)
            ? selectDetailValueRevision
            : root.lr2SkinSettingsRevision;
    }

    function selectTextUsesFocusedState(st) {
        return (st >= 10 && st <= 29) && st !== 26;
    }

    function textValueRevision(src) {
        if (!rootReady) {
            return 0;
        }
        if (root.effectiveScreenKey !== "select") {
            return valueRevision;
        }
        let st = src ? (src.st || 0) : 0;
        if (selectTextUsesFocusedState(st)) {
            return selectDetailValueRevision;
        }
        if (st === 30 || st === 60 || st === 61 || st === 62
                || (st >= 1000 && st <= 1003)) {
            return selectContext.listRevision + root.lr2SkinSettingsRevision;
        }
        return root.lr2SkinSettingsRevision;
    }

    function selectBarGraphUsesFocusedState(type) {
        return type === 101
            || (type >= 5 && type <= 9)
            || (type >= 40 && type <= 47)
            || (type >= 103 && type <= 115)
            || (type >= 140 && type <= 147);
    }

    function barGraphValueRevision(src) {
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

    function sourceUsesChartAsset(src) {
        if (!src) {
            return false;
        }
        return src.specialType === 1 || src.specialType === 3 || src.specialType === 4;
    }

    function sourceTreeUsesChartAsset(value, depth) {
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

    function hoverPointInSkinCoordinates() {
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

                    readonly property var elementState: {
                        sceneRoot.runtimeRevision;
                        return sceneRoot.skinRuntime
                            ? sceneRoot.skinRuntime.descriptor(index)
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
                    readonly property int spriteStateOverrideKind: elementState.spriteStateOverrideKind
                    readonly property bool usesSpriteStateOverride: elementState.usesSpriteStateOverride
                    readonly property bool usesSpriteForceHidden: elementState.usesSpriteForceHidden
                    readonly property bool usesButtonFrameOverride: elementState.usesButtonFrameOverride
                    readonly property var elementActiveOptionsState: sceneRoot.skinRuntime
                        ? sceneRoot.skinRuntime.elementActiveOptionsState(index)
                        : null
                    readonly property var elementTimerState: {
                        sceneRoot.runtimeRevision;
                        return sceneRoot.skinRuntime
                            ? sceneRoot.skinRuntime.elementTimerState(index)
                            : null;
                    }
                    readonly property var elementActiveOptions: elemLoader.usesActiveOptions
                        && elemLoader.elementActiveOptionsState
                        ? elemLoader.elementActiveOptionsState.activeOptions
                        : root.emptyActiveOptions
                    readonly property int dstTimerFire: {
                        if (!elemLoader.usesDynamicDstTimer) {
                            return elemLoader.dstTimer === 0 ? 0 : -1;
                        }
                        return elemLoader.elementTimerState
                            ? elemLoader.elementTimerState.dstTimerFire
                            : -1;
                    }
                    readonly property int srcTimerFire: {
                        if (!elemLoader.usesDynamicSrcTimer) {
                            return elemLoader.srcTimer === 0 ? 0 : -1;
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
                        root.registerSelectHoverElement(index, model.src, usesSpriteForceHidden);
                        selectPointerController.registerElement(index, model.type, model.src, z);
                    }
                    Component.onDestruction: {
                        root.unregisterSelectHoverElement(index);
                        selectPointerController.unregisterElement(index);
                    }

                    sourceComponent: {
                        if (model.type === 0) {
                            return elementState.sourceMouseCursor ? undefined : imageComponent;
                        } else if (model.type === 1) {
                            return numberComponent;
                        } else if (model.type === 2) {
                            return model.src && model.src.readme ? readmeTextComponent : textComponent;
                        } else if (model.type === 3) {
                            return barImageComponent;
                        } else if (model.type === 4) {
                            return barTextComponent;
                        } else if (model.type === 5) {
                            return barNumberComponent;
                        } else if (model.type === 6) {
                            return barGraphComponent;
                        } else if (model.type === 7) {
                            return bgaComponent;
                        } else if (model.type === 8) {
                            return playNotesComponent;
                        } else if (model.type === 9) {
                            return grooveGaugeComponent;
                        } else if (model.type === 10) {
                            return resultChartComponent;
                        } else if (model.type === 11) {
                            return noteChartComponent;
                        } else if (model.type === 12) {
                            return bpmChartComponent;
                        } else if (model.type === 13) {
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
                            readonly property real nowJudgeOffsetX: root.nowJudgeOffsetX(model.src, model.dsts)
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
                                ? root.spriteSliderPositionForKind(elemLoader.spriteStateOverrideKind, model.src)
                                : 0
                            readonly property bool dstOffsetsEnabled: elementState.dstOffsetsEnabled
                            readonly property int dstOffsetSide: elementState.dstOffsetSide

                            Component.onCompleted: root.observeSelectSortButton(model.src)

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: root.imageSetSourceFor(model.src)
                                skinTime: parent.useDirectSkinClock ? 0 : parent.spriteSkinClock
                                sourceSkinTime: parent.useDirectSkinClock ? 0 : parent.spriteSourceSkinClock
                                skinClock: parent.useDirectSkinClock ? root.skinClockRef : null
                                skinClockMode: parent.spriteSkinClockMode
                                sourceSkinClockMode: parent.spriteSourceSkinClockMode
                                activeOptions: elemLoader.elementActiveOptions
                                timerFire: elemLoader.dstTimerFire
                                sourceTimerFire: elemLoader.srcTimerFire
                                chart: sceneRoot.sourceUsesChartAsset(model.src) ? sceneRoot.renderChart : null
                                scaleOverride: skinScale
                                mediaActive: root.enabled
                                transColor: skinModel.transColor
                                colorKeyEnabled: skinModel.hasTransColor
                                screenRoot: sceneRoot.root
                                offsetX: parent.nowJudgeOffsetX
                                frameOverride: elemLoader.usesButtonFrameOverride ? root.buttonFrame(model.src) : -1
                                sliderTranslationEnabled: parent.sliderTranslationEnabled
                                sliderPosition: parent.sliderPosition
                                sliderRange: model.src ? model.src.sliderRange || 0 : 0
                                sliderDirection: model.src ? model.src.sliderDirection || 0 : 0
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
                                    ? root.spriteForceHidden(model.src, index)
                                    : false
                                scratchAngle1: parent.scratchRotationSide === 1 ? playContext.scratchAngle1 : 0
                                scratchAngle2: parent.scratchRotationSide === 2 ? playContext.scratchAngle2 : 0
                            }
                        }
                    }

                    Component {
                        id: bgaComponent
                        Lr2BgaRenderer {
                            dsts: model.dsts
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            chart: root.chart
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.isGameplayScreen() && root.lr2BgaEnabled()
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
                            enabled: sceneRoot.root.enabled && sceneRoot.root.isGameplayScreen()
                        }
                    }

                    Component {
                        id: grooveGaugeComponent
                        Lr2GrooveGaugeRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            screenRoot: sceneRoot.root
                            scaleOverride: skinScale
                            mediaActive: sceneRoot.root.enabled && sceneRoot.root.isGameplayScreen()
                            transColor: sceneRoot.root.skinModelRef ? sceneRoot.root.skinModelRef.transColor : "black"
                        }
                    }

                    Component {
                        id: resultChartComponent
                        Lr2ResultChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
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
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: bpmChartComponent
                        Lr2BpmChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: numberComponent
                        Lr2NumberRenderer {
                            id: numberRenderer
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.useDirectElementSkinClock && !sourceAnimates ? 0 : elemLoader.elementSkinTime
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            scaleOverride: skinScale
                            value: numberRenderer.hasCurrentState
                                ? root.numberValue(model.src, sceneRoot.numberValueRevision(model.src))
                                : 0
                            forceHidden: root.numberForceHidden(model.src)
                            animationRevision: root.numberAnimationRevision(model.src)
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
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            valueRevision: sceneRoot.textValueRevision(model.src)
                            skinScale: skinScale
                        }
                    }

                    Component {
                        id: readmeTextComponent
                        Lr2ReadmeTextElement {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            screenRoot: sceneRoot.root
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            chart: sceneRoot.renderChart
                            skinScale: skinScale
                        }
                    }

                    Component {
                        id: barImageComponent
                        Lr2BarSpriteRenderer {
                            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
                            dsts: model.dsts
                            srcData: model.src
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
                            chart: sceneRoot.sourceTreeUsesChartAsset(model.src, 0) ? sceneRoot.renderChart : null
                            transColor: skinModel.transColor
                            colorKeyEnabled: skinModel.hasTransColor
                        }
                    }

                    Component {
                        id: barTextComponent
                        Lr2BarTextRenderer {
                            dsts: model.dsts
                            srcData: model.src
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
                            dsts: model.dsts
                            srcData: model.src
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
                            dsts: model.dsts
                            srcData: model.src
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
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            chart: sceneRoot.sourceUsesChartAsset(model.src) ? sceneRoot.renderChart : null
                            scaleOverride: skinScale
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            value: {
                                sceneRoot.barGraphValueRevision(model.src);
                                let graphType = model.src ? model.src.graphType || 0 : 0;
                                return root.effectiveScreenKey === "select"
                                    ? selectContext.nativeBarGraphValue(graphType)
                                    : root.resolveBarGraph(graphType);
                            }
                            animateValue: root.effectiveScreenKey === "select"
                                && model.src
                                && model.src.graphType >= 5
                                && model.src.graphType <= 9
                        }
                    }

                    Component {
                        id: barDistributionGraphComponent
                        Lr2BarDistributionGraphRenderer {
                            readonly property bool barDistributionSourceAnimates: root.barDistributionGraphSourceAnimates(model.src)
                            dsts: model.dsts
                            srcData: model.src
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
                            chart: sceneRoot.renderChart
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
