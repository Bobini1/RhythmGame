pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: sliders

    required property var screenRoot
    required property var selectContext
    property var optionChangeSound

    readonly property var root: screenRoot
    property int selectSliderFixedPoint: -1
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    property Lr2SkinSliderGeometry sliderGeometry: Lr2SkinSliderGeometry {}

    function isSelectScrollSlider(src) {
        return sliderGeometry.isSelectScrollSlider(root.effectiveScreenKey, src);
    }

    function isLr2GenericSlider(src) {
        return sliderGeometry.isGenericSlider(root.effectiveScreenKey, src);
    }

    function isGameplayProgressSlider(src) {
        return sliderGeometry.isGameplayProgressSlider(root.isGameplayScreen(), src);
    }

    function isGameplayLaneCoverSlider(src) {
        return sliderGeometry.isGameplayLaneCoverSlider(root.isGameplayScreen(), src);
    }

    function isLr2NumberRefSlider(src) {
        return sliderGeometry.isNumberRefSlider(src);
    }

    function trackState(src, dsts, skinTime, timerFire, activeOptions) {
        if (!src || !src.slider) {
            return null;
        }
        let clock = skinTime === undefined
            ? root.skinTimeForElement(src, dsts)
            : skinTime;
        let timer = timelineResolver.firstTimerFor(dsts);
        let liveClock = root.elementUsesLiveDstClock(dsts);
        let effectiveTimerFire = timerFire === undefined
            ? root.skinTimerFireTime(timer, liveClock)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? (root.dstsUseActiveOptions(dsts)
                ? root.activeOptionsForElementDsts(dsts)
                : root.emptyActiveOptions)
            : activeOptions;
        let base = timelineResolver.stateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base) {
            return null;
        }
        return sliderGeometry.trackState(src, base);
    }

    function positionFromPointer(src, track, pointerX, pointerY) {
        return sliderGeometry.positionFromPointer(src, track, pointerX, pointerY);
    }

    function translatedState(src, dsts, position, skinTime, timerFire, activeOptions) {
        let clock = skinTime === undefined
            ? root.skinTimeForElement(src, dsts)
            : skinTime;
        let timer = timelineResolver.firstTimerFor(dsts);
        let liveClock = root.elementUsesLiveDstClock(dsts);
        let effectiveTimerFire = timerFire === undefined
            ? root.skinTimerFireTime(timer, liveClock)
            : timerFire;
        let effectiveActiveOptions = activeOptions === undefined
            ? (root.dstsUseActiveOptions(dsts)
                ? root.activeOptionsForElementDsts(dsts)
                : root.emptyActiveOptions)
            : activeOptions;
        let base = timelineResolver.stateFromTimerFire(
            dsts,
            clock,
            effectiveTimerFire,
            effectiveActiveOptions);
        if (!base) {
            return null;
        }
        return sliderGeometry.translatedState(src, base, position);
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
