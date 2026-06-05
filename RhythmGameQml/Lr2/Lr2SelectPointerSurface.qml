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
    onActiveChanged: requestReplayTooltipRefresh()
    onRuntimeActiveOptionsChanged: requestReplayTooltipRefresh()

    function clearReplayTooltip() : void {
        replayTooltipShowTimer.stop();
        replayTooltipPopup.displayText = "";
        pointerSurface.replayTooltipTarget = null;
    }

    function requestReplayTooltipRefresh() : void {
        pointerSurface.clearReplayTooltip();
        if (pointerSurface.active
                && mouseArea.containsMouse
                && mouseArea.pressedTarget.kind === "none") {
            replayTooltipShowTimer.restart();
        }
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

        onContainsMouseChanged: pointerSurface.requestReplayTooltipRefresh()

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
                    && (mouse.button === Qt.LeftButton || mouse.button === Qt.RightButton);
                return;
            }

            mouse.accepted = pressedTarget.kind === "button";
        }

        onPositionChanged: (mouse) => {
            pointerSurface.requestReplayTooltipRefresh();
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
            pointerSurface.requestReplayTooltipRefresh();
            if (target.kind === "button") {
                if (!pointerSurface.pointerController.sameTarget(target, releasedTarget)) {
                    mouse.accepted = false;
                    return;
                }
                pointerSurface.pointerController.clickButton(target, mouse);
                return;
            }

            if (target.kind === "bar") {
                if (mouse.button !== Qt.LeftButton && mouse.button !== Qt.RightButton) {
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
            pointerSurface.requestReplayTooltipRefresh();
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
        parent: pointerSurface
        visible: displayText.length > 0
        text: displayText
        delay: 0
        timeout: -1
        x: {
            const maxX = Math.max(0, pointerSurface.width - width);
            const centered = replayTooltipAnchor.x + replayTooltipAnchor.width / 2 - width / 2;
            return Math.max(0, Math.min(maxX, centered));
        }
        y: {
            const margin = 6;
            const above = replayTooltipAnchor.y - height - margin;
            const below = replayTooltipAnchor.y + replayTooltipAnchor.height + margin;
            const preferred = above >= 0 ? above : below;
            const maxY = Math.max(0, pointerSurface.height - height);
            return Math.max(0, Math.min(maxY, preferred));
        }

        property string displayText: ""
    }

    Timer {
        id: replayTooltipShowTimer
        interval: 500
        repeat: false

        onTriggered: {
            pointerSurface.replayTooltipTarget = pointerSurface.active && mouseArea.containsMouse
                ? pointerSurface.pointerController.replayTooltipTargetAt(mouseArea.mouseX, mouseArea.mouseY)
                : null;
            replayTooltipPopup.displayText = pointerSurface.replayTooltipText;
        }
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
