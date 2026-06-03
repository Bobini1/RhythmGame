pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

Item {
    id: pointerSurface

    required property var screenRoot
    required property var selectContext
    required property var skinModel
    required property var pointerController
    required property bool active

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: pointerSurface.active
        visible: enabled
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        propagateComposedEvents: true
        preventStealing: true

        property var pressedTarget: ({ kind: "none" })

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton
                    && pointerSurface.screenRoot.effectiveScreenKey === "select"
                    && pointerSurface.screenRoot.focusSelectSearchAt(mouse.x, mouse.y)) {
                pressedTarget = { kind: "search" };
                mouse.accepted = true;
                return;
            }

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

            if (target.kind === "search") {
                pressedTarget = { kind: "none" };
                mouse.accepted = true;
                return;
            }

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

        onWheel: (wheel) => {
            if (pointerSurface.screenRoot.effectiveScreenKey !== "select") {
                wheel.accepted = false;
                return;
            }
            pointerSurface.screenRoot.handleSelectWheel(wheel);
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
