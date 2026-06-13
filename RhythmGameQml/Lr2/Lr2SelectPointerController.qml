pragma ValueTypeBehavior: Addressable
import QtQuick

QtObject {
    id: pointerController

    required property var screenRoot
    required property var selectContext
    required property var selectBarGeometry
    property var sliderState: null
    property var selectPanelController: null
    property var runtimeElementDescriptors: []
    property var skinRuntime: null
    property real skinScale: 1
    property var elements: ({})
    property var elementList: []

    readonly property var root: screenRoot
    readonly property var barGeometry: selectBarGeometry
    readonly property bool ready: root !== undefined && root !== null
    readonly property bool selectReady: selectContext !== undefined && selectContext !== null

    function descriptorFor(index: var) : var {
        const descriptors = runtimeElementDescriptors || [];
        return index >= 0 && index < descriptors.length
            ? descriptors[index]
            : null;
    }

    function copyObject(object: var) : var {
        let result = {};
        const keys = Object.keys(object || {});
        for (let i = 0; i < keys.length; ++i) {
            result[keys[i]] = object[keys[i]];
        }
        return result;
    }

    function updateElementList() : void {
        const keys = Object.keys(elements || {});
        let nextList = [];
        for (let i = 0; i < keys.length; ++i) {
            const element = elements[keys[i]];
            if (element) {
                nextList.push(element);
            }
        }
        elementList = nextList;
    }

    function registerElement(elementIndex: var, type: var, src: var, z: var, dsts: var) : var {
        if (!ready) {
            return;
        }
        if (type !== 0 || !src) {
            unregisterElement(elementIndex);
            return;
        }

        const descriptor = descriptorFor(elementIndex);
        const hasDescriptor = descriptor && descriptor.valid;
        const buttonId = hasDescriptor && descriptor.buttonId > 0
            ? descriptor.buttonId
            : (selectPanelController ? selectPanelController.elementButtonId(src) : 0);
        const selectScroll = hasDescriptor
            ? descriptor.selectScrollSlider
            : (sliderState ? sliderState.isSelectScrollSlider(src) : false);
        const genericSlider = hasDescriptor
            ? descriptor.genericSlider
            : (sliderState ? sliderState.isLr2GenericSlider(src) : false);
        if (buttonId <= 0 && !selectScroll && !genericSlider) {
            unregisterElement(elementIndex);
            return;
        }

        let nextElements = copyObject(elements);
        nextElements[String(elementIndex)] = {
            index: elementIndex,
            src: src,
            runtimeIndex: elementIndex,
            z: z || 0,
            buttonId: buttonId,
            sourceCount: root.elementSourceFrameCount(src),
            replayType: replayTypeForDsts(dsts),
            selectScroll: selectScroll,
            genericSlider: genericSlider
        };
        elements = nextElements;
        updateElementList();
    }

    function unregisterElement(elementIndex: var) : var {
        const key = String(elementIndex);
        if (elements[key] === undefined) {
            return;
        }
        let nextElements = copyObject(elements);
        delete nextElements[key];
        elements = nextElements;
        updateElementList();
    }

    function elementState(element: var) : var {
        if (!ready || !element) {
            return null;
        }
        if (!skinRuntime || element.runtimeIndex === undefined) {
            return null;
        }
        const staticState = skinRuntime.staticStateForElement(element.runtimeIndex);
        if (staticState && staticState.valid) {
            return staticState;
        }
        const descriptor = descriptorFor(element.runtimeIndex);
        const state = skinRuntime.stateForElement(
            element.runtimeIndex,
            descriptor && descriptor.usesLiveDstClock ? root.selectSourceSkinTime : root.renderSkinTime);
        return state && state.valid ? state : null;
    }

    function rectContains(state: var, skinX: var, skinY: var) : var {
        if (!state) {
            return false;
        }
        const left = Math.min(state.x, state.x + state.w);
        const right = Math.max(state.x, state.x + state.w);
        const top = Math.min(state.y, state.y + state.h);
        const bottom = Math.max(state.y, state.y + state.h);
        return skinX >= left && skinX <= right && skinY >= top && skinY <= bottom;
    }

    function sliderTrackState(element: var) : var {
        if (!ready || !element || (!element.selectScroll && !element.genericSlider)) {
            return null;
        }
        if (skinRuntime && element.runtimeIndex !== undefined) {
            const descriptor = descriptorFor(element.runtimeIndex);
            const sliderSkinClock = descriptor && descriptor.usesLiveSelectClock
                ? root.selectSourceSkinTime
                : root.renderSkinTime;
            const state = skinRuntime.sliderTrackStateForElement(element.runtimeIndex, sliderSkinClock);
            return state && state.valid ? state : null;
        }
        return null;
    }

    function hitSlider(skinX: var, skinY: var) : var {
        if (!ready || !root.selectPointerScrollReady()) {
            return null;
        }
        let best = null;
        const list = elementList || [];
        for (let i = 0; i < list.length; ++i) {
            const element = list[i];
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

    function hitButton(skinX: var, skinY: var) : var {
        if (!ready || !selectPanelController || !root.selectPointerInputReady()) {
            return null;
        }
        let best = null;
        const list = elementList || [];
        for (let i = 0; i < list.length; ++i) {
            const element = list[i];
            if (!element || element.buttonId <= 0) {
                continue;
            }
            if (!selectPanelController.elementButtonClickEnabled(element.src)
                    || !selectPanelController.elementButtonPanelMatches(element.src)) {
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

    function rowAt(skinX: var, skinY: var) : var {
        if (!ready || !barGeometry || !root.selectPointerScrollReady()) {
            return -1;
        }
        const barBaseStates = barGeometry.barBaseStates || [];
        for (let row = barGeometry.clickEndRow; row >= barGeometry.clickStartRow; --row) {
            const state = row >= 0 && row < barBaseStates.length && barBaseStates[row].valid
                ? barBaseStates[row]
                : null;
            if (rectContains(state, skinX, skinY)) {
                return row;
            }
        }
        return -1;
    }

    function targetAt(x: var, y: var, button: var) : var {
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

    function buttonAt(x: var, y: var) : var {
        if (!ready) {
            return null;
        }
        const skinX = x / skinScale;
        const skinY = y / skinScale;
        const buttonTarget = hitButton(skinX, skinY);
        if (!buttonTarget) {
            return null;
        }
        buttonTarget.skinX = skinX;
        buttonTarget.skinY = skinY;
        return buttonTarget;
    }

    function clickBarRow(row: var, mouse: var) : void {
        if (barGeometry) {
            barGeometry.handleBarRowClick(row, mouse);
        }
    }

    function replayLabel(replayType: var) : var {
        switch (Math.floor(replayType || 0)) {
        case 0:
            return qsTr("NEWEST");
        case 1:
            return qsTr("BEST SCORE");
        case 2:
            return qsTr("BEST CLEAR");
        case 3:
            return qsTr("BEST COMBO");
        default:
            return "";
        }
    }

    function replayTypeForButton(buttonId: var) : var {
        switch (Math.floor(buttonId || 0)) {
        case 19:
        case 83:
            return root.lr2ReplayType;
        case 316:
            return 1;
        case 317:
            return 2;
        case 318:
            return 3;
        default:
            return -1;
        }
    }

    function replayTypeForOption(option: var) : var {
        switch (Math.floor(option || 0)) {
        case 197:
        case 1205:
            return 0;
        case 1197:
        case 1206:
            return 1;
        case 1200:
        case 1207:
            return 2;
        case 1203:
        case 1208:
            return 3;
        default:
            return -1;
        }
    }

    function replayTypeForDst(dst: var) : var {
        if (!dst) {
            return -1;
        }
        let type = replayTypeForOption(dst.op1);
        if (type >= 0) {
            return type;
        }
        type = replayTypeForOption(dst.op2);
        if (type >= 0) {
            return type;
        }
        type = replayTypeForOption(dst.op3);
        if (type >= 0) {
            return type;
        }
        return replayTypeForOption(dst.op4);
    }

    function replayTypeForDsts(dsts: var) : var {
        const count = dsts ? (dsts.length || 0) : 0;
        for (let i = 0; i < count; ++i) {
            const type = replayTypeForDst(dsts[i]);
            if (type >= 0) {
                return type;
            }
        }
        return -1;
    }

    function replayTypeForTarget(target: var) : var {
        if (!target || !target.element) {
            return -1;
        }
        if (Math.floor(target.element.buttonId || 0) === 19) {
            return root.lr2ReplayType;
        }
        return target.element.replayType >= 0
            ? target.element.replayType
            : replayTypeForButton(target.element.buttonId);
    }

    function replayActionTypeForTarget(target: var) : var {
        if (!target || !target.element) {
            return -1;
        }
        switch (Math.floor(target.element.buttonId || 0)) {
        case 19:
            return root.lr2ReplayType;
        case 316:
        case 317:
        case 318:
            return target.element.replayType;
        default:
            return -1;
        }
    }

    function replayTooltipTargetAt(x: var, y: var) : var {
        const target = buttonAt(x, y);
        if (!target || target.kind !== "button" || !target.element) {
            return null;
        }

        const label = replayLabel(replayTypeForTarget(target));
        if (label.length <= 0) {
            return null;
        }

        const state = target.state || elementState(target.element);
        if (!state) {
            return null;
        }

        const left = Math.min(state.x, state.x + state.w) * skinScale;
        const top = Math.min(state.y, state.y + state.h) * skinScale;
        const width = Math.abs(state.w) * skinScale;
        const height = Math.abs(state.h) * skinScale;
        return {
            text: label,
            x: left,
            y: top,
            w: width,
            h: height
        };
    }

    function clickReplayType(replayType: var, mouse: var) : var {
        if (root.launchLr2RankingReplayType(replayType, mouse.button)) {
            return true;
        }
        let targetItem = selectContext.activationItem();
        let replayScore = selectContext.replayScoreForType(targetItem, replayType);
        if (!replayScore) {
            return false;
        }
        if (mouse.button === Qt.RightButton) {
            selectContext.openReplayResult(targetItem, replayScore);
            return true;
        }
        root.selectGoForward(targetItem, false, mouse.button !== Qt.MiddleButton, replayScore);
        return true;
    }

    function updateSlider(target: var, x: var, y: var) : var {
        if (!ready || !target || target.kind !== "slider") {
            return;
        }
        const skinX = x / skinScale;
        const skinY = y / skinScale;
        if (target.element.selectScroll) {
            if (sliderState) {
                sliderState.setSelectScrollFromTrack(target.element.src, target.state, skinX, skinY);
            }
        } else if (target.element.genericSlider) {
            if (sliderState) {
                sliderState.setGenericFromTrack(target.element.src, target.state, skinX, skinY);
            }
        }
    }

    function finishSlider(target: var) : var {
        if (!ready || !selectReady || !target || target.kind !== "slider" || !target.element.selectScroll) {
            return;
        }
        selectContext.finishScrollFixedPoint(100);
        if (sliderState) {
            sliderState.selectSliderFixedPoint = -1;
        }
    }

    function clickButton(target: var, mouse: var) : var {
        if (!ready || !target || !selectPanelController) {
            return;
        }
        const state = target.state || elementState(target.element);
        if (!state) {
            return;
        }
        const left = Math.min(state.x, state.x + state.w);
        const width = Math.abs(state.w);
        const delta = selectPanelController.buttonMouseDelta(
            target.element.src,
            target.skinX - left,
            width);
        const replayType = replayActionTypeForTarget(target);
        if (replayType >= 0) {
            root.resetSelectSearch();
            clickReplayType(replayType, mouse);
            mouse.accepted = true;
            return;
        }
        selectPanelController.handleLr2Button(
            target.element.buttonId,
            delta,
            selectPanelController.elementButtonPanel(target.element.src),
            undefined,
            target.element.sourceCount,
            mouse.button);
        mouse.accepted = true;
    }

    function sameTarget(a: var, b: var) : var {
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
