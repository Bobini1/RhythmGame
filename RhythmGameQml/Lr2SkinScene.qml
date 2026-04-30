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
    readonly property var renderChart: rootReady ? root.renderChart : null
    readonly property bool screenUpdatesActive: rootReady && root.screenUpdatesActive
    readonly property bool selectScreenActive: screenUpdatesActive && root.effectiveScreenKey === "select"

    function hoverPointInSkinCoordinates() {
        return rootReady ? root.selectHoverPointInSkinCoordinates() : Qt.point(0, 0);
    }

    Lr2SelectPointerController {
        id: selectPointerController
        screenRoot: sceneRoot.root
        selectContext: sceneRoot.selectContext
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
                    root.selectGoForward(selectContext.current);
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
                    z: root.elementZ(model.type, index, model.src, model.dsts)
                    readonly property bool usesActiveOptions: root.dstsUseActiveOptions(model.dsts)
                    readonly property bool usesSkinTime: root.elementUsesSkinTime(model.src, model.dsts)
                    readonly property int dstTimer: model.dsts && model.dsts.length > 0
                        ? (model.dsts[0].timer || 0)
                        : 0
                    readonly property int srcTimer: model.src ? (model.src.timer || 0) : 0
                    readonly property bool usesSelectHeldButtonTimer: root.isSelectHeldButtonTimer(dstTimer)
                    readonly property bool usesLiveDstClock: root.elementUsesLiveDstClock(model.dsts)
                    readonly property bool usesLiveSourceClock: root.elementUsesLiveSourceClock(model.src)
                    readonly property bool usesLiveSelectClock: usesLiveDstClock || usesLiveSourceClock
                    readonly property bool usesSpriteStateOverride: root.elementUsesSpriteStateOverride(model.src)
                    readonly property bool usesSpriteForceHidden: root.elementUsesSpriteForceHidden(model.src)
                    readonly property bool usesButtonFrameOverride: root.elementUsesButtonFrameOverride(model.src)
                    readonly property var elementActiveOptions: usesActiveOptions
                        ? root.runtimeActiveOptions
                        : root.emptyActiveOptions
                    readonly property int dstTimerFire: dstTimer !== 0
                        ? root.skinTimerFireTime(dstTimer)
                        : 0
                    readonly property int srcTimerFire: srcTimer !== 0
                        ? root.skinTimerFireTime(srcTimer)
                        : 0
                    readonly property bool usesElementSkinTime: usesSkinTime
                        && model.type !== 0
                        && model.type !== 3
                        && model.type !== 4
                        && model.type !== 5
                        && model.type !== 8
                        && model.type !== 9
                    readonly property int manualClock: 0
                    readonly property int renderClock: 1
                    readonly property int selectSourceClock: 2
                    readonly property int elementSkinClockMode: usesElementSkinTime
                        ? (usesLiveSelectClock ? selectSourceClock : renderClock)
                        : manualClock
                    readonly property bool useDirectElementSkinClock: usesElementSkinTime
                    readonly property int elementSkinTime: usesElementSkinTime
                        ? (usesLiveSelectClock ? root.selectSourceSkinTime : root.renderSkinTime)
                        : 0

                    Component.onCompleted: {
                        root.registerSelectHoverElement(index, model.src, model.dsts, usesSpriteForceHidden);
                        selectPointerController.registerElement(index, model.type, model.src, model.dsts, z);
                    }
                    Component.onDestruction: {
                        root.unregisterSelectHoverElement(index);
                        selectPointerController.unregisterElement(index);
                    }

                    sourceComponent: {
                        if (model.type === 0) {
                            return model.src && model.src.mouseCursor ? undefined : imageComponent;
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
                            readonly property bool useDirectSkinClock: elemLoader.usesSkinTime
                                && !elemLoader.usesSelectHeldButtonTimer
                                && !elemLoader.usesSpriteStateOverride
                            readonly property int spriteSkinClockMode: useDirectSkinClock
                                ? (elemLoader.usesLiveDstClock ? selectSourceClock : renderClock)
                                : manualClock
                            readonly property int spriteSourceSkinClockMode: useDirectSkinClock && elemLoader.usesLiveSourceClock
                                ? selectSourceClock
                                : manualClock
                            readonly property int selectHeldSkinClock: elemLoader.usesSelectHeldButtonTimer
                                ? (root.hasSelectHeldButtonTimers
                                    ? root.selectHeldButtonSkinTime
                                    : root.currentSelectHeldButtonSkinTime())
                                : 0
                            readonly property int spriteSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (elemLoader.usesLiveDstClock
                                        ? root.selectSourceSkinTime
                                        : root.renderSkinTime))
                                : 0
                            readonly property int spriteSourceSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (elemLoader.usesLiveSourceClock
                                        ? root.selectSourceSkinTime
                                        : spriteSkinClock))
                                : 0

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
                                chart: sceneRoot.renderChart
                                scaleOverride: skinScale
                                mediaActive: root.enabled
                                transColor: skinModel.transColor
                                colorKeyEnabled: skinModel.hasTransColor
                                frameOverride: elemLoader.usesButtonFrameOverride ? root.buttonFrame(model.src) : -1
                                stateOverride: elemLoader.usesSpriteStateOverride
                                    ? root.spriteStateOverride(model.src, model.dsts, parent.spriteSkinClock)
                                    : null
                                forceHidden: elemLoader.usesSpriteForceHidden
                                    ? root.spriteForceHidden(model.src, index)
                                    : false
                                scratchAngle1: playContext.scratchAngle1
                                scratchAngle2: playContext.scratchAngle2
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
                            timers: sceneRoot.root.noteFieldUsesTimers()
                                ? sceneRoot.root.timers
                                : sceneRoot.root.zeroTimers
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
                            readonly property bool sourceAnimates: root.sourceHasFrameAnimation(model.src)
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.useDirectElementSkinClock && !sourceAnimates ? 0 : elemLoader.elementSkinTime
                            skinClock: elemLoader.useDirectElementSkinClock ? root.skinClockRef : null
                            skinClockMode: elemLoader.elementSkinClockMode
                            activeOptions: elemLoader.elementActiveOptions
                            timerFire: elemLoader.dstTimerFire
                            sourceTimerFire: elemLoader.srcTimerFire
                            scaleOverride: skinScale
                            value: root.numberValue(model.src)
                            forceHidden: root.numberForceHidden(model.src)
                            animationRevision: root.numberAnimationRevision(model.src)
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
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
                            chart: sceneRoot.renderChart
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
                            readonly property bool sourceAnimates: root.sourceHasFrameAnimation(model.src)
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            sourceSkinTime: root.effectiveScreenKey === "select"
                                ? (sourceAnimates ? root.selectSourceSkinTime : 0)
                                : (sourceAnimates ? root.renderSkinTime : 0)
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            chart: sceneRoot.renderChart
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
                            readonly property bool sourceAnimates: root.sourceHasFrameAnimation(model.src)
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
                            chart: sceneRoot.renderChart
                            scaleOverride: skinScale
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            value: root.resolveBarGraph(model.src ? model.src.graphType : 0)
                            animateValue: root.effectiveScreenKey === "select"
                                && model.src
                                && model.src.graphType >= 5
                                && model.src.graphType <= 9
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
