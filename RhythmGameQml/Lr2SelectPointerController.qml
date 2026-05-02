pragma ValueTypeBehavior: Addressable

import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

QtObject {
    id: pointerController

    required property var screenRoot
    required property var selectContext
    property real skinScale: 1
    property var elements: ({})

    readonly property var root: screenRoot
    readonly property bool ready: root !== undefined && root !== null
    readonly property bool selectReady: selectContext !== undefined && selectContext !== null

    function copyObject(object) {
        let result = {};
        const keys = Object.keys(object || {});
        for (let i = 0; i < keys.length; ++i) {
            result[keys[i]] = object[keys[i]];
        }
        return result;
    }

    function registerElement(elementIndex, type, src, dsts, z) {
        if (!ready || type !== 0 || !src) {
            return;
        }

        const buttonId = root.elementButtonId(src);
        const selectScroll = root.isSelectScrollSlider(src);
        const genericSlider = root.isLr2GenericSlider(src);
        if (buttonId <= 0 && !selectScroll && !genericSlider) {
            return;
        }

        let nextElements = copyObject(elements);
        nextElements[String(elementIndex)] = {
            index: elementIndex,
            src: src,
            dsts: dsts,
            z: z || 0,
            buttonId: buttonId,
            sourceCount: root.elementSourceFrameCount(src),
            selectScroll: selectScroll,
            genericSlider: genericSlider,
            staticState: Lr2Timeline.canUseStaticState(dsts)
                ? Lr2Timeline.getCurrentState(dsts, 0, root.zeroTimers, root.emptyActiveOptions)
                : null
        };
        elements = nextElements;
    }

    function unregisterElement(elementIndex) {
        const key = String(elementIndex);
        if (elements[key] === undefined) {
            return;
        }
        let nextElements = copyObject(elements);
        delete nextElements[key];
        elements = nextElements;
    }

    function elementState(element) {
        if (!ready || !element) {
            return null;
        }
        if (element.staticState) {
            return element.staticState;
        }
        const dstTimer = element.dsts && element.dsts.length > 0
            ? (element.dsts[0].timer || 0)
            : 0;
        return Lr2Timeline.getCurrentStateFromTimerFire(
            element.dsts,
            root.renderSkinTime,
            root.skinTimerFireTime(dstTimer),
            root.dstsUseActiveOptions(element.dsts)
                ? root.activeOptionsForDsts(element.dsts, root.runtimeActiveOptions)
                : root.emptyActiveOptions);
    }

    function rectContains(state, skinX, skinY) {
        if (!state) {
            return false;
        }
        const left = Math.min(state.x, state.x + state.w);
        const right = Math.max(state.x, state.x + state.w);
        const top = Math.min(state.y, state.y + state.h);
        const bottom = Math.max(state.y, state.y + state.h);
        return skinX >= left && skinX <= right && skinY >= top && skinY <= bottom;
    }

    function sliderTrackState(element) {
        if (!ready || !element || (!element.selectScroll && !element.genericSlider)) {
            return null;
        }
        const sliderSkinClock = root.elementUsesLiveSelectClock(element.src, element.dsts)
            ? root.selectSourceSkinTime
            : root.renderSkinTime;
        return element.selectScroll
            ? root.selectScrollSliderTrackState(element.src, element.dsts, sliderSkinClock)
            : root.lr2GenericSliderTrackState(element.src, element.dsts, sliderSkinClock);
    }

    function hitSlider(skinX, skinY) {
        if (!ready || !root.selectPointerScrollReady()) {
            return null;
        }
        let best = null;
        const keys = Object.keys(elements);
        for (let i = 0; i < keys.length; ++i) {
            const element = elements[keys[i]];
            if (!element || (!element.selectScroll && !element.genericSlider)) {
                continue;
            }
            const trackState = sliderTrackState(element);
            if (!rectContains(trackState, skinX, skinY)) {
                continue;
            }
            const z = 100300 + element.index;
            if (!best || z > best.z) {
                best = { kind: "slider", element: element, state: trackState, z: z };
            }
        }
        return best;
    }

    function hitButton(skinX, skinY) {
        if (!ready || !root.selectPointerInputReady()) {
            return null;
        }
        let best = null;
        const keys = Object.keys(elements);
        for (let i = 0; i < keys.length; ++i) {
            const element = elements[keys[i]];
            if (!element || element.buttonId <= 0) {
                continue;
            }
            if (!root.elementButtonClickEnabled(element.src)
                    || !root.elementButtonPanelMatches(element.src)) {
                continue;
            }
            const state = elementState(element);
            if (!rectContains(state, skinX, skinY)) {
                continue;
            }
            if (!best || element.z > best.z) {
                best = { kind: "button", element: element, state: state, z: element.z };
            }
        }
        return best;
    }

    function rowAt(skinX, skinY) {
        if (!ready || !root.selectPointerScrollReady()) {
            return -1;
        }
        const states = root.cachedBarBaseStates || [];
        for (let row = root.barClickEnd(); row >= root.barClickStart(); --row) {
            const state = row >= 0 && row < states.length ? states[row] : null;
            if (rectContains(state, skinX, skinY)) {
                return row;
            }
        }
        return -1;
    }

    function targetAt(x, y, button) {
        if (!ready) {
            return { kind: "blank", skinX: 0, skinY: 0 };
        }
        const skinX = x / skinScale;
        const skinY = y / skinScale;
        const slider = button === Qt.LeftButton ? hitSlider(skinX, skinY) : null;
        if (slider) {
            slider.skinX = skinX;
            slider.skinY = skinY;
            return slider;
        }

        const buttonTarget = hitButton(skinX, skinY);
        if (buttonTarget) {
            buttonTarget.skinX = skinX;
            buttonTarget.skinY = skinY;
            return buttonTarget;
        }

        const row = rowAt(skinX, skinY);
        return row >= 0
            ? { kind: "bar", row: row, skinX: skinX, skinY: skinY }
            : { kind: "blank", skinX: skinX, skinY: skinY };
    }

    function updateSlider(target, x, y) {
        if (!ready || !target || target.kind !== "slider") {
            return;
        }
        const skinX = x / skinScale;
        const skinY = y / skinScale;
        if (target.element.selectScroll) {
            root.setSelectScrollFromSliderTrack(target.element.src, target.state, skinX, skinY);
        } else if (target.element.genericSlider) {
            root.setLr2GenericSliderFromTrack(target.element.src, target.state, skinX, skinY);
        }
    }

    function finishSlider(target) {
        if (!ready || !selectReady || !target || target.kind !== "slider" || !target.element.selectScroll) {
            return;
        }
        selectContext.finishScrollFixedPoint(100);
        root.selectSliderFixedPoint = -1;
    }

    function clickButton(target, mouse) {
        if (!ready || !target) {
            return;
        }
        const state = target.state || elementState(target.element);
        if (!state) {
            return;
        }
        const left = Math.min(state.x, state.x + state.w);
        const width = Math.abs(state.w);
        const delta = root.buttonMouseDelta(
            target.element.src,
            target.skinX - left,
            width);
        root.handleLr2Button(
            target.element.buttonId,
            delta,
            root.elementButtonPanel(target.element.src),
            undefined,
            target.element.sourceCount);
        mouse.accepted = true;
    }

    function sameTarget(a, b) {
        if (!a || !b || a.kind !== b.kind) {
            return false;
        }
        if (a.kind === "button" || a.kind === "slider") {
            return a.element && b.element && a.element.index === b.element.index;
        }
        if (a.kind === "bar") {
            return a.row === b.row;
        }
        return a.kind === "blank";
    }
}
