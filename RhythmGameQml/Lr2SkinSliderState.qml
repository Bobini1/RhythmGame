pragma ValueTypeBehavior: Addressable

import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

QtObject {
    id: sliders

    required property var screenRoot
    required property var selectContext
    property var optionChangeSound

    readonly property var root: screenRoot
    property int selectSliderFixedPoint: -1

    function isSelectScrollSlider(src) {
        return root.effectiveScreenKey === "select"
            && !!src
            && !!src.slider
            && src.sliderType === 1
            && src.sliderRange > 0
            && src.sliderDisabled === 0;
    }

    function isLr2GenericSlider(src) {
        return root.effectiveScreenKey === "select"
            && !!src
            && !!src.slider
            && !sliders.isSelectScrollSlider(src)
            && src.sliderRange > 0
            && src.sliderDisabled === 0;
    }

    function isGameplayProgressSlider(src) {
        return root.isGameplayScreen()
            && !!src
            && !!src.slider
            && src.sliderType === 6
            && src.sliderRange > 0;
    }

    function isGameplayLaneCoverSlider(src) {
        return root.isGameplayScreen()
            && !!src
            && !!src.slider
            && (src.sliderType === 4 || src.sliderType === 5)
            && src.sliderRange > 0;
    }

    function isLr2NumberRefSlider(src) {
        return !!src
            && !!src.slider
            && !!src.sliderRefNumber
            && src.sliderRange > 0;
    }

    function trackState(src, dsts, skinTime, timerFire, activeOptions) {
        if (!src || !src.slider) {
            return null;
        }
        let clock = skinTime === undefined
            ? root.skinTimeForElement(src, dsts)
            : skinTime;
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        let liveClock = root.elementUsesLiveDstClock(dsts);
        let effectiveTimerFire = timerFire === undefined
            ? root.skinTimerFireTime(timer, liveClock)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? (root.dstsUseActiveOptions(dsts)
                ? root.activeOptionsForElementDsts(dsts)
                : root.emptyActiveOptions)
            : activeOptions;
        let base = Lr2Timeline.getCurrentStateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base) {
            return null;
        }

        let track = {
            x: base.x,
            y: base.y,
            w: base.w,
            h: base.h
        };
        let range = Math.max(1, src.sliderRange || 0);
        switch (src.sliderDirection) {
        case 0:
            track.y -= range;
            track.h += range;
            break;
        case 1:
            track.w += range;
            break;
        case 2:
            track.h += range;
            break;
        case 3:
            track.x -= range;
            track.w += range;
            break;
        default:
            return null;
        }
        return track;
    }

    function positionFromPointer(src, track, pointerX, pointerY) {
        let range = Math.max(1, src.sliderRange || 0);
        let moveX = (track.w - range) / 2;
        let moveY = (track.h - range) / 2;
        let position = 0;
        switch (src.sliderDirection) {
        case 0: {
            let start = track.y + moveY;
            let end = track.y + track.h - moveY;
            position = end !== start ? (end - pointerY) / (end - start) : 0;
            break;
        }
        case 1: {
            let start = track.x + moveX;
            let end = track.x + track.w - moveX;
            position = end !== start ? (pointerX - start) / (end - start) : 0;
            break;
        }
        case 2: {
            let start = track.y + moveY;
            let end = track.y + track.h - moveY;
            position = end !== start ? (pointerY - start) / (end - start) : 0;
            break;
        }
        case 3: {
            let start = track.x + moveX;
            let end = track.x + track.w - moveX;
            position = end !== start ? (end - pointerX) / (end - start) : 0;
            break;
        }
        default:
            return 0;
        }
        return Math.max(0, Math.min(1, position));
    }

    function translatedState(src, dsts, position, skinTime, timerFire, activeOptions) {
        let clock = skinTime === undefined
            ? root.skinTimeForElement(src, dsts)
            : skinTime;
        let timer = dsts && dsts.length > 0 ? (dsts[0].timer || 0) : 0;
        let liveClock = root.elementUsesLiveDstClock(dsts);
        let effectiveTimerFire = timerFire === undefined
            ? root.skinTimerFireTime(timer, liveClock)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? (root.dstsUseActiveOptions(dsts)
                ? root.activeOptionsForElementDsts(dsts)
                : root.emptyActiveOptions)
            : activeOptions;
        let base = Lr2Timeline.getCurrentStateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base) {
            return null;
        }

        let sliderOffset = position * Math.max(1, src.sliderRange || 0);
        let offsetX = 0;
        let offsetY = 0;
        switch (src.sliderDirection) {
        case 0:
            offsetY = -sliderOffset;
            break;
        case 1:
            offsetX = sliderOffset;
            break;
        case 2:
            offsetY = sliderOffset;
            break;
        case 3:
            offsetX = -sliderOffset;
            break;
        default:
            return null;
        }

        return {
            x: base.x + offsetX,
            y: base.y + offsetY,
            w: base.w,
            h: base.h,
            a: base.a,
            r: base.r,
            g: base.g,
            b: base.b,
            angle: base.angle || 0,
            center: base.center || 0,
            blend: base.blend || 0,
            filter: base.filter || 0,
            op1: base.op1 || 0,
            op2: base.op2 || 0,
            op3: base.op3 || 0,
            op4: base.op4 || 0
        };
    }

    function selectScrollPosition(src) {
        if (!sliders.isSelectScrollSlider(src)) {
            return 0;
        }

        let logicalCount = Math.max(1, selectContext.logicalCount);
        let maxFixed = Math.max(1, logicalCount * 1000 - 1);
        let visualState = selectContext.visualStateObject;
        let fixedValue = visualState
            ? visualState.fixed
            : selectContext.listCalculatedBarFixed;
        fixedValue = Math.max(0, Math.min(maxFixed, fixedValue));
        return logicalCount > 1 ? fixedValue / maxFixed : 0;
    }

    function genericPosition(src) {
        if (!sliders.isLr2GenericSlider(src)) {
            return 0;
        }
        return src.sliderType === 7
            ? root.lr2SkinCustomPosition()
            : root.sliderRawValue(src.sliderType) / 100;
    }

    function gameplayProgressPosition(src) {
        if (!sliders.isGameplayProgressSlider(src)) {
            return 0;
        }
        let frameState = root.gameplayFrameStateRef;
        return frameState ? frameState.progressPosition || 0 : 0;
    }

    function gameplayLaneCoverPosition(src) {
        if (!sliders.isGameplayLaneCoverSlider(src)) {
            return 0;
        }
        let side = src.sliderType === 5 ? 2 : 1;
        return Math.max(0, Math.min(1, root.gameplayLaneCoverSliderPosition(side)));
    }

    function numberRefSliderPosition(src) {
        if (!sliders.isLr2NumberRefSlider(src)) {
            return 0;
        }
        let minValue = src.sliderMinValue || 0;
        let maxValue = src.sliderMaxValue || 0;
        if (minValue === maxValue) {
            return 0;
        }
        let value = root.resolveNumber(src.sliderType || 0);
        if (minValue < maxValue) {
            return value <= minValue ? 0
                : (value >= maxValue ? 1 : Math.abs((value - minValue) / (maxValue - minValue)));
        }
        return value <= maxValue ? 1
            : (value >= minValue ? 0 : Math.abs((value - minValue) / (maxValue - minValue)));
    }

    function selectScrollState(src, dsts, skinTime, timerFire, activeOptions) {
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

    function genericState(src, dsts, skinTime, timerFire, activeOptions) {
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

    function gameplayProgressState(src, dsts, skinTime, timerFire, activeOptions) {
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

    function gameplayLaneCoverState(src, dsts, skinTime, timerFire, activeOptions) {
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

    function numberRefSliderState(src, dsts, skinTime, timerFire, activeOptions) {
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

    function selectScrollTrackState(src, dsts, skinTime) {
        return sliders.isSelectScrollSlider(src)
            ? sliders.trackState(src, dsts, skinTime)
            : null;
    }

    function genericTrackState(src, dsts, skinTime) {
        return sliders.isLr2GenericSlider(src)
            ? sliders.trackState(src, dsts, skinTime)
            : null;
    }

    function setSelectScrollFromPointer(src, dsts, pointerX, pointerY) {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0) {
            return;
        }
        sliders.setSelectScrollFromTrack(
            src,
            sliders.selectScrollTrackState(src, dsts),
            pointerX,
            pointerY);
    }

    function setSelectScrollFromTrack(src, track, pointerX, pointerY) {
        if (root.effectiveScreenKey !== "select" || selectContext.logicalCount <= 0 || !track) {
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

    function setGenericFromPointer(src, dsts, pointerX, pointerY) {
        sliders.setGenericFromTrack(
            src,
            sliders.genericTrackState(src, dsts),
            pointerX,
            pointerY);
    }

    function setGenericFromTrack(src, track, pointerX, pointerY) {
        if (!track) {
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
