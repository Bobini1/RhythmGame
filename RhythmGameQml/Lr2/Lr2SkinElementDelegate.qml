pragma ValueTypeBehavior: Addressable
pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Loader {
    id: elemLoader
    x: 0; y: 0
    width: skinW * skinScale
    height: skinH * skinScale
    z: elementState.z || 0
    required property var screenRoot
    required property var skinModel
    required property var selectContext
    required property var playContext
    required property var pointerController
    required property var skinRuntime
    required property int runtimeRevision
    required property int liveGameplayRhythmTimer
    required property bool screenUpdatesActive
    required property string stageFileSource
    required property string backBmpSource
    required property string bannerSource
    required property real skinW
    required property real skinH
    required property real skinScale
    required property int index
    required property int type
    required property var src
    required property var dsts

    readonly property int elementIndex: index
    readonly property var elementData: ({ type: type, src: src, dsts: dsts || [] })
    readonly property bool elementSourceTreeAnimates: Lr2SkinUtils.sourceTreeCyclesContinuously(src)
    readonly property bool elementSourceTreeUsesChartAsset: Lr2SkinUtils.sourceTreeUsesChartAsset(src)

    readonly property var elementState: {
        elemLoader.runtimeRevision;
        return elemLoader.skinRuntime
            ? elemLoader.skinRuntime.descriptor(elementIndex)
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
    readonly property bool usesLiveGameplayDstTimer: !!elemLoader.screenRoot
        && elemLoader.screenRoot.gameplayScreenActive
        && elemLoader.dstTimer === 140
    readonly property bool usesLiveGameplaySrcTimer: !!elemLoader.screenRoot
        && elemLoader.screenRoot.gameplayScreenActive
        && elemLoader.srcTimer === 140
    readonly property int spriteStateOverrideKind: elementState.spriteStateOverrideKind
    readonly property bool usesSpriteStateOverride: elementState.usesSpriteStateOverride
    readonly property bool usesSpriteForceHidden: elementState.usesSpriteForceHidden
    readonly property bool usesButtonFrameOverride: elementState.usesButtonFrameOverride
    readonly property var elementActiveOptionsState: elemLoader.skinRuntime
        ? elemLoader.skinRuntime.elementActiveOptionsState(elementIndex)
        : null
    readonly property var elementTimerState: {
        elemLoader.runtimeRevision;
        return elemLoader.skinRuntime
            ? elemLoader.skinRuntime.elementTimerState(elementIndex)
            : null;
    }
    readonly property int dstTimerFire: {
        if (!elemLoader.usesDynamicDstTimer) {
            return elemLoader.dstTimer === 0 ? 0 : -1;
        }
        if (elemLoader.usesLiveGameplayDstTimer) {
            return elemLoader.liveGameplayRhythmTimer;
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
            return elemLoader.liveGameplayRhythmTimer;
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
    readonly property int selectHeldButtonSkinClock: usesSelectHeldButtonTimer
        ? (elemLoader.screenRoot.hasSelectHeldButtonTimers
            ? elemLoader.screenRoot.selectHeldButtonSkinTime
            : elemLoader.screenRoot.currentSelectHeldButtonSkinTime())
        : 0
    readonly property int elementSkinTime: needsManualElementSkinTime
        ? skinTimeForClock(elementSkinClockMode, usesLiveSelectClock, elemLoader.screenRoot.renderSkinTime, false)
        : 0

    function skinTimeForClock(clockMode: var, liveClockEnabled: var, fallbackSkinTime: var, useHeldButtonTimer: var) : var {
        if (useHeldButtonTimer && usesSelectHeldButtonTimer) {
            return selectHeldButtonSkinClock;
        }
        if (clockMode === selectInfoClock) {
            return elemLoader.screenRoot.selectInfoElapsed;
        }
        return liveClockEnabled ? elemLoader.screenRoot.selectSourceSkinTime : fallbackSkinTime;
    }

    function sourceClockMode(sourceAnimates: var) : var {
        if (!sourceAnimates) {
            return manualClock;
        }
        return elemLoader.screenRoot.effectiveScreenKey === "select"
            ? selectSourceClock
            : renderClock;
    }

    function sourceSkinTimeForAnimation(sourceAnimates: var) : var {
        if (!sourceAnimates) {
            return 0;
        }
        return elemLoader.screenRoot.effectiveScreenKey === "select"
            ? elemLoader.screenRoot.selectSourceSkinTime
            : elemLoader.screenRoot.renderSkinTime;
    }

    function chartAssetSourceFor(source: var) : var {
        return Lr2SkinUtils.chartAssetSourceFor(
            source,
            stageFileSource,
            backBmpSource,
            bannerSource);
    }

    function componentForElement() : var {
        switch (elemLoader.elementData.type) {
        case 0:
            return elementState.sourceMouseCursor ? undefined : imageComponent;
        case 1:
            return numberComponent;
        case 2:
            return elemLoader.elementData.src && elemLoader.elementData.src.readme
                ? readmeTextComponent
                : textComponent;
        case 3:
            return barImageComponent;
        case 4:
            return barTextComponent;
        case 5:
            return barNumberComponent;
        case 6:
            return barGraphComponent;
        case 7:
            return bgaComponent;
        case 8:
            return playNotesComponent;
        case 9:
            return grooveGaugeComponent;
        case 10:
            return resultChartComponent;
        case 11:
            return noteChartComponent;
        case 12:
            return bpmChartComponent;
        case 13:
            return barDistributionGraphComponent;
        default:
            return undefined;
        }
    }

    Component.onCompleted: {
        elemLoader.screenRoot.registerSelectHoverElement(elementIndex, elemLoader.elementData.src, usesSpriteForceHidden);
        elemLoader.screenRoot.registerSelectButtonSource(elementIndex, elemLoader.elementData.src);
        pointerController.registerElement(elementIndex, elemLoader.elementData.type, elemLoader.elementData.src, elementState.z || 0);
    }
    Component.onDestruction: {
        elemLoader.screenRoot.unregisterSelectHoverElement(elementIndex);
        elemLoader.screenRoot.unregisterSelectButtonSource(elementIndex);
        pointerController.unregisterElement(elementIndex);
    }

    sourceComponent: componentForElement()

    Component {
        id: imageComponent
        Item {
            id: imageComponentRoot
            width: skinW * skinScale
            height: skinH * skinScale
            readonly property bool sourceAnimates: elementState.sourceHasFrameAnimation
            readonly property real nowJudgeOffsetX: elemLoader.screenRoot.nowJudgeOffsetX(elemLoader.elementData.src, elemLoader.elementData.dsts)
            readonly property int scratchRotationSide: elementState.scratchRotationSide
            readonly property bool useDirectSkinClock: elementState.spriteUsesDirectSkinClock
            readonly property int spriteSkinClockMode: elementState.spriteSkinClockMode
            readonly property int spriteSourceSkinClockMode: elementState.spriteSourceSkinClockMode
            readonly property int spriteSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                ? elemLoader.skinTimeForClock(
                    spriteSkinClockMode,
                    elemLoader.usesLiveDstClock,
                    elemLoader.screenRoot.renderSkinTime,
                    true)
                : 0
            readonly property int spriteSourceSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                ? elemLoader.skinTimeForClock(
                    spriteSourceSkinClockMode,
                    elemLoader.usesLiveSourceClock,
                    spriteSkinClock,
                    true)
                : 0
            readonly property bool sliderTranslationEnabled: elemLoader.usesSpriteStateOverride
            readonly property real sliderPosition: sliderTranslationEnabled
                ? elemLoader.screenRoot.spriteSliderPositionForKind(elemLoader.spriteStateOverrideKind, elemLoader.elementData.src)
                : 0
            readonly property bool dstOffsetsEnabled: elementState.dstOffsetsEnabled
            readonly property int dstOffsetSide: elementState.dstOffsetSide
            readonly property int buttonFrameOverrideValue: elemLoader.usesButtonFrameOverride
                ? elemLoader.screenRoot.buttonFrame(elemLoader.elementData.src)
                : -1

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: elemLoader.elementData.dsts
                srcData: elemLoader.screenRoot.imageSetSourceFor(elemLoader.elementData.src)
                skinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSkinClock
                sourceSkinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSourceSkinClock
                skinClock: imageComponentRoot.useDirectSkinClock ? elemLoader.screenRoot.skinClockRef : null
                skinClockMode: imageComponentRoot.spriteSkinClockMode
                sourceSkinClockMode: imageComponentRoot.spriteSourceSkinClockMode
                activeOptionsState: elemLoader.elementActiveOptionsState
                timerFire: elemLoader.dstTimerFire
                sourceTimerFire: elemLoader.srcTimerFire
                chartAssetSource: elemLoader.chartAssetSourceFor(elemLoader.elementData.src)
                scaleOverride: skinScale
                mediaActive: elemLoader.screenRoot.enabled && elemLoader.screenUpdatesActive
                transColor: skinModel.transColor
                colorKeyEnabled: skinModel.hasTransColor
                screenRoot: elemLoader.screenRoot
                offsetX: imageComponentRoot.nowJudgeOffsetX
                frameOverride: imageComponentRoot.buttonFrameOverrideValue >= 0
                    ? imageComponentRoot.buttonFrameOverrideValue
                    : -1
                sliderTranslationEnabled: imageComponentRoot.sliderTranslationEnabled
                sliderPosition: imageComponentRoot.sliderPosition
                sliderRange: elemLoader.elementData.src ? elemLoader.elementData.src.sliderRange || 0 : 0
                sliderDirection: elemLoader.elementData.src ? elemLoader.elementData.src.sliderDirection || 0 : 0
                dstOffsetsEnabled: imageComponentRoot.dstOffsetsEnabled
                dstOffsetLiftY: imageComponentRoot.dstOffsetSide === 2
                    ? elemLoader.screenRoot.gameplayDstOffsetLiftY2
                    : elemLoader.screenRoot.gameplayDstOffsetLiftY1
                dstOffsetLaneCoverY: imageComponentRoot.dstOffsetSide === 2
                    ? elemLoader.screenRoot.gameplayDstOffsetLaneCoverY2
                    : elemLoader.screenRoot.gameplayDstOffsetLaneCoverY1
                dstOffsetHiddenY: imageComponentRoot.dstOffsetSide === 2
                    ? elemLoader.screenRoot.gameplayDstOffsetHiddenY2
                    : elemLoader.screenRoot.gameplayDstOffsetHiddenY1
                dstOffsetHiddenA: imageComponentRoot.dstOffsetSide === 2
                    ? elemLoader.screenRoot.gameplayDstOffsetHiddenA2
                    : elemLoader.screenRoot.gameplayDstOffsetHiddenA1
                forceHidden: (elemLoader.usesSpriteForceHidden
                        ? elemLoader.screenRoot.spriteForceHidden(elemLoader.elementData.src, elemLoader.elementIndex)
                        : false)
                    || imageComponentRoot.buttonFrameOverrideValue < -1
                scratchAngle1: imageComponentRoot.scratchRotationSide === 1 ? playContext.scratchAngle1 : 0
                scratchAngle2: imageComponentRoot.scratchRotationSide === 2 ? playContext.scratchAngle2 : 0
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
            chart: elemLoader.screenRoot.chart
            scaleOverride: skinScale
            mediaActive: elemLoader.screenRoot.enabled && elemLoader.screenRoot.gameplayScreenActive && elemLoader.screenRoot.lr2BgaEnabled()
            poorVisible: elemLoader.screenRoot.gameplayPoorBgaVisible()
        }
    }

    Component {
        id: playNotesComponent
        Lr2PlayNoteField {
            anchors.fill: parent
            screenRoot: elemLoader.screenRoot
            skinModel: elemLoader.screenRoot.skinModelRef
            skinScale: skinScale
            renderSkinTime: elemLoader.screenRoot.renderSkinTime
            runtimeActiveOptions: elemLoader.screenRoot.noteFieldUsesActiveOptions()
                ? elemLoader.screenRoot.runtimeActiveOptions
                : elemLoader.screenRoot.emptyActiveOptions
            timers: null
            transColor: elemLoader.screenRoot.skinModelRef ? elemLoader.screenRoot.skinModelRef.transColor : "black"
            enabled: elemLoader.screenRoot.enabled && elemLoader.screenRoot.gameplayScreenActive
        }
    }

    Component {
        id: grooveGaugeComponent
        Lr2GrooveGaugeRenderer {
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.renderSkinTime
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            screenRoot: elemLoader.screenRoot
            scaleOverride: skinScale
            mediaActive: elemLoader.screenRoot.enabled && elemLoader.screenRoot.gameplayScreenActive
            transColor: elemLoader.screenRoot.skinModelRef ? elemLoader.screenRoot.skinModelRef.transColor : "black"
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
            screenRoot: elemLoader.screenRoot
        }
    }

    Component {
        id: noteChartComponent
        Lr2NoteChartRenderer {
            anchors.fill: parent
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            scaleOverride: skinScale
            chart: elemLoader.screenRoot.visualSelectChart
            chartRevision: elemLoader.screenRoot.selectChartContentRevision
        }
    }

    Component {
        id: bpmChartComponent
        Lr2BpmChartRenderer {
            anchors.fill: parent
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            scaleOverride: skinScale
            chart: elemLoader.screenRoot.visualSelectChart
            chartRevision: elemLoader.screenRoot.selectChartContentRevision
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
            readonly property bool numberSourceAnimates: elementState.sourceHasFrameAnimation
            dsts: elemLoader.elementData.dsts
            srcData: numberSrc
            skinTime: elemLoader.useDirectElementSkinClock && !numberSourceAnimates ? 0 : elemLoader.elementSkinTime
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            scaleOverride: skinScale
            value: {
                if (!numberRenderer.hasCurrentState) {
                    return 0;
                }
                if (elemLoader.screenRoot.effectiveScreenKey === "select") {
                    let num = numberRenderer.numberId;
                    if (!elementState.numberUsesFocusedSelectState) {
                        return elemLoader.screenRoot.numberValue(numberRenderer.numberSrc);
                    }
                    if ((num >= 410 && num <= 419) || (num >= 421 && num <= 424)) {
                        return elemLoader.screenRoot.numberValue(numberRenderer.numberSrc);
                    }
                    if (num >= 1312 && num <= 1327) {
                        return elemLoader.screenRoot.numberValue(numberRenderer.numberSrc);
                    }
                    return selectContext.numberValue(num);
                }
                if (elemLoader.screenRoot.gameplayScreenActive) {
                    if (numberRenderer.numberNowCombo) {
                        return numberRenderer.numberNowComboSide === 2
                            ? elemLoader.screenRoot.gameplayJudgeCombo2
                            : elemLoader.screenRoot.gameplayJudgeCombo1;
                    }
                    if (numberRenderer.numberId === 20) {
                        return elemLoader.screenRoot.lr2CurrentFps;
                    }
                    if (numberRenderer.numberId === 160) {
                        let p1 = elemLoader.screenRoot.gameplayPlayer(1);
                        return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
                    }
                    if (numberRenderer.numberId === 161) {
                        return Math.floor(elemLoader.screenRoot.gameplayTimeSeconds(1, false) / 60);
                    }
                    if (numberRenderer.numberId === 162) {
                        return elemLoader.screenRoot.gameplayTimeSeconds(1, false) % 60;
                    }
                    if (numberRenderer.numberId === 163) {
                        return Math.floor(elemLoader.screenRoot.gameplayTimeSeconds(1, true) / 60);
                    }
                    if (numberRenderer.numberId === 164) {
                        return elemLoader.screenRoot.gameplayTimeSeconds(1, true) % 60;
                    }
                    return elemLoader.screenRoot.numberValue(numberRenderer.numberSrc);
                }
                return elemLoader.screenRoot.numberValue(numberRenderer.numberSrc);
            }
            forceHidden: elemLoader.screenRoot.numberForceHidden(numberSrc)
            animationRevision: elemLoader.screenRoot.numberAnimationRevision(numberSrc)
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            screenRoot: elemLoader.screenRoot
        }
    }

    Component {
        id: textComponent
        Lr2TextElement {
            width: elemLoader.skinW * elemLoader.skinScale
            height: elemLoader.skinH * elemLoader.skinScale
            screenRoot: elemLoader.screenRoot
            selectContext: elemLoader.selectContext
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            skinScale: elemLoader.skinScale
        }
    }

    Component {
        id: readmeTextComponent
        Lr2ReadmeTextElement {
            width: elemLoader.skinW * elemLoader.skinScale
            height: elemLoader.skinH * elemLoader.skinScale
            screenRoot: elemLoader.screenRoot
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.renderSkinTime
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            skinScale: elemLoader.skinScale
        }
    }

    Component {
        id: barImageComponent
        Lr2BarSpriteRenderer {
            readonly property bool sourceAnimates: elemLoader.elementSourceTreeAnimates
            readonly property bool needsTimelineSkinTime: elemLoader.elementData.src
                && elemLoader.elementData.src.kind >= 2
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: needsTimelineSkinTime ? elemLoader.screenRoot.barSkinTime : 0
            sourceSkinTime: 0
            skinClock: sourceAnimates ? elemLoader.screenRoot.skinClockRef : null
            sourceSkinClockMode: elemLoader.sourceClockMode(sourceAnimates)
            activeOptions: elemLoader.screenRoot.barActiveOptions
            timers: elemLoader.screenRoot.barTimers
            scaleOverride: skinScale
            selectContext: elemLoader.screenRoot.selectContextRef
            barRows: skinModel.barRows
            barLampVariants: skinModel.barLampVariants
            barBaseStateResolver: elemLoader.screenRoot.barBaseStateResolver
            barPositionMap: elemLoader.screenRoot.barPositionMap
            barCells: selectContext.visibleBarTextCells
            barTextCells: selectContext.visibleBarTextCells
            fastBarScrollActive: elemLoader.screenRoot.fastBarScrollActive
            fastBarScrollX: elemLoader.screenRoot.fastBarScrollX
            fastBarScrollY: elemLoader.screenRoot.fastBarScrollY
            selectedFastBarDrawX: elemLoader.screenRoot.selectedFastBarDrawX
            selectedFastBarDrawY: elemLoader.screenRoot.selectedFastBarDrawY
            barCenter: skinModel.barCenter
            stageFileSource: elemLoader.elementSourceTreeUsesChartAsset ? elemLoader.stageFileSource : ""
            backBmpSource: elemLoader.elementSourceTreeUsesChartAsset ? elemLoader.backBmpSource : ""
            bannerSource: elemLoader.elementSourceTreeUsesChartAsset ? elemLoader.bannerSource : ""
            transColor: skinModel.transColor
            colorKeyEnabled: skinModel.hasTransColor
        }
    }

    Component {
        id: barTextComponent
        Lr2BarTextRenderer {
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.barSkinTime
            activeOptions: elemLoader.screenRoot.barActiveOptions
            timers: elemLoader.screenRoot.barTimers
            scaleOverride: skinScale
            selectContext: elemLoader.screenRoot.selectContextRef
            barRows: skinModel.barRows
            barBaseStateResolver: elemLoader.screenRoot.barBaseStateResolver
            barPositionMap: elemLoader.screenRoot.barPositionMap
            barCells: selectContext.visibleBarTextCells
            fastBarScrollActive: elemLoader.screenRoot.fastBarScrollActive
            fastBarScrollX: elemLoader.screenRoot.fastBarScrollX
            fastBarScrollY: elemLoader.screenRoot.fastBarScrollY
            selectedFastBarDrawX: elemLoader.screenRoot.selectedFastBarDrawX
            selectedFastBarDrawY: elemLoader.screenRoot.selectedFastBarDrawY
            barCenter: skinModel.barCenter
        }
    }

    Component {
        id: barNumberComponent
        Lr2BarNumberRenderer {
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.barSkinTime
            activeOptions: elemLoader.screenRoot.barActiveOptions
            timers: elemLoader.screenRoot.barTimers
            scaleOverride: skinScale
            selectContext: elemLoader.screenRoot.selectContextRef
            barRows: skinModel.barRows
            barBaseStateResolver: elemLoader.screenRoot.barBaseStateResolver
            barPositionMap: elemLoader.screenRoot.barPositionMap
            barCells: selectContext.visibleBarTextCells
            fastBarScrollActive: elemLoader.screenRoot.fastBarScrollActive
            fastBarScrollX: elemLoader.screenRoot.fastBarScrollX
            fastBarScrollY: elemLoader.screenRoot.fastBarScrollY
            selectedFastBarDrawX: elemLoader.screenRoot.selectedFastBarDrawX
            selectedFastBarDrawY: elemLoader.screenRoot.selectedFastBarDrawY
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
                : elemLoader.sourceSkinTimeForAnimation(sourceAnimates)
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            sourceSkinClockMode: elemLoader.useDirectElementSkinClock
                ? elemLoader.sourceClockMode(sourceAnimates)
                : elemLoader.manualClock
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            chartAssetSource: elemLoader.chartAssetSourceFor(elemLoader.elementData.src)
            scaleOverride: skinScale
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            value: {
                let graphType = elemLoader.elementData.src ? elemLoader.elementData.src.graphType || 0 : 0;
                return elemLoader.screenRoot.effectiveScreenKey === "select"
                    ? selectContext.nativeBarGraphValue(graphType)
                    : elemLoader.screenRoot.resolveBarGraph(graphType);
            }
            animateValue: elemLoader.screenRoot.effectiveScreenKey === "select"
                && elemLoader.elementData.src
                && elemLoader.elementData.src.graphType >= 5
                && elemLoader.elementData.src.graphType <= 9
        }
    }

    Component {
        id: barDistributionGraphComponent
        Lr2BarDistributionGraphRenderer {
            readonly property bool barDistributionSourceAnimates: elemLoader.screenRoot.barDistributionGraphSourceAnimates(elemLoader.elementData.src)
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.barSkinTime
            sourceSkinTime: 0
            skinClock: barDistributionSourceAnimates ? elemLoader.screenRoot.skinClockRef : null
            sourceSkinClockMode: elemLoader.sourceClockMode(barDistributionSourceAnimates)
            activeOptions: elemLoader.screenRoot.barActiveOptions
            timers: elemLoader.screenRoot.barTimers
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            scaleOverride: skinScale
            selectContext: elemLoader.screenRoot.selectContextRef
            barRows: skinModel.barRows
            barPositionMap: elemLoader.screenRoot.barPositionMap
            barCells: selectContext.visibleBarTextCells
            fastBarScrollActive: elemLoader.screenRoot.fastBarScrollActive
            fastBarScrollX: elemLoader.screenRoot.fastBarScrollX
            fastBarScrollY: elemLoader.screenRoot.fastBarScrollY
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            chartAssetSource: elemLoader.chartAssetSourceFor(elemLoader.elementData.src)
        }
    }
}
