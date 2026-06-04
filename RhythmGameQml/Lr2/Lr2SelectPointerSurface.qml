pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls

Item {
    id: pointerSurface

    required property var screenRoot
    required property var selectContext
    required property var skinModel
    required property var pointerController
    required property bool active
    readonly property string replayTooltipRevision: {
        const runtimeRevision = pointerSurface.pointerController.skinRuntime
            ? pointerSurface.pointerController.skinRuntime.activeOptionsRevision
            : 0;
        return [
            pointerSurface.screenRoot.selectRevision,
            pointerSurface.screenRoot.selectChartContentRevision,
            runtimeRevision
        ].join("|");
    }
    readonly property var replayTooltipTarget: pointerSurface.replayTooltipRevision.length >= 0
        && pointerSurface.active && mouseArea.containsMouse
        ? pointerSurface.pointerController.replayTooltipTargetAt(mouseArea.mouseX, mouseArea.mouseY)
        : null
    readonly property string replayTooltipText: pointerSurface.replayTooltipTarget
        ? pointerSurface.replayTooltipTarget.text
        : ""
    readonly property string replayTooltipTargetKey: {
        const target = pointerSurface.replayTooltipTarget;
        if (!target || pointerSurface.replayTooltipText.length <= 0) {
            return "";
        }
        return [
            pointerSurface.replayTooltipRevision,
            target.text,
            target.x,
            target.y,
            target.w,
            target.h
        ].join("|");
    }

    onReplayTooltipTargetKeyChanged: {
        replayTooltipShowTimer.stop();
        replayTooltipPopup.displayText = "";
        if (pointerSurface.replayTooltipTargetKey.length > 0) {
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

        onPressed: (mouse) => {
            pressedTarget = pointerSurface.pointerController.targetAt(mouse.x, mouse.y, mouse.button);
            if (pressedTarget.kind === "slider") {
                pointerSurface.screenRoot.clearSelectSearchFocus();
                pointerSurface.screenRoot.selectSliderFixedPoint = -1;
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
                pointerSurface.screenRoot.handleBarRowClick(target.row, mouse);
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
            if (pointerSurface.replayTooltipTargetKey.length <= 0) {
                return;
            }
            replayTooltipPopup.displayText = pointerSurface.replayTooltipText;
        }
    }

    function updateHoverPoint() : void {
        let hoverPoint = hoverHandler.point;
        if (!hoverPoint || !hoverPoint.position) {
            return;
        }

        if (pointerSurface.screenRoot.updateSelectHoverPoint(hoverPoint.position.x, hoverPoint.position.y)) {
            pointerSurface.screenRoot.refreshSelectHoverState();
        }
    }

    HoverHandler {
        id: hoverHandler
        enabled: pointerSurface.screenRoot.selectHoverTracking
            && pointerSurface.screenRoot.selectHoverElementCount > 0
        target: pointerSurface

        onHoveredChanged: {
            if (!hovered) {
                pointerSurface.screenRoot.clearSelectHoverPoint();
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
