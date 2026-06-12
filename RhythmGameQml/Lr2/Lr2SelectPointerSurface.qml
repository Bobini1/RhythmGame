pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls

Item {
    id: pointerSurface

    required property var screenRoot
    required property var selectContext
    required property var skinModel
    required property var pointerController
    required property var sliderState
    required property var selectHoverState
    required property bool active
    readonly property var runtimeActiveOptions: pointerSurface.pointerController.skinRuntime
        ? pointerSurface.pointerController.skinRuntime.runtimeActiveOptions
        : []
    property var replayTooltipTarget: null
    readonly property string replayTooltipText: pointerSurface.replayTooltipTarget
        ? pointerSurface.replayTooltipTarget.text
        : ""
    property var replayTooltipPendingTarget: null
    property bool replayTooltipRefreshPending: false
    property bool replayTooltipResetDelayPending: false
    property bool replayTooltipPointerValid: false
    property real replayTooltipPointerX: 0
    property real replayTooltipPointerY: 0
    onActiveChanged: {
        if (active) {
            requestReplayTooltipRefresh(true);
        } else {
            clearReplayTooltip();
        }
    }
    onRuntimeActiveOptionsChanged: requestReplayTooltipRefresh(true)

    function clearReplayTooltip() : void {
        pointerSurface.replayTooltipRefreshPending = false;
        pointerSurface.replayTooltipResetDelayPending = false;
        pointerSurface.replayTooltipPointerValid = false;
        replayTooltipShowTimer.stop();
        replayTooltipPopup.displayText = "";
        pointerSurface.replayTooltipTarget = null;
        pointerSurface.replayTooltipPendingTarget = null;
    }

    function replayTooltipTargetsMatch(a: var, b: var) : bool {
        if (!a || !b) {
            return false;
        }
        return a.text === b.text
            && Math.abs((a.x || 0) - (b.x || 0)) < 0.5
            && Math.abs((a.y || 0) - (b.y || 0)) < 0.5
            && Math.abs((a.w || 0) - (b.w || 0)) < 0.5
            && Math.abs((a.h || 0) - (b.h || 0)) < 0.5;
    }

    function currentReplayTooltipTarget(x: var, y: var) : var {
        if (!pointerSurface.active
                || !mouseArea.containsMouse
                || mouseArea.pressedTarget.kind !== "none") {
            return null;
        }
        const targetX = x === undefined ? mouseArea.mouseX : x;
        const targetY = y === undefined ? mouseArea.mouseY : y;
        return pointerSurface.pointerController.replayTooltipTargetAt(targetX, targetY);
    }

    function requestReplayTooltipRefresh(resetDelay: var, x: var, y: var) : void {
        if (x !== undefined && y !== undefined) {
            pointerSurface.replayTooltipPointerX = x;
            pointerSurface.replayTooltipPointerY = y;
            pointerSurface.replayTooltipPointerValid = true;
        } else if (mouseArea.containsMouse) {
            pointerSurface.replayTooltipPointerX = mouseArea.mouseX;
            pointerSurface.replayTooltipPointerY = mouseArea.mouseY;
            pointerSurface.replayTooltipPointerValid = true;
        } else {
            pointerSurface.replayTooltipPointerValid = false;
        }

        pointerSurface.replayTooltipResetDelayPending =
            pointerSurface.replayTooltipResetDelayPending || resetDelay === true;
        pointerSurface.replayTooltipRefreshPending = true;
    }

    function flushReplayTooltipRefresh() : void {
        if (!pointerSurface.replayTooltipRefreshPending) {
            return;
        }

        const forceReset = pointerSurface.replayTooltipResetDelayPending;
        const hasPointer = pointerSurface.replayTooltipPointerValid;
        const pointerX = pointerSurface.replayTooltipPointerX;
        const pointerY = pointerSurface.replayTooltipPointerY;
        pointerSurface.replayTooltipRefreshPending = false;
        pointerSurface.replayTooltipResetDelayPending = false;

        if (!hasPointer) {
            pointerSurface.clearReplayTooltip();
            return;
        }

        const target = pointerSurface.currentReplayTooltipTarget(pointerX, pointerY);
        if (!target) {
            pointerSurface.clearReplayTooltip();
            return;
        }

        if (!forceReset) {
            if (replayTooltipShowTimer.running
                    && pointerSurface.replayTooltipTargetsMatch(target, pointerSurface.replayTooltipPendingTarget)) {
                return;
            }
            if (replayTooltipPopup.displayText.length > 0
                    && pointerSurface.replayTooltipTargetsMatch(target, pointerSurface.replayTooltipTarget)) {
                return;
            }
        }

        replayTooltipShowTimer.stop();
        replayTooltipPopup.displayText = "";
        pointerSurface.replayTooltipTarget = null;
        pointerSurface.replayTooltipPendingTarget = target;
        replayTooltipShowTimer.start();
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: pointerSurface.active
        visible: enabled
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        propagateComposedEvents: true
        preventStealing: true
        hoverEnabled: true

        property var pressedTarget: ({ kind: "none" })

        onContainsMouseChanged: {
            if (containsMouse) {
                pointerSurface.requestReplayTooltipRefresh(true);
            } else {
                pointerSurface.clearReplayTooltip();
            }
        }

        onPressed: (mouse) => {
            pointerSurface.clearReplayTooltip();
            pressedTarget = pointerSurface.pointerController.targetAt(mouse.x, mouse.y, mouse.button);
            if (pressedTarget.kind === "slider") {
                pointerSurface.screenRoot.clearSelectSearchFocus();
                if (pointerSurface.sliderState) {
                    pointerSurface.sliderState.selectSliderFixedPoint = -1;
                }
                if (pressedTarget.element.selectScroll) {
                    pointerSurface.screenRoot.selectScrollStartSkinTime = pointerSurface.screenRoot.renderSkinTime;
                    pointerSurface.selectContext.beginScrollFixedPointDrag();
                }
                pointerSurface.pointerController.updateSlider(pressedTarget, mouse.x, mouse.y);
                mouse.accepted = true;
                return;
            }

            if (pressedTarget.kind === "bar" || pressedTarget.kind === "blank") {
                pointerSurface.screenRoot.clearSelectSearchFocus();
                mouse.accepted = pressedTarget.kind === "bar"
                    && (mouse.button === Qt.LeftButton
                        || mouse.button === Qt.MiddleButton
                        || mouse.button === Qt.RightButton);
                return;
            }

            mouse.accepted = pressedTarget.kind === "button";
        }

        onPositionChanged: (mouse) => {
            pointerSurface.requestReplayTooltipRefresh(false, mouse.x, mouse.y);
            if (pressedTarget.kind !== "slider") {
                return;
            }
            pointerSurface.pointerController.updateSlider(pressedTarget, mouse.x, mouse.y);
            mouse.accepted = true;
        }

        onReleased: (mouse) => {
            const releasedTarget = pointerSurface.pointerController.targetAt(mouse.x, mouse.y, mouse.button);
            const target = pressedTarget.kind !== "none"
                ? pressedTarget
                : releasedTarget;

            if (target.kind === "slider") {
                pointerSurface.pointerController.finishSlider(target);
                pressedTarget = { kind: "none" };
                mouse.accepted = true;
                return;
            }

            pressedTarget = { kind: "none" };
            pointerSurface.requestReplayTooltipRefresh(true);
            if (target.kind === "button") {
                if (!pointerSurface.pointerController.sameTarget(target, releasedTarget)) {
                    mouse.accepted = false;
                    return;
                }
                pointerSurface.pointerController.clickButton(target, mouse);
                return;
            }

            if (target.kind === "bar") {
                if (mouse.button !== Qt.LeftButton
                        && mouse.button !== Qt.MiddleButton
                        && mouse.button !== Qt.RightButton) {
                    mouse.accepted = false;
                    return;
                }
                pointerSurface.pointerController.clickBarRow(target.row, mouse);
                mouse.accepted = true;
                return;
            }

            if (target.kind === "blank") {
                pointerSurface.screenRoot.clearSelectSearchFocus();
                mouse.accepted = false;
            }
        }

        onClicked: (mouse) => {
            const releasedTarget = pointerSurface.pointerController.targetAt(mouse.x, mouse.y, mouse.button);
            mouse.accepted = releasedTarget.kind !== "blank";
        }

        onDoubleClicked: (mouse) => {
            const target = pointerSurface.pointerController.targetAt(mouse.x, mouse.y, mouse.button);
            pressedTarget = { kind: "none" };
            if (target.kind !== "bar" || mouse.button !== Qt.LeftButton) {
                mouse.accepted = false;
                return;
            }

            pointerSurface.selectContext.selectVisibleRow(target.row, pointerSurface.skinModel.barCenter);
            pointerSurface.screenRoot.selectGoForward(pointerSurface.selectContext.activationItem());
            mouse.accepted = true;
        }

        onCanceled: {
            pointerSurface.pointerController.finishSlider(pressedTarget);
            pressedTarget = { kind: "none" };
            pointerSurface.requestReplayTooltipRefresh(true);
        }

        onWheel: (wheel) => pointerSurface.screenRoot.handleSelectWheel(wheel)
    }

    Item {
        id: replayTooltipAnchor
        x: pointerSurface.replayTooltipTarget ? pointerSurface.replayTooltipTarget.x : 0
        y: pointerSurface.replayTooltipTarget ? pointerSurface.replayTooltipTarget.y : 0
        width: pointerSurface.replayTooltipTarget ? pointerSurface.replayTooltipTarget.w : 0
        height: pointerSurface.replayTooltipTarget ? pointerSurface.replayTooltipTarget.h : 0
    }

    ToolTip {
        id: replayTooltipPopup
        parent: pointerSurface.screenRoot
        visible: displayText.length > 0
        text: displayText
        delay: 0
        timeout: -1
        x: {
            const tooltipParent = replayTooltipPopup.parent;
            const anchorCenter = pointerSurface.mapToItem(
                tooltipParent,
                replayTooltipAnchor.x + replayTooltipAnchor.width / 2,
                replayTooltipAnchor.y);
            const parentWidth = tooltipParent ? tooltipParent.width : pointerSurface.width;
            const maxX = Math.max(0, parentWidth - width);
            const centered = anchorCenter.x - width / 2;
            return Math.max(0, Math.min(maxX, centered));
        }
        y: {
            const margin = 6;
            const tooltipParent = replayTooltipPopup.parent;
            const anchorTop = pointerSurface.mapToItem(
                tooltipParent,
                replayTooltipAnchor.x,
                replayTooltipAnchor.y);
            const anchorBottom = pointerSurface.mapToItem(
                tooltipParent,
                replayTooltipAnchor.x,
                replayTooltipAnchor.y + replayTooltipAnchor.height);
            const above = anchorTop.y - height - margin;
            const below = anchorBottom.y + margin;
            const preferred = above >= 0 ? above : below;
            const parentHeight = tooltipParent ? tooltipParent.height : pointerSurface.height;
            const maxY = Math.max(0, parentHeight - height);
            return Math.max(0, Math.min(maxY, preferred));
        }

        property string displayText: ""
    }

    Timer {
        id: replayTooltipShowTimer
        interval: 500
        repeat: false

        onTriggered: {
            const target = pointerSurface.currentReplayTooltipTarget();
            if (!pointerSurface.replayTooltipTargetsMatch(target, pointerSurface.replayTooltipPendingTarget)) {
                pointerSurface.replayTooltipPendingTarget = null;
                pointerSurface.requestReplayTooltipRefresh(false);
                return;
            }

            pointerSurface.replayTooltipPendingTarget = null;
            pointerSurface.replayTooltipTarget = target;
            replayTooltipPopup.displayText = pointerSurface.replayTooltipText;
        }
    }

    FrameAnimation {
        running: pointerSurface.replayTooltipRefreshPending

        onTriggered: pointerSurface.flushReplayTooltipRefresh()
    }

    function updateHoverPoint() : void {
        let hoverPoint = hoverHandler.point;
        if (!hoverPoint || !hoverPoint.position) {
            return;
        }

        if (pointerSurface.selectHoverState) {
            pointerSurface.selectHoverState.updatePoint(hoverPoint.position.x, hoverPoint.position.y);
        }
    }

    HoverHandler {
        id: hoverHandler
        enabled: !!pointerSurface.selectHoverState
            && pointerSurface.selectHoverState.tracking
            && pointerSurface.selectHoverState.elementCount > 0
        target: pointerSurface

        onHoveredChanged: {
            if (!hovered) {
                if (pointerSurface.selectHoverState) {
                    pointerSurface.selectHoverState.clearPoint();
                }
                return;
            }

            pointerSurface.updateHoverPoint();
        }
    }

    FrameAnimation {
        running: hoverHandler.enabled && hoverHandler.hovered

        onTriggered: {
            pointerSurface.updateHoverPoint();
        }
    }
}
