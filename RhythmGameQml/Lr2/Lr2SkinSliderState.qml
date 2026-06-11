pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: sliders

    required property var screenRoot
    required property var selectContext
    required property var valueResolver
    property var optionChangeSound
    property var skinTiming: null

    readonly property var root: screenRoot
    readonly property var skinTimerLookup: skinTiming || (root ? root.skinTimingRef : null)
    property int selectSliderFixedPoint: -1
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    property Lr2SkinSliderGeometry sliderGeometry: Lr2SkinSliderGeometry {}
    readonly property int selectScrollSpriteStateOverride: 1
    readonly property int gameplayProgressSpriteStateOverride: 2
    readonly property int gameplayLaneCoverSpriteStateOverride: 3
    readonly property int numberRefSpriteStateOverride: 4
    readonly property int genericSliderSpriteStateOverride: 5

    function isSelectScrollSlider(src: var) : var {
        return sliderGeometry.isSelectScrollSlider(root.effectiveScreenKey, src);
    }

    function isLr2GenericSlider(src: var) : var {
        return sliderGeometry.isGenericSlider(root.effectiveScreenKey, src);
    }

    function isGameplayProgressSlider(src: var) : var {
        return sliderGeometry.isGameplayProgressSlider(root.gameplayScreenActive, src);
    }

    function isGameplayLaneCoverSlider(src: var) : var {
        return sliderGeometry.isGameplayLaneCoverSlider(root.gameplayScreenActive, src);
    }

    function isLr2NumberRefSlider(src: var) : var {
        return sliderGeometry.isNumberRefSlider(src);
    }

    function trackState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!src || !src.slider) {
            return null;
        }
        let timer = timelineResolver.firstTimerFor(dsts);
        let liveClock = root.effectiveScreenKey === "select"
            && (((timer >= 21 && timer <= 29) || (timer >= 31 && timer <= 39))
                || timelineResolver.loopsContinuouslyFor(dsts));
        let sourceLiveClock = root.effectiveScreenKey === "select"
            && timelineResolver.sourceCyclesContinuously(src);
        let clock = skinTime === undefined
            ? ((liveClock || sourceLiveClock) ? root.selectSourceSkinTime : root.renderSkinTime)
            : skinTime;
        let effectiveTimerFire = timerFire === undefined
            ? (skinTimerLookup ? skinTimerLookup.skinTimerFireTime(timer, liveClock) : -1)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? root.activeOptionsForDsts(dsts, root.runtimeActiveOptions)
            : activeOptions;
        let base = timelineResolver.stateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base || !base.valid) {
            return null;
        }
        let track = sliderGeometry.trackState(src, base);
        return track && track.valid ? track : null;
    }

    function positionFromPointer(src: var, track: var, pointerX: var, pointerY: var) : var {
        if (!track || !track.valid) {
            return 0;
        }
        return sliderGeometry.positionFromPointer(src, track, pointerX, pointerY);
    }

    function translatedState(src: var, dsts: var, position: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        let timer = timelineResolver.firstTimerFor(dsts);
        let liveClock = root.effectiveScreenKey === "select"
            && (((timer >= 21 && timer <= 29) || (timer >= 31 && timer <= 39))
                || timelineResolver.loopsContinuouslyFor(dsts));
        let sourceLiveClock = root.effectiveScreenKey === "select"
            && timelineResolver.sourceCyclesContinuously(src);
        let clock = skinTime === undefined
            ? ((liveClock || sourceLiveClock) ? root.selectSourceSkinTime : root.renderSkinTime)
            : skinTime;
        let effectiveTimerFire = timerFire === undefined
            ? (skinTimerLookup ? skinTimerLookup.skinTimerFireTime(timer, liveClock) : -1)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? root.activeOptionsForDsts(dsts, root.runtimeActiveOptions)
            : activeOptions;
        let base = timelineResolver.stateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base || !base.valid) {
            return null;
        }
        let state = sliderGeometry.translatedState(src, base, position);
        return state && state.valid ? state : null;
    }

    function selectScrollPosition(src: var) : var {
        if (!sliders.isSelectScrollSlider(src)) {
            return 0;
        }
        return sliders.selectScrollPositionForSlider();
    }

    function selectScrollPositionForSlider() : var {
        let logicalCount = Math.max(1, selectContext.logicalCount);
        let maxFixed = Math.max(1, logicalCount * 1000 - 1);
        let visualState = selectContext.visualStateObject;
        let fixedValue = visualState
            ? visualState.fixed
            : selectContext.listCalculatedBarFixed;
        fixedValue = Math.max(0, Math.min(maxFixed, fixedValue));
        return logicalCount > 1 ? fixedValue / maxFixed : 0;
    }

    function genericPosition(src: var) : var {
        if (!sliders.isLr2GenericSlider(src)) {
            return 0;
        }
        return sliders.genericPositionForSlider(src);
    }

    function genericPositionForSlider(src: var) : var {
        return src.sliderType === 7
            ? root.lr2SkinCustomPosition()
            : root.sliderRawValue(src.sliderType) / 100;
    }

    function gameplayProgressPosition(src: var) : var {
        if (!sliders.isGameplayProgressSlider(src)) {
            return 0;
        }
        return sliders.gameplayProgressPositionForSlider();
    }

    function gameplayProgressPositionForSlider() : var {
        let frameState = root.gameplayFrameStateRef;
        return frameState ? frameState.progressPosition || 0 : 0;
    }

    function gameplayLaneCoverPosition(src: var) : var {
        if (!sliders.isGameplayLaneCoverSlider(src)) {
            return 0;
        }
        return sliders.gameplayLaneCoverPositionForSlider(src);
    }

    function gameplayLaneCoverPositionForSlider(src: var) : var {
        let side = src.sliderType === 5 ? 2 : 1;
        return Math.max(0, Math.min(1, root.gameplayLaneCoverSliderPosition(side)));
    }

    function numberRefSliderPosition(src: var) : var {
        if (!sliders.isLr2NumberRefSlider(src)) {
            return 0;
        }
        return sliders.numberRefSliderPositionForSlider(src);
    }

    function numberRefSliderPositionForSlider(src: var) : var {
        let minValue = src.sliderMinValue || 0;
        let maxValue = src.sliderMaxValue || 0;
        if (minValue === maxValue) {
            return 0;
        }
        let value = valueResolver.resolveNumber(src.sliderType || 0);
        if (minValue < maxValue) {
            return value <= minValue ? 0
                : (value >= maxValue ? 1 : Math.abs((value - minValue) / (maxValue - minValue)));
        }
        return value <= maxValue ? 1
            : (value >= minValue ? 0 : Math.abs((value - minValue) / (maxValue - minValue)));
    }

    function positionForKind(kind: var, src: var) : var {
        switch (kind) {
        case selectScrollSpriteStateOverride:
            return sliders.selectScrollPositionForSlider();
        case gameplayProgressSpriteStateOverride:
            return sliders.gameplayProgressPositionForSlider();
        case gameplayLaneCoverSpriteStateOverride:
            return sliders.gameplayLaneCoverPositionForSlider(src);
        case numberRefSpriteStateOverride:
            return sliders.numberRefSliderPositionForSlider(src);
        case genericSliderSpriteStateOverride:
            return sliders.genericPositionForSlider(src);
        default:
            return 0;
        }
    }

    function selectScrollState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!sliders.isSelectScrollSlider(src)) {
            return null;
        }

        return sliders.translatedState(
            src,
            dsts,
            sliders.selectScrollPosition(src),
            skinTime,
            timerFire,
            activeOptions);
    }

    function genericState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!sliders.isLr2GenericSlider(src)) {
            return null;
        }
        return sliders.translatedState(
            src,
            dsts,
            sliders.genericPosition(src),
            skinTime,
            timerFire,
            activeOptions);
    }

    function gameplayProgressState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!sliders.isGameplayProgressSlider(src)) {
            return null;
        }
        return sliders.translatedState(
            src,
            dsts,
            sliders.gameplayProgressPosition(src),
            skinTime,
            timerFire,
            activeOptions);
    }

    function gameplayLaneCoverState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!sliders.isGameplayLaneCoverSlider(src)) {
            return null;
        }
        return sliders.translatedState(
            src,
            dsts,
            sliders.gameplayLaneCoverPosition(src),
            skinTime,
            timerFire,
            activeOptions);
    }

    function numberRefSliderState(src: var, dsts: var, skinTime: var, timerFire: var, activeOptions: var) : var {
        if (!sliders.isLr2NumberRefSlider(src)) {
            return null;
        }
        return sliders.translatedState(
            src,
            dsts,
            sliders.numberRefSliderPosition(src),
            skinTime,
            timerFire,
            activeOptions);
    }

    function selectScrollTrackState(src: var, dsts: var, skinTime: var) : var {
        return sliders.isSelectScrollSlider(src)
            ? sliders.trackState(src, dsts, skinTime)
            : null;
    }

    function genericTrackState(src: var, dsts: var, skinTime: var) : var {
        return sliders.isLr2GenericSlider(src)
            ? sliders.trackState(src, dsts, skinTime)
            : null;
    }

    function setSelectScrollFromPointer(src: var, dsts: var, pointerX: var, pointerY: var) : var {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0) {
            return;
        }
        sliders.setSelectScrollFromTrack(
            src,
            sliders.selectScrollTrackState(src, dsts),
            pointerX,
            pointerY);
    }

    function setSelectScrollFromTrack(src: var, track: var, pointerX: var, pointerY: var) : var {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0 || !track || !track.valid) {
            return;
        }

        let position = sliders.positionFromPointer(src, track, pointerX, pointerY);
        let maxFixed = Math.max(0, selectContext.logicalCount * 1000 - 1);
        let fixedPoint = Math.max(0, Math.min(maxFixed, Math.round(position * maxFixed)));
        if (fixedPoint === sliders.selectSliderFixedPoint) {
            return;
        }
        sliders.selectSliderFixedPoint = fixedPoint;
        selectContext.dragScrollFixedPoint(fixedPoint);
    }

    function setGenericFromPointer(src: var, dsts: var, pointerX: var, pointerY: var) : void {
        sliders.setGenericFromTrack(
            src,
            sliders.genericTrackState(src, dsts),
            pointerX,
            pointerY);
    }

    function setGenericFromTrack(src: var, track: var, pointerX: var, pointerY: var) : var {
        if (!track || !track.valid) {
            return;
        }
        let position = sliders.positionFromPointer(src, track, pointerX, pointerY);
        if (src.sliderType === 7) {
            if (root.setLr2SkinCustomPosition(position)) {
                root.playOneShot(sliders.optionChangeSound);
            }
            return;
        }
        root.setSliderRawValue(src.sliderType, position * 100);
    }
}

