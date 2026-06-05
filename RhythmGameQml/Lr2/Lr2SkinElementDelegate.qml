pragma ValueTypeBehavior: Addressable
pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

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
    required property var skinTiming
    required property var valueResolver
    required property var sliderState
    required property var selectPanelController
    required property var selectHoverState
    required property var selectSearchState
    required property var runtimeElementDescriptors
    required property var runtimeElementTimerStates
    required property var gameplayFrameState
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
    readonly property string elementKey: String(elementIndex)
    readonly property var elementData: ({ type: type, src: src, dsts: dsts || [] })
    property bool componentCompleted: false

    readonly property var elementState: elementIndex >= 0
        && elementIndex < elemLoader.runtimeElementDescriptors.length
        ? elemLoader.runtimeElementDescriptors[elementIndex]
        : ({})
    readonly property bool sourceTreeHasFrameAnimation: elementState.sourceTreeHasFrameAnimation
    readonly property bool sourceTreeUsesChartAsset: elementState.sourceTreeUsesChartAsset
    readonly property int directChartAssetSourceType: elementState.directChartAssetSourceType
    readonly property string directChartAssetSource: directChartAssetSourceType === 1
        ? stageFileSource
        : directChartAssetSourceType === 3
            ? backBmpSource
            : directChartAssetSourceType === 4
                ? bannerSource
                : ""

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
    readonly property var elementTimerState: elementIndex >= 0
        && elementIndex < elemLoader.runtimeElementTimerStates.length
        ? elemLoader.runtimeElementTimerStates[elementIndex]
        : null
    readonly property int dstTimerFire: {
        if (!elemLoader.usesDynamicDstTimer) {
            return elemLoader.dstTimer === 0 ? 0 : -1;
        }
        if (elemLoader.usesLiveGameplayDstTimer) {
            return elemLoader.gameplayFrameState
                ? elemLoader.gameplayFrameState.rhythmTimerSkinTime
                : -1;
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
            return elemLoader.gameplayFrameState
                ? elemLoader.gameplayFrameState.rhythmTimerSkinTime
                : -1;
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
    readonly property int elementTypeImage: 0
    readonly property int elementTypeNumber: 1
    readonly property int elementTypeText: 2
    readonly property int elementTypeBarImage: 3
    readonly property int elementTypeBarText: 4
    readonly property int elementTypeBarNumber: 5
    readonly property int elementTypeBarGraph: 6
    readonly property int elementTypeBga: 7
    readonly property int elementTypePlayNotes: 8
    readonly property int elementTypeGrooveGauge: 9
    readonly property int elementTypeResultChart: 10
    readonly property int elementTypeNoteChart: 11
    readonly property int elementTypeBpmChart: 12
    readonly property int elementTypeBarDistributionGraph: 13
    readonly property int elementSkinClockMode: elementState.elementSkinClockMode
    readonly property bool useDirectElementSkinClock: elementState.useDirectElementSkinClock
    readonly property bool needsManualElementSkinTime: elementState.needsManualElementSkinTime
    readonly property int selectHeldButtonSkinClock: usesSelectHeldButtonTimer
        ? (elemLoader.selectPanelController
            ? (elemLoader.selectPanelController.hasSelectHeldButtonTimers
                ? elemLoader.selectPanelController.selectHeldButtonSkinTime
                : elemLoader.selectPanelController.currentSelectHeldButtonSkinTime())
            : 0)
        : 0
    readonly property int elementSkinTime: needsManualElementSkinTime
        ? elementSkinTimeForClock(elementSkinClockMode, usesLiveSelectClock, elemLoader.screenRoot.renderSkinTime, false)
        : 0

    function elementSkinTimeForClock(clockMode: var, useSelectLiveClock: var, fallbackSkinTime: var, allowHeldButtonTimer: var) : var {
        if (allowHeldButtonTimer && usesSelectHeldButtonTimer) {
            return selectHeldButtonSkinClock;
        }
        if (clockMode === selectInfoClock) {
            return elemLoader.screenRoot.selectInfoElapsed;
        }
        return useSelectLiveClock ? elemLoader.screenRoot.selectSourceSkinTime : fallbackSkinTime;
    }

    function sourceSkinClockModeFor(sourceAnimates: var) : var {
        if (!sourceAnimates) {
            return manualClock;
        }
        return elemLoader.screenRoot.effectiveScreenKey === "select"
            ? selectSourceClock
            : renderClock;
    }

    function sourceSkinTimeForAnimatedSource(sourceAnimates: var) : var {
        if (!sourceAnimates) {
            return 0;
        }
        return elemLoader.screenRoot.effectiveScreenKey === "select"
            ? elemLoader.screenRoot.selectSourceSkinTime
            : elemLoader.screenRoot.renderSkinTime;
    }

    function componentForElement() : var {
        switch (elemLoader.elementData.type) {
        case elemLoader.elementTypeImage:
            return elementState.sourceMouseCursor ? undefined : imageComponent;
        case elemLoader.elementTypeNumber:
            return numberComponent;
        case elemLoader.elementTypeText:
            return elemLoader.elementData.src && elemLoader.elementData.src.readme
                ? readmeTextComponent
                : textComponent;
        case elemLoader.elementTypeBarImage:
            return barImageComponent;
        case elemLoader.elementTypeBarText:
            return barTextComponent;
        case elemLoader.elementTypeBarNumber:
            return barNumberComponent;
        case elemLoader.elementTypeBarGraph:
            return barGraphComponent;
        case elemLoader.elementTypeBga:
            return bgaComponent;
        case elemLoader.elementTypePlayNotes:
            return playNotesComponent;
        case elemLoader.elementTypeGrooveGauge:
            return grooveGaugeComponent;
        case elemLoader.elementTypeResultChart:
            return resultChartComponent;
        case elemLoader.elementTypeNoteChart:
            return noteChartComponent;
        case elemLoader.elementTypeBpmChart:
            return bpmChartComponent;
        case elemLoader.elementTypeBarDistributionGraph:
            return barDistributionGraphComponent;
        default:
            return undefined;
        }
    }

    function registerPointerElement() : void {
        if (!componentCompleted) {
            return;
        }
        pointerController.registerElement(
            elementIndex,
            elemLoader.elementData.type,
            elemLoader.elementData.src,
            elementState.z || 0,
            elemLoader.elementData.dsts);
    }

    onElementDataChanged: registerPointerElement()
    onElementStateChanged: registerPointerElement()

    Component.onCompleted: {
        componentCompleted = true;
        if (elemLoader.selectHoverState) {
            elemLoader.selectHoverState.registerElement(elementIndex, elemLoader.elementData.src, usesSpriteForceHidden);
        }
        if (elemLoader.selectPanelController) {
            elemLoader.selectPanelController.registerSelectButtonSource(elementIndex, elemLoader.elementData.src);
        }
        registerPointerElement();
    }
    Component.onDestruction: {
        if (elemLoader.selectHoverState) {
            elemLoader.selectHoverState.unregisterElement(elementIndex);
        }
        if (elemLoader.selectPanelController) {
            elemLoader.selectPanelController.unregisterSelectButtonSource(elementIndex);
        }
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
            readonly property var nowJudgeSource: elemLoader.elementData.src
            readonly property bool nowJudgeSprite: elemLoader.screenRoot.gameplayScreenActive
                && !!nowJudgeSource
                && (nowJudgeSource.timer === 46 || nowJudgeSource.timer === 47)
                && (nowJudgeSource.w || 0) > 0
                && (nowJudgeSource.h || 0) > 0
                && (nowJudgeSource.op1 || 0) === 0
            readonly property int nowJudgeCombo: nowJudgeSprite
                ? (nowJudgeSource.timer === 47
                    ? elemLoader.screenRoot.gameplayJudgeCombo2
                    : elemLoader.screenRoot.gameplayJudgeCombo1)
                : 0
            readonly property var nowJudgeDst: elemLoader.elementData.dsts && elemLoader.elementData.dsts.length > 0
                ? elemLoader.elementData.dsts[0]
                : null
            readonly property real nowJudgeBaseHeight: nowJudgeDst && nowJudgeDst.h
                ? Math.abs(nowJudgeDst.h)
                : (nowJudgeSource ? nowJudgeSource.h || 30 : 30)
            readonly property int nowJudgeComboDigitWidth: Math.max(1, Math.round(nowJudgeBaseHeight * 22 / 30))
            readonly property real nowJudgeOffsetX: nowJudgeCombo > 0
                ? -Math.abs(Math.round(nowJudgeCombo)).toString().length * nowJudgeComboDigitWidth * 0.5
                : 0
            readonly property int scratchRotationSide: elementState.scratchRotationSide
            readonly property bool useDirectSkinClock: elementState.spriteUsesDirectSkinClock
            readonly property int spriteSkinClockMode: elementState.spriteSkinClockMode
            readonly property int spriteSourceSkinClockMode: elementState.spriteSourceSkinClockMode
            readonly property int spriteSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                ? elemLoader.elementSkinTimeForClock(
                    spriteSkinClockMode,
                    elemLoader.usesLiveDstClock,
                    elemLoader.screenRoot.renderSkinTime,
                    true)
                : 0
            readonly property int spriteSourceSkinClock: !useDirectSkinClock && elemLoader.usesSkinTime
                ? elemLoader.elementSkinTimeForClock(
                    spriteSourceSkinClockMode,
                    elemLoader.usesLiveSourceClock,
                    spriteSkinClock,
                    true)
                : 0
            readonly property bool sliderTranslationEnabled: elemLoader.usesSpriteStateOverride
            readonly property real sliderPosition: sliderTranslationEnabled && elemLoader.sliderState
                ? elemLoader.sliderState.positionForKind(elemLoader.spriteStateOverrideKind, elemLoader.elementData.src)
                : 0
            readonly property bool dstOffsetsEnabled: elementState.dstOffsetsEnabled
            readonly property int dstOffsetSide: elementState.dstOffsetSide
            readonly property int buttonFrameOverrideValue: elemLoader.usesButtonFrameOverride
                ? (elemLoader.selectPanelController
                    ? elemLoader.selectPanelController.buttonFrame(elemLoader.elementData.src)
                    : -1)
                : -1
            readonly property bool hiddenBySelectHover: elemLoader.usesSpriteForceHidden
                && !!elemLoader.selectHoverState
                && !!elemLoader.elementData.src
                && elemLoader.elementData.src.onMouse
                && elemLoader.selectHoverState.visibleByIndex[elemLoader.elementKey] !== true

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: elemLoader.elementData.dsts
                srcData: elemLoader.valueResolver.imageSetSourceFor(elemLoader.elementData.src)
                sourceHasFrameAnimation: imageComponentRoot.sourceAnimates
                skinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSkinClock
                sourceSkinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSourceSkinClock
                skinClock: imageComponentRoot.useDirectSkinClock ? elemLoader.screenRoot.skinClockRef : null
                skinClockMode: imageComponentRoot.spriteSkinClockMode
                sourceSkinClockMode: imageComponentRoot.spriteSourceSkinClockMode
                activeOptionsState: elemLoader.elementActiveOptionsState
                timerFire: elemLoader.dstTimerFire
                sourceTimerFire: elemLoader.srcTimerFire
                chartAssetSource: elemLoader.directChartAssetSource
                scaleOverride: skinScale
                mediaActive: elemLoader.screenRoot.enabled && elemLoader.screenUpdatesActive
                transColor: skinModel.transColor
                colorKeyEnabled: skinModel.hasTransColor
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
                forceHidden: imageComponentRoot.hiddenBySelectHover
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
            mediaActive: elemLoader.screenRoot.enabled && elemLoader.screenRoot.gameplayScreenActive && elemLoader.screenRoot.lr2BgaEnabled
            poorVisible: elemLoader.screenRoot.gameplayPoorBgaVisible1
        }
    }

    Component {
        id: playNotesComponent
        Lr2PlayNoteField {
            anchors.fill: parent
            screenRoot: elemLoader.screenRoot
            skinTiming: elemLoader.skinTiming
            skinModel: elemLoader.screenRoot.skinModelRef
            skinScale: skinScale
            renderSkinTime: elemLoader.screenRoot.renderSkinTime
            runtimeActiveOptions: elemLoader.skinRuntime && elemLoader.skinRuntime.noteFieldUsesActiveOptions
                ? elemLoader.skinRuntime.runtimeActiveOptions
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
        }
    }

    Component {
        id: numberComponent
        Lr2NumberRenderer {
            id: numberRenderer
            readonly property var numberSrc: elemLoader.elementData.src
            readonly property int numberId: numberSrc ? numberSrc.num || 0 : 0
            readonly property bool numberSourceAnimates: elementState.sourceHasFrameAnimation
            readonly property bool selectNumberScreen: elemLoader.screenRoot.effectiveScreenKey === "select"
            readonly property bool usesFpsNumber: selectNumberScreen && numberId === 20
            readonly property bool selectNumberHasFocusedState: !!elementState.numberUsesFocusedSelectState
            readonly property bool selectNumberAlwaysUsesResolver: (numberId >= 410 && numberId <= 419)
                || (numberId >= 421 && numberId <= 424)
                || (numberId >= 1312 && numberId <= 1327)
            readonly property bool selectNumberUsesValueResolver: !selectNumberScreen
                || !selectNumberHasFocusedState
                || selectNumberAlwaysUsesResolver
            readonly property bool usesFocusedSelectNumber: hasCurrentState
                && selectNumberScreen
                && !usesFpsNumber
                && !selectNumberUsesValueResolver
            readonly property bool valueResolverNumberNeeded: hasCurrentState
                && !usesFpsNumber
                && !usesFocusedSelectNumber
            readonly property int valueResolverNumber: valueResolverNumberNeeded
                ? elemLoader.valueResolver.numberValue(numberSrc)
                : 0
            readonly property int focusedSelectNumber: usesFocusedSelectNumber
                ? selectContext.numberValue(numberId)
                : 0
            readonly property int currentNumberValue: hasCurrentState
                ? (usesFpsNumber
                    ? elemLoader.screenRoot.lr2CurrentFps
                    : (usesFocusedSelectNumber ? focusedSelectNumber : valueResolverNumber))
                : 0
            dsts: elemLoader.elementData.dsts
            srcData: numberSrc
            skinTime: elemLoader.useDirectElementSkinClock && !numberSourceAnimates ? 0 : elemLoader.elementSkinTime
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            scaleOverride: skinScale
            value: numberRenderer.currentNumberValue
            forceHidden: elemLoader.valueResolver.numberForceHidden(numberSrc)
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
            selectSearchState: elemLoader.selectSearchState
            valueResolver: elemLoader.valueResolver
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
            readonly property bool sourceAnimates: elemLoader.sourceTreeHasFrameAnimation
            readonly property bool needsTimelineSkinTime: elemLoader.elementData.src
                && elemLoader.elementData.src.kind >= 2
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: needsTimelineSkinTime ? elemLoader.screenRoot.barSkinTime : 0
            sourceSkinTime: 0
            skinClock: sourceAnimates ? elemLoader.screenRoot.skinClockRef : null
            sourceSkinClockMode: elemLoader.sourceSkinClockModeFor(sourceAnimates)
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
            stageFileSource: elemLoader.sourceTreeUsesChartAsset ? elemLoader.stageFileSource : ""
            backBmpSource: elemLoader.sourceTreeUsesChartAsset ? elemLoader.backBmpSource : ""
            bannerSource: elemLoader.sourceTreeUsesChartAsset ? elemLoader.bannerSource : ""
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
            sourceHasFrameAnimation: sourceAnimates
            skinTime: elemLoader.useDirectElementSkinClock ? 0 : elemLoader.elementSkinTime
            sourceSkinTime: elemLoader.useDirectElementSkinClock
                ? 0
                : elemLoader.sourceSkinTimeForAnimatedSource(sourceAnimates)
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            sourceSkinClockMode: elemLoader.useDirectElementSkinClock
                ? elemLoader.sourceSkinClockModeFor(sourceAnimates)
                : elemLoader.manualClock
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            chartAssetSource: elemLoader.directChartAssetSource
            scaleOverride: skinScale
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            value: {
                let graphType = elemLoader.elementData.src ? elemLoader.elementData.src.graphType || 0 : 0;
                return elemLoader.screenRoot.effectiveScreenKey === "select"
                    ? selectContext.barGraphValue(graphType)
                    : elemLoader.valueResolver.resolveBarGraph(graphType);
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
            dsts: elemLoader.elementData.dsts
            srcData: elemLoader.elementData.src
            skinTime: elemLoader.screenRoot.barSkinTime
            sourceSkinTime: 0
            skinClock: sourceAnimates ? elemLoader.screenRoot.skinClockRef : null
            sourceSkinClockMode: elemLoader.sourceSkinClockModeFor(sourceAnimates)
            activeOptionsState: elemLoader.elementActiveOptionsState
            activeOptions: elemLoader.screenRoot.barActiveOptions
            timers: elemLoader.screenRoot.barTimers
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            scaleOverride: skinScale
            selectContext: elemLoader.screenRoot.selectContextRef
            barRows: skinModel.barRows
            barPositionMap: elemLoader.screenRoot.barPositionMap
            barCells: elemLoader.selectContext ? elemLoader.selectContext.visibleBarTextCells : []
            fastBarScrollActive: elemLoader.screenRoot.fastBarScrollActive
            fastBarScrollX: elemLoader.screenRoot.fastBarScrollX
            fastBarScrollY: elemLoader.screenRoot.fastBarScrollY
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            chartAssetSource: elemLoader.directChartAssetSource
        }
    }
}
