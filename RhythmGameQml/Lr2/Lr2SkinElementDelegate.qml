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
    readonly property var elementData: ({ type: type, src: src, dsts: dsts || [] })
    readonly property bool elementSourceTreeAnimates: Lr2SkinUtils.sourceTreeCyclesContinuously(src)
    readonly property bool elementSourceTreeUsesChartAsset: Lr2SkinUtils.sourceTreeUsesChartAsset(src)
    property bool luaDrawVisible: true
    property bool luaDrawVisibleRefreshPending: false

    readonly property var elementState: {
        elemLoader.runtimeRevision;
        return elemLoader.skinRuntime
            ? elemLoader.skinRuntime.descriptor(elementIndex)
            : ({});
    }
    readonly property int luaDrawCallback: elementState.luaDrawCallback || 0

    readonly property bool usesActiveOptions: elementState.usesActiveOptions
    readonly property bool usesSkinTime: elementState.usesSkinTime
    readonly property int dstTimer: elementState.dstTimer
    readonly property int srcTimer: elementState.srcTimer
    readonly property int dstTimerCallback: elementState.dstTimerCallback || 0
    readonly property int srcTimerCallback: elementState.srcTimerCallback || 0
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
        if (elemLoader.dstTimerCallback > 0) {
            elemLoader.screenRoot.renderSkinTime;
            elemLoader.screenRoot.beatorajaLuaRevision();
            return elemLoader.screenRoot.beatorajaLuaTimerCallback(elemLoader.dstTimerCallback);
        }
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
        if (elemLoader.srcTimerCallback > 0) {
            elemLoader.screenRoot.renderSkinTime;
            elemLoader.screenRoot.beatorajaLuaRevision();
            return elemLoader.screenRoot.beatorajaLuaTimerCallback(elemLoader.srcTimerCallback);
        }
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
        ? (elemLoader.screenRoot.hasSelectHeldButtonTimers
            ? elemLoader.screenRoot.selectHeldButtonSkinTime
            : elemLoader.screenRoot.currentSelectHeldButtonSkinTime())
        : 0
    readonly property int elementSkinTime: needsManualElementSkinTime
        ? elementSkinTimeForClock(elementSkinClockMode, usesLiveSelectClock, elemLoader.screenRoot.renderSkinTime, false)
        : 0

    visible: luaDrawVisible
    enabled: luaDrawVisible

    function refreshLuaDrawVisible() : void {
        luaDrawVisible = elemLoader.luaDrawCallback > 0
            ? elemLoader.screenRoot.beatorajaLuaDrawCallback(elemLoader.luaDrawCallback, true)
            : true;
    }

    function scheduleLuaDrawVisibleRefresh() : void {
        if (elemLoader.luaDrawVisibleRefreshPending) {
            return;
        }
        elemLoader.luaDrawVisibleRefreshPending = true;
        Qt.callLater(function() {
            elemLoader.luaDrawVisibleRefreshPending = false;
            if (elemLoader) {
                elemLoader.refreshLuaDrawVisible();
            }
        });
    }

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

    function chartAssetSourceFor(source: var) : var {
        return Lr2SkinUtils.chartAssetSourceFor(
            source,
            stageFileSource,
            backBmpSource,
            bannerSource);
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

    Component.onCompleted: {
        elemLoader.scheduleLuaDrawVisibleRefresh();
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

    onLuaDrawCallbackChanged: scheduleLuaDrawVisibleRefresh()

    Connections {
        target: elemLoader.screenRoot
        function onRenderSkinTimeChanged() : void {
            if (elemLoader.luaDrawCallback > 0) {
                elemLoader.scheduleLuaDrawVisibleRefresh();
            }
        }
        function onRuntimeActiveOptionsChanged() : void { elemLoader.scheduleLuaDrawVisibleRefresh(); }
        function onSelectRevisionChanged() : void { elemLoader.scheduleLuaDrawVisibleRefresh(); }
    }

    Connections {
        target: elemLoader.screenRoot ? elemLoader.screenRoot.beatorajaLuaRuntime() : null
        function onRevisionChanged() : void { elemLoader.scheduleLuaDrawVisibleRefresh(); }
    }

    Component {
        id: imageComponent
        Item {
            id: imageComponentRoot
            width: skinW * skinScale
            height: skinH * skinScale
            readonly property bool sourceAnimates: Lr2SkinUtils.sourceCyclesContinuously(spriteSrcData)
                || elementState.sourceHasFrameAnimation
            readonly property real nowJudgeOffsetX: elemLoader.screenRoot.nowJudgeOffsetX(elemLoader.elementData.src, elemLoader.elementData.dsts)
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
            readonly property real sliderPosition: sliderTranslationEnabled
                ? elemLoader.screenRoot.spriteSliderPositionForKind(elemLoader.spriteStateOverrideKind, elemLoader.elementData.src)
                : 0
            readonly property bool dstOffsetsEnabled: elementState.dstOffsetsEnabled
            readonly property int dstOffsetSide: elementState.dstOffsetSide
            readonly property int buttonFrameOverrideValue: elemLoader.usesButtonFrameOverride
                ? elemLoader.screenRoot.buttonFrame(elemLoader.elementData.src)
                : -1
            readonly property bool spriteSourceIsImageSet: {
                const source = elemLoader.elementData.src;
                return !!source
                    && !!source.imageSet
                    && !!source.imageSetSources
                    && source.imageSetSources.length > 0;
            }
            readonly property bool spriteSourceUsesLuaImageSetCallback: {
                const source = elemLoader.elementData.src;
                return spriteSourceIsImageSet && (source.imageSetValueCallback || 0) > 0;
            }
            readonly property bool spriteSourceNeedsRenderRefresh: spriteSourceUsesLuaImageSetCallback
                || (spriteSourceIsImageSet && elemLoader.screenRoot.isGameplayScreen())
            property var spriteSrcData: null
            property bool spriteSrcRefreshPending: false
            readonly property int spriteSourceTimerFire: sourceTimerFireForSprite(
                spriteSrcData,
                elemLoader.srcTimerFire)

            function sourceTimerFireForSprite(source: var, fallbackFire: var) : var {
                if (!source) {
                    return fallbackFire;
                }
                let callback = Math.floor(Number(source.timerCallback) || 0);
                if (callback > 0) {
                    elemLoader.screenRoot.renderSkinTime;
                    elemLoader.screenRoot.beatorajaLuaRevision();
                    return elemLoader.screenRoot.beatorajaLuaTimerCallback(callback);
                }
                let timer = Math.floor(Number(source.timer) || 0);
                if (timer === elemLoader.srcTimer && elemLoader.srcTimerCallback <= 0) {
                    return fallbackFire;
                }
                if (timer === 0) {
                    return 0;
                }
                if (elemLoader.screenRoot.skinTimerStateRef
                        && elemLoader.screenRoot.skinTimerStateRef.skinTimerCanFire(timer)) {
                    let fireMs = elemLoader.screenRoot.skinTimerStateRef.skinTimerFireTime(
                        timer,
                        elemLoader.usesLiveSourceClock);
                    return fireMs >= 0 ? fireMs : -1;
                }
                return -1;
            }

            function refreshSpriteSrcData() : void {
                const nextSource = elemLoader.screenRoot.imageSetSourceFor(elemLoader.elementData.src);
                if (imageComponentRoot.spriteSrcData !== nextSource) {
                    imageComponentRoot.spriteSrcData = nextSource;
                }
            }

            function scheduleSpriteSrcDataRefresh() : void {
                if (imageComponentRoot.spriteSrcRefreshPending) {
                    return;
                }
                imageComponentRoot.spriteSrcRefreshPending = true;
                Qt.callLater(function() {
                    imageComponentRoot.spriteSrcRefreshPending = false;
                    if (imageComponentRoot) {
                        imageComponentRoot.refreshSpriteSrcData();
                    }
                });
            }

            Component.onCompleted: scheduleSpriteSrcDataRefresh()

            Connections {
                target: elemLoader
                function onRuntimeRevisionChanged() : void {
                    if (imageComponentRoot.spriteSourceUsesLuaImageSetCallback) {
                        imageComponentRoot.scheduleSpriteSrcDataRefresh();
                    }
                }
                function onSrcChanged() : void { imageComponentRoot.scheduleSpriteSrcDataRefresh(); }
            }

            Connections {
                target: elemLoader.screenRoot
                function onRenderSkinTimeChanged() : void {
                    if (imageComponentRoot.spriteSourceNeedsRenderRefresh) {
                        imageComponentRoot.scheduleSpriteSrcDataRefresh();
                    }
                }
                function onRuntimeActiveOptionsChanged() : void {
                    if (imageComponentRoot.spriteSourceIsImageSet) {
                        imageComponentRoot.scheduleSpriteSrcDataRefresh();
                    }
                }
                function onSelectRevisionChanged() : void {
                    if (imageComponentRoot.spriteSourceIsImageSet) {
                        imageComponentRoot.scheduleSpriteSrcDataRefresh();
                    }
                }
            }

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: elemLoader.elementData.dsts
                srcData: imageComponentRoot.spriteSrcData
                skinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSkinClock
                sourceSkinTime: imageComponentRoot.useDirectSkinClock ? 0 : imageComponentRoot.spriteSourceSkinClock
                skinClock: imageComponentRoot.useDirectSkinClock ? elemLoader.screenRoot.skinClockRef : null
                skinClockMode: imageComponentRoot.spriteSkinClockMode
                sourceSkinClockMode: imageComponentRoot.spriteSourceSkinClockMode
                activeOptionsState: elemLoader.elementActiveOptionsState
                timerFire: elemLoader.dstTimerFire
                sourceTimerFire: imageComponentRoot.spriteSourceTimerFire
                chartAssetSource: elemLoader.chartAssetSourceFor(imageComponentRoot.spriteSrcData)
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
        Item {
            id: numberComponentRoot

            width: elemLoader.skinW * elemLoader.skinScale
            height: elemLoader.skinH * elemLoader.skinScale

            readonly property var numberSrc: elemLoader.elementData.src
            readonly property int numberId: numberSrc ? numberSrc.num || 0 : 0
            readonly property bool numberNowCombo: !!(numberSrc && numberSrc.nowCombo)
            readonly property int numberNowComboSide: numberNowCombo
                ? (numberSrc.side || (numberSrc.timer === 47 ? 2 : 1))
                : 0
            readonly property bool numberSourceAnimates: elementState.sourceHasFrameAnimation
            property int numberValue: 0
            property bool numberValueRefreshPending: false

            function computeNumberValue() : int {
                if (elemLoader.screenRoot.effectiveScreenKey === "select") {
                    let num = numberComponentRoot.numberId;
                    if (num === 20) {
                        return elemLoader.screenRoot.lr2CurrentFps;
                    }
                    if (!elementState.numberUsesFocusedSelectState) {
                        return elemLoader.screenRoot.numberValue(numberComponentRoot.numberSrc);
                    }
                    if ((num >= 410 && num <= 419) || (num >= 421 && num <= 424)) {
                        return elemLoader.screenRoot.numberValue(numberComponentRoot.numberSrc);
                    }
                    if (num >= 1312 && num <= 1327) {
                        return elemLoader.screenRoot.numberValue(numberComponentRoot.numberSrc);
                    }
                    return selectContext.numberValue(num);
                }
                if (elemLoader.screenRoot.gameplayScreenActive) {
                    if (numberComponentRoot.numberNowCombo) {
                        return numberComponentRoot.numberNowComboSide === 2
                            ? elemLoader.screenRoot.gameplayJudgeCombo2
                            : elemLoader.screenRoot.gameplayJudgeCombo1;
                    }
                    if (numberComponentRoot.numberId === 20) {
                        return elemLoader.screenRoot.lr2CurrentFps;
                    }
                    if (numberComponentRoot.numberId === 160) {
                        let p1 = elemLoader.screenRoot.gameplayPlayer(1);
                        return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
                    }
                    if (numberComponentRoot.numberId === 161) {
                        return Math.floor(elemLoader.screenRoot.gameplayTimeSeconds(1, false) / 60);
                    }
                    if (numberComponentRoot.numberId === 162) {
                        return elemLoader.screenRoot.gameplayTimeSeconds(1, false) % 60;
                    }
                    if (numberComponentRoot.numberId === 163) {
                        return Math.floor(elemLoader.screenRoot.gameplayTimeSeconds(1, true) / 60);
                    }
                    if (numberComponentRoot.numberId === 164) {
                        return elemLoader.screenRoot.gameplayTimeSeconds(1, true) % 60;
                    }
                    return elemLoader.screenRoot.numberValue(numberComponentRoot.numberSrc);
                }
                return elemLoader.screenRoot.numberValue(numberComponentRoot.numberSrc);
            }

            function refreshNumberValue() : void {
                numberComponentRoot.numberValue = numberComponentRoot.computeNumberValue();
            }

            function scheduleNumberValueRefresh() : void {
                if (numberComponentRoot.numberValueRefreshPending) {
                    return;
                }
                numberComponentRoot.numberValueRefreshPending = true;
                Qt.callLater(function() {
                    numberComponentRoot.numberValueRefreshPending = false;
                    if (numberComponentRoot) {
                        numberComponentRoot.refreshNumberValue();
                    }
                });
            }

            Component.onCompleted: scheduleNumberValueRefresh()
            onNumberIdChanged: scheduleNumberValueRefresh()

            Connections {
                target: elemLoader
                function onRuntimeRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onSrcChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
            }

            Connections {
                target: elemLoader.screenRoot
                function onRenderSkinTimeChanged() : void {
                    if (numberComponentRoot.numberId === 20) {
                        numberComponentRoot.refreshNumberValue();
                        return;
                    }
                    numberComponentRoot.scheduleNumberValueRefresh();
                }
                function onRuntimeActiveOptionsChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onSelectRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onLr2CurrentFpsChanged() : void {
                    if (numberComponentRoot.numberId === 20) {
                        numberComponentRoot.refreshNumberValue();
                    }
                }
            }

            Connections {
                target: elemLoader.screenRoot ? elemLoader.screenRoot.beatorajaLuaRuntime() : null
                function onRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
            }

            Connections {
                target: elemLoader.selectContext
                function onFocusRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onScoreRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onListRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onPlayerStatsRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
                function onRankingStatsRevisionChanged() : void { numberComponentRoot.scheduleNumberValueRefresh(); }
            }

            Lr2NumberRenderer {
                anchors.fill: parent
                dsts: elemLoader.elementData.dsts
                srcData: numberComponentRoot.numberSrc
                skinTime: elemLoader.useDirectElementSkinClock && !numberComponentRoot.numberSourceAnimates
                    ? 0
                    : elemLoader.elementSkinTime
                skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
                skinClockMode: elemLoader.elementSkinClockMode
                activeOptionsState: elemLoader.elementActiveOptionsState
                timerFire: elemLoader.dstTimerFire
                sourceTimerFire: elemLoader.srcTimerFire
                scaleOverride: skinScale
                value: numberComponentRoot.numberValue
                forceHidden: elemLoader.screenRoot.numberForceHidden(numberComponentRoot.numberSrc)
                animationRevision: elemLoader.screenRoot.numberAnimationRevision(numberComponentRoot.numberSrc)
                colorKeyEnabled: skinModel.hasTransColor
                transColor: skinModel.transColor
                screenRoot: elemLoader.screenRoot
            }
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
            showUnresolvedTextFallback: !elemLoader.screenRoot.lr2SkinUsesBeatorajaSemantics
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
                : elemLoader.sourceSkinTimeForAnimatedSource(sourceAnimates)
            skinClock: elemLoader.useDirectElementSkinClock ? elemLoader.screenRoot.skinClockRef : null
            skinClockMode: elemLoader.elementSkinClockMode
            sourceSkinClockMode: elemLoader.useDirectElementSkinClock
                ? elemLoader.sourceSkinClockModeFor(sourceAnimates)
                : elemLoader.manualClock
            activeOptionsState: elemLoader.elementActiveOptionsState
            timerFire: elemLoader.dstTimerFire
            sourceTimerFire: elemLoader.srcTimerFire
            chartAssetSource: elemLoader.chartAssetSourceFor(elemLoader.elementData.src)
            scaleOverride: skinScale
            colorKeyEnabled: skinModel.hasTransColor
            transColor: skinModel.transColor
            value: {
                let src = elemLoader.elementData.src;
                if (!src) {
                    return 0;
                }
                let graphType = src.graphType || 0;
                return elemLoader.screenRoot.effectiveScreenKey === "select"
                    && (src.valueCallback || 0) <= 0
                    && !src.graphRefNumber
                    ? selectContext.nativeBarGraphValue(graphType)
                    : elemLoader.screenRoot.barGraphValue(src);
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
            sourceSkinClockMode: elemLoader.sourceSkinClockModeFor(barDistributionSourceAnimates)
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
