import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaOverlayPlacement"
    when: windowShown
    width: 1400
    height: 900
    visible: true

    Component {
        id: viewportComponent

        Item {
            id: viewportRoot

            property int backgroundTapCount: 0

            Rectangle {
                anchors.fill: parent
                color: "transparent"

                TapHandler {
                    onTapped: viewportRoot.backgroundTapCount += 1
                }
            }
        }
    }

    Component {
        id: themeVarsComponent

        FakeArenaThemeVars {
        }
    }

    Component {
        id: frameComponent

        ArenaOverlayPlacementFrame {
        }
    }

    Component {
        id: selectFrameComponent

        ArenaOverlayPlacementFrame {
            property alias suppliedTitleHandle: titleHandle

            placementKind: "selectRoom"
            layoutVariant: "select"
            customizeMode: false
            defaultPixelRectHint: Qt.rect(728, 120, 520, 480)
            directMoveEnabled: true
            directResizeEnabled: true
            minimumPixelSize: Qt.size(520, 320)
            minimumScreenPixelSize: Qt.size(280, 312)
            moveHandle: titleHandle

            Item {
                id: titleHandle

                objectName: "arenaSelectMoveHandle"
                x: 24
                y: 16
                width: parent.width - 48
                height: 48
            }
        }
    }

    function closeEnough(actual, expected, epsilon = 0.01) {
        verify(Math.abs(actual - expected) <= epsilon,
               "Expected " + actual + " to be within " + epsilon + " of " + expected);
    }

    function safeMargin(length) {
        return Math.min(24, Math.max(0, (length - 1) / 2));
    }

    function verifyInsideSafeRect(frame, viewport) {
        const marginX = safeMargin(viewport.width);
        const marginY = safeMargin(viewport.height);
        verify(frame.x >= marginX - 0.01);
        verify(frame.y >= marginY - 0.01);
        verify(frame.x + frame.width <= viewport.width - marginX + 0.01);
        verify(frame.y + frame.height <= viewport.height - marginY + 0.01);
    }

    function createHarness(viewportWidth, viewportHeight,
                           placementKind = "gameplayLeaderboard",
                           record = ({ "stored": false })) {
        const viewport = createTemporaryObject(viewportComponent, testCase, {
            "width": viewportWidth,
            "height": viewportHeight
        });
        verify(viewport !== null);
        const stored = record && record.stored === true;
        const result = placementKind === "resultStandings";
        const prefix = result ? "arenaOverlayResult" : "arenaOverlayK7";
        let initialValues = {};
        initialValues[prefix + "XNormalized"] = stored ? record.x : -1;
        initialValues[prefix + "YNormalized"] = stored ? record.y : -1;
        initialValues[prefix + "WidthNormalized"] = stored ? record.width : -1;
        initialValues[prefix + "HeightNormalized"] = stored ? record.height : -1;
        const themeVars = createTemporaryObject(themeVarsComponent, testCase,
                                                initialValues);
        verify(themeVars !== null);
        const frame = createTemporaryObject(frameComponent, viewport, {
            "themeVars": themeVars,
            "viewport": viewport,
            "placementKind": placementKind,
            "layoutVariant": result ? "result" : "k7",
            "customizeMode": true
        });
        verify(frame !== null);
        frame.placementCommitted.connect(function() {
            themeVars.commitCount += 1;
        });
        wait(1);
        themeVars.beginTracking();
        return { "viewport": viewport, "themeVars": themeVars, "frame": frame };
    }

    function test_default_geometry_and_forced_visibility() {
        let gameplay = createHarness(1024, 768);
        closeEnough(gameplay.frame.x, 692.8);
        closeEnough(gameplay.frame.y, 24);
        closeEnough(gameplay.frame.width, 307.2);
        closeEnough(gameplay.frame.height, 337.92);
        compare(gameplay.frame.forcedVisible, true);
        compare(gameplay.frame.visible, true);
        gameplay.frame.customizeMode = false;
        compare(gameplay.frame.forcedVisible, true);
        compare(gameplay.frame.visible, true);

        let result = createHarness(1024, 768, "resultStandings");
        closeEnough(result.frame.x, 590.4);
        closeEnough(result.frame.y, 24);
        closeEnough(result.frame.width, 409.6);
        closeEnough(result.frame.height, 460.8);
    }

    function test_select_direct_move_and_invisible_resize() {
        const viewport = createTemporaryObject(viewportComponent, testCase, {
            "width": 1600,
            "height": 900
        });
        verify(viewport !== null);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase);
        verify(themeVars !== null);
        const frame = createTemporaryObject(selectFrameComponent, viewport, {
            "themeVars": themeVars,
            "viewport": viewport
        });
        verify(frame !== null);
        frame.placementCommitted.connect(function() {
            themeVars.commitCount += 1;
        });
        wait(1);
        themeVars.beginTracking();

        compare(frame.resolvedPixelRect, Qt.rect(728, 120, 520, 480));
        verify(frame.directMoveEnabled);
        verify(frame.directResizeEnabled);
        const top = findChild(frame, "arenaResizeTop");
        const right = findChild(frame, "arenaResizeRight");
        const bottom = findChild(frame, "arenaResizeBottom");
        const rightChrome = findChild(frame, "arenaResizeRightChrome");
        verify(top !== null);
        verify(right !== null);
        verify(bottom !== null);
        verify(rightChrome !== null);
        compare(right.visible, true);
        compare(right.enabled, true);
        compare(right.activeFocusOnTab, false);
        compare(rightChrome.visible, false);
        compare(top.height, 16);
        verify(top.width >= frame.width - 32);
        compare(right.width, 16);
        verify(right.height >= frame.height - 32);
        compare(bottom.height, 16);
        verify(bottom.width >= frame.width - 32);

        const initialRect = Qt.rect(frame.x, frame.y,
                                    frame.width, frame.height);
        let interactionStates = [];
        frame.interactionStateChanged.connect(function(active) {
            interactionStates.push(active);
        });
        mousePress(frame, frame.width / 2, frame.height / 2,
                   Qt.LeftButton);
        mouseMove(frame, frame.width / 2 - 60,
                  frame.height / 2 + 30, 10);
        mouseRelease(frame, frame.width / 2 - 60,
                     frame.height / 2 + 30, Qt.LeftButton);
        wait(1);
        compare(frame.resolvedPixelRect, initialRect);
        compare(themeVars.commitCount, 0);
        compare(interactionStates.length, 0);

        const titleHandle = frame.suppliedTitleHandle;
        mousePress(titleHandle, titleHandle.width / 2,
                   titleHandle.height / 2, Qt.LeftButton);
        mouseMove(titleHandle, titleHandle.width / 2 - 20,
                  titleHandle.height / 2 + 10, 10);
        mouseMove(titleHandle, titleHandle.width / 2 - 80,
                  titleHandle.height / 2 + 40, 10);
        mouseRelease(titleHandle, titleHandle.width / 2 - 80,
                     titleHandle.height / 2 + 40, Qt.LeftButton);
        wait(1);
        verify(frame.resolvedPixelRect.x < initialRect.x,
               "Expected title drag to move x from " + initialRect.x
               + ", got " + frame.resolvedPixelRect.x
               + "; interaction states: " + interactionStates.join(","));
        verify(frame.resolvedPixelRect.y > initialRect.y);
        compare(themeVars.commitCount, 1);
        verify(themeVars.arenaOverlaySelectXNormalized >= 0);
        verify(themeVars.arenaOverlaySelectYNormalized >= 0);
        verify(themeVars.arenaOverlaySelectWidthNormalized > 0);
        verify(themeVars.arenaOverlaySelectHeightNormalized > 0);

        const widthAfterMove = frame.resolvedPixelRect.width;
        mousePress(right, right.width / 2, 24,
                   Qt.LeftButton);
        mouseMove(right, right.width / 2 + 20,
                   24, 10);
        mouseMove(right, right.width / 2 + 60,
                   24, 10);
        mouseRelease(right, right.width / 2 + 60,
                     24, Qt.LeftButton);
        wait(1);
        verify(frame.resolvedPixelRect.width > widthAfterMove);
        compare(themeVars.commitCount, 2);

        const heightAfterRightResize = frame.resolvedPixelRect.height;
        mousePress(bottom, 24, bottom.height / 2, Qt.LeftButton);
        mouseMove(bottom, 24, bottom.height / 2 + 20, 10);
        mouseMove(bottom, 24, bottom.height / 2 + 60, 10);
        mouseRelease(bottom, 24, bottom.height / 2 + 60,
                     Qt.LeftButton);
        wait(1);
        verify(frame.resolvedPixelRect.height > heightAfterRightResize);
        compare(themeVars.commitCount, 3);
        compare(themeVars.arenaOverlayK7XNormalized, -1);
        compare(themeVars.arenaOverlayK7YNormalized, -1);
        compare(themeVars.arenaOverlayK7WidthNormalized, -1);
        compare(themeVars.arenaOverlayK7HeightNormalized, -1);
        compare(themeVars.arenaOverlayResultXNormalized, -1);
        compare(themeVars.arenaOverlayResultYNormalized, -1);
        compare(themeVars.arenaOverlayResultWidthNormalized, -1);
        compare(themeVars.arenaOverlayResultHeightNormalized, -1);

        const persistedRect = frame.resolvedPixelRect;
        const reloadedFrame = createTemporaryObject(selectFrameComponent,
                                                    viewport, {
            "themeVars": themeVars,
            "viewport": viewport
        });
        verify(reloadedFrame !== null);
        tryCompare(reloadedFrame, "resolvedPixelRect", persistedRect);
    }

    function test_stored_conversion_safe_clamp_and_passive_resize() {
        const harness = createHarness(1024, 768, "gameplayLeaderboard", {
            "stored": true,
            "x": 0.65,
            "y": 0.03,
            "width": 0.30,
            "height": 0.50
        });
        closeEnough(harness.frame.x, 665.6);
        closeEnough(harness.frame.y, 24);
        closeEnough(harness.frame.width, 307.2);
        closeEnough(harness.frame.height, 384);
        compare(harness.themeVars.commitCount, 0);
        compare(harness.themeVars.writeCount, 0);

        harness.viewport.width = 640;
        harness.viewport.height = 360;
        wait(1);
        verifyInsideSafeRect(harness.frame, harness.viewport);
        compare(harness.themeVars.commitCount, 0);
        compare(harness.themeVars.writeCount, 0);

        harness.viewport.width = 1024;
        harness.viewport.height = 768;
        wait(1);
        closeEnough(harness.frame.x, 665.6);
        closeEnough(harness.frame.y, 24);
        closeEnough(harness.frame.width, 307.2);
        closeEnough(harness.frame.height, 384);
        compare(harness.themeVars.commitCount, 0);
        compare(harness.themeVars.writeCount, 0);
    }

    function test_select_stored_placement_shrinks_with_short_viewport() {
        const viewport = createTemporaryObject(viewportComponent, testCase, {
            "width": 1920,
            "height": 1080
        });
        verify(viewport !== null);
        const themeVars = createTemporaryObject(themeVarsComponent, testCase, {
            "arenaOverlaySelectXNormalized": 0.1,
            "arenaOverlaySelectYNormalized": 0.1,
            "arenaOverlaySelectWidthNormalized": 520 / 1920,
            "arenaOverlaySelectHeightNormalized": 480 / 1080
        });
        verify(themeVars !== null);
        const frame = createTemporaryObject(selectFrameComponent, viewport, {
            "themeVars": themeVars,
            "viewport": viewport
        });
        verify(frame !== null);
        wait(1);
        themeVars.beginTracking();

        closeEnough(frame.width, 520);
        closeEnough(frame.height, 480);

        viewport.width = 1098;
        viewport.height = 416;
        wait(1);

        closeEnough(frame.x, 109.8);
        closeEnough(frame.y, 41.6);
        closeEnough(frame.width, 297.375);
        closeEnough(frame.height, 312);
        verifyInsideSafeRect(frame, viewport);
        compare(themeVars.commitCount, 0);
        compare(themeVars.writeCount, 0);

        viewport.width = 1920;
        viewport.height = 1080;
        wait(1);

        closeEnough(frame.x, 192);
        closeEnough(frame.y, 108);
        closeEnough(frame.width, 520);
        closeEnough(frame.height, 480);
        compare(themeVars.commitCount, 0);
        compare(themeVars.writeCount, 0);
    }

    function test_minimum_and_all_supported_viewports() {
        const viewports = [[1024, 768], [1280, 720], [1920, 1080],
                           [2560, 1080], [3840, 2160]];
        for (const dimensions of viewports) {
            let harness = createHarness(dimensions[0], dimensions[1]);
            verify(harness.frame.width >= 280);
            verify(harness.frame.height >= 160);
            verifyInsideSafeRect(harness.frame, harness.viewport);

            let result = createHarness(dimensions[0], dimensions[1],
                                       "resultStandings");
            verify(result.frame.width >= 280);
            verify(result.frame.height >= 160);
            verifyInsideSafeRect(result.frame, result.viewport);
        }

        let small = createHarness(250, 140);
        closeEnough(small.frame.width, 202);
        closeEnough(small.frame.height, 92);
        verifyInsideSafeRect(small.frame, small.viewport);
    }

    function test_move_commits_once_at_drag_end() {
        const harness = createHarness(1280, 720);
        const oldX = harness.frame.x;
        let interactionStates = [];
        harness.frame.interactionStateChanged.connect(function(active) {
            interactionStates.push(active);
        });
        mousePress(harness.frame, harness.frame.width / 2,
                   harness.frame.height / 2, Qt.LeftButton);
        mouseMove(harness.frame, harness.frame.width / 2 - 20,
                  harness.frame.height / 2 + 15, 10);
        mouseMove(harness.frame, harness.frame.width / 2 - 80,
                  harness.frame.height / 2 + 60, 10);
        compare(harness.themeVars.commitCount, 0);
        compare(harness.themeVars.writeCount, 0);
        mouseRelease(harness.frame, harness.frame.width / 2 - 80,
                     harness.frame.height / 2 + 60, Qt.LeftButton);
        wait(1);
        compare(harness.themeVars.commitCount, 1);
        compare(harness.themeVars.writeCount, 4);
        verify(Math.abs(harness.frame.x - oldX) > 1,
               "Move gesture must change the frame position");
        verifyInsideSafeRect(harness.frame, harness.viewport);
        compare(harness.frame.interactionActive, false);
        compare(interactionStates.length, 2);
        compare(interactionStates[0], true);
        compare(interactionStates[1], false);
    }

    function test_all_eight_resize_handles_commit_once() {
        const handles = [
            "arenaResizeTopLeft", "arenaResizeTop", "arenaResizeTopRight",
            "arenaResizeRight", "arenaResizeBottomRight", "arenaResizeBottom",
            "arenaResizeBottomLeft", "arenaResizeLeft"
        ];
        for (const objectName of handles) {
            const harness = createHarness(1280, 720, "gameplayLeaderboard", {
                "stored": true,
                "x": 0.25,
                "y": 0.20,
                "width": 0.40,
                "height": 0.45
            });
            const handle = findChild(harness.frame, objectName);
            verify(handle !== null, objectName);
            const horizontalSide = objectName === "arenaResizeTop"
                || objectName === "arenaResizeBottom";
            const verticalSide = objectName === "arenaResizeLeft"
                || objectName === "arenaResizeRight";
            if (horizontalSide) {
                verify(handle.width >= harness.frame.width - 32, objectName);
                compare(handle.height, 32, objectName);
            } else if (verticalSide) {
                compare(handle.width, 32, objectName);
                verify(handle.height >= harness.frame.height - 32,
                       objectName);
            } else {
                compare(handle.width, 32, objectName);
                compare(handle.height, 32, objectName);
            }
            verify(handle.Accessible.name.length > 0, objectName);
            verify(handle.Accessible.description.length > 0, objectName);
            compare(handle.Accessible.role, Accessible.Grip, objectName);
            compare(handle.Accessible.focusable, true, objectName);
            compare(handle.activeFocusOnTab, true, objectName);
            handle.forceActiveFocus();
            tryCompare(handle, "activeFocus", true);
            compare(handle.focusIndicatorVisible, true, objectName);
            const oldLeft = harness.frame.x;
            const oldTop = harness.frame.y;
            const oldRight = harness.frame.x + harness.frame.width;
            const oldBottom = harness.frame.y + harness.frame.height;
            mousePress(handle, handle.width / 2, handle.height / 2,
                       Qt.LeftButton);
            mouseMove(handle, handle.width / 2 + 20,
                      handle.height / 2 + 15, 10);
            mouseMove(handle, handle.width / 2 + 60,
                      handle.height / 2 + 45, 10);
            compare(harness.themeVars.commitCount, 0, objectName);
            compare(harness.themeVars.writeCount, 0, objectName);
            mouseRelease(handle, handle.width / 2 + 60,
                         handle.height / 2 + 45, Qt.LeftButton);
            wait(1);
            compare(harness.themeVars.commitCount, 1, objectName);
            const expectedWrites = {
                "arenaResizeTopLeft": 4,
                "arenaResizeTop": 2,
                "arenaResizeTopRight": 3,
                "arenaResizeRight": 1,
                "arenaResizeBottomRight": 2,
                "arenaResizeBottom": 1,
                "arenaResizeBottomLeft": 3,
                "arenaResizeLeft": 2
            };
            compare(harness.themeVars.writeCount,
                    expectedWrites[objectName], objectName);
            verifyInsideSafeRect(harness.frame, harness.viewport);
            const newLeft = harness.frame.x;
            const newTop = harness.frame.y;
            const newRight = harness.frame.x + harness.frame.width;
            const newBottom = harness.frame.y + harness.frame.height;
            if (objectName.indexOf("Left") >= 0) {
                verify(newLeft > oldLeft,
                       objectName + " expected left " + newLeft
                       + " > " + oldLeft);
                closeEnough(newRight, oldRight);
            } else if (objectName.indexOf("Right") >= 0) {
                closeEnough(newLeft, oldLeft);
                verify(newRight > oldRight,
                       objectName + " expected right " + newRight
                       + " > " + oldRight);
            } else {
                closeEnough(newLeft, oldLeft);
                closeEnough(newRight, oldRight);
            }
            if (objectName.indexOf("Top") >= 0) {
                verify(newTop > oldTop,
                       objectName + " expected top " + newTop
                       + " > " + oldTop);
                closeEnough(newBottom, oldBottom);
            } else if (objectName.indexOf("Bottom") >= 0) {
                closeEnough(newTop, oldTop);
                verify(newBottom > oldBottom,
                       objectName + " expected bottom " + newBottom
                       + " > " + oldBottom);
            } else {
                closeEnough(newTop, oldTop);
                closeEnough(newBottom, oldBottom);
            }
        }
    }

    function test_passive_mode_is_pointer_transparent() {
        const harness = createHarness(1280, 720);
        const original = Qt.rect(harness.frame.x, harness.frame.y,
                                 harness.frame.width, harness.frame.height);
        harness.frame.customizeMode = false;
        compare(harness.frame.activeFocusOnTab, false);
        const handle = findChild(harness.frame, "arenaResizeBottomRight");
        compare(handle.visible, false);
        compare(handle.enabled, false);
        compare(handle.activeFocusOnTab, false);
        compare(handle.Accessible.focusable, false);
        mousePress(harness.frame, harness.frame.width / 2,
                   harness.frame.height / 2, Qt.LeftButton);
        mouseMove(harness.frame, harness.frame.width / 2 - 80,
                  harness.frame.height / 2 - 40, 10);
        mouseRelease(harness.frame, harness.frame.width / 2 - 80,
                     harness.frame.height / 2 - 40, Qt.LeftButton);
        wait(1);
        closeEnough(harness.frame.x, original.x);
        closeEnough(harness.frame.y, original.y);
        closeEnough(harness.frame.width, original.width);
        closeEnough(harness.frame.height, original.height);
        compare(harness.themeVars.commitCount, 0);
        compare(harness.themeVars.writeCount, 0);

        mouseClick(harness.viewport,
                   harness.frame.x + harness.frame.width / 2,
                   harness.frame.y + harness.frame.height / 2,
                   Qt.LeftButton);
        compare(harness.viewport.backgroundTapCount, 1);
    }

    function test_keyboard_resize_enforces_minimum_and_safe_edges() {
        const harness = createHarness(1280, 720);
        const frame = harness.frame;
        compare(frame.activeFocusOnTab, true);
        frame.forceActiveFocus();
        for (let index = 0; index < 100; ++index) {
            keyClick(Qt.Key_Left, Qt.AltModifier);
            keyClick(Qt.Key_Up, Qt.AltModifier);
        }
        closeEnough(frame.width, 280);
        closeEnough(frame.height, 160);

        for (let index = 0; index < 200; ++index) {
            keyClick(Qt.Key_Right, Qt.AltModifier | Qt.ShiftModifier);
            keyClick(Qt.Key_Down, Qt.AltModifier | Qt.ShiftModifier);
        }
        closeEnough(frame.x + frame.width, 1256);
        closeEnough(frame.y + frame.height, 696);
        verifyInsideSafeRect(frame, harness.viewport);
    }

    function test_keyboard_move_resize_reset_and_exit() {
        const harness = createHarness(1280, 720);
        const frame = harness.frame;
        frame.forceActiveFocus();
        verify(frame.activeFocus);
        const originalX = frame.x;
        const originalY = frame.y;
        const originalWidth = frame.width;
        const originalHeight = frame.height;

        keyClick(Qt.Key_Left);
        closeEnough(frame.x, originalX - 4);
        compare(harness.themeVars.commitCount, 1);
        compare(harness.themeVars.writeCount, 4);
        keyClick(Qt.Key_Down, Qt.ShiftModifier);
        closeEnough(frame.y, originalY + 16);
        compare(harness.themeVars.commitCount, 2);
        compare(harness.themeVars.writeCount, 5);
        keyClick(Qt.Key_Left, Qt.AltModifier);
        closeEnough(frame.width, originalWidth - 4);
        compare(harness.themeVars.commitCount, 3);
        compare(harness.themeVars.writeCount, 6);
        keyClick(Qt.Key_Down, Qt.AltModifier | Qt.ShiftModifier);
        closeEnough(frame.height, originalHeight + 16);
        compare(harness.themeVars.commitCount, 4);
        compare(harness.themeVars.writeCount, 7);

        let exits = 0;
        frame.requestExitCustomization.connect(function() { exits += 1; });
        keyClick(Qt.Key_Return);
        keyClick(Qt.Key_Escape);
        compare(exits, 2);

        keyClick(Qt.Key_R);
        compare(harness.themeVars.commitCount, 5);
        compare(harness.themeVars.writeCount, 11);
        compare(harness.themeVars.arenaOverlayK7XNormalized, -1);
        compare(harness.themeVars.arenaOverlayK7YNormalized, -1);
        compare(harness.themeVars.arenaOverlayK7WidthNormalized, -1);
        compare(harness.themeVars.arenaOverlayK7HeightNormalized, -1);
        closeEnough(frame.x, 872);
        closeEnough(frame.y, 24);
        closeEnough(frame.width, 384);
        closeEnough(frame.height, 316.8);

        const resetButton = findChild(frame, "arenaOverlayReset");
        verify(resetButton !== null);
        verify(resetButton.width >= 32);
        verify(resetButton.height >= 32);
        mouseClick(resetButton, resetButton.width / 2, resetButton.height / 2,
                   Qt.LeftButton);
        compare(harness.themeVars.commitCount, 6);
        compare(harness.themeVars.writeCount, 11);
    }

    function test_resize_handles_support_keyboard_and_accessibility_actions() {
        const harness = createHarness(1280, 720, "gameplayLeaderboard", {
            "stored": true,
            "x": 0.25,
            "y": 0.20,
            "width": 0.40,
            "height": 0.45
        });
        const right = findChild(harness.frame, "arenaResizeRight");
        verify(right !== null);
        right.forceActiveFocus();
        tryCompare(right, "activeFocus", true);

        const originalX = harness.frame.x;
        const originalWidth = harness.frame.width;
        const originalRight = originalX + originalWidth;
        const originalTop = harness.frame.y;
        keyClick(Qt.Key_Right);
        closeEnough(harness.frame.x, originalX);
        closeEnough(harness.frame.width, originalWidth + 4);
        closeEnough(harness.frame.x + harness.frame.width,
                    originalRight + 4);
        closeEnough(harness.frame.y, originalTop);
        compare(harness.themeVars.commitCount, 1);

        keyClick(Qt.Key_Up);
        closeEnough(harness.frame.y, originalTop);
        closeEnough(harness.frame.x + harness.frame.width,
                    originalRight + 4);
        compare(harness.themeVars.commitCount, 1);

        keyClick(Qt.Key_Left, Qt.ShiftModifier);
        closeEnough(harness.frame.x + harness.frame.width,
                    originalRight - 12);
        compare(harness.themeVars.commitCount, 2);

        const widthBeforeAction = harness.frame.width;
        right.Accessible.increaseAction();
        closeEnough(harness.frame.width, widthBeforeAction + 4);
        compare(harness.themeVars.commitCount, 3);
        right.Accessible.decreaseAction();
        closeEnough(harness.frame.width, widthBeforeAction);
        compare(harness.themeVars.commitCount, 4);

        const bottomLeft = findChild(harness.frame,
                                     "arenaResizeBottomLeft");
        verify(bottomLeft !== null);
        bottomLeft.forceActiveFocus();
        const oldLeft = harness.frame.x;
        const oldRight = harness.frame.x + harness.frame.width;
        const oldBottom = harness.frame.y + harness.frame.height;
        keyClick(Qt.Key_Left, Qt.ShiftModifier);
        closeEnough(harness.frame.x, oldLeft - 16);
        closeEnough(harness.frame.x + harness.frame.width, oldRight);
        compare(harness.themeVars.commitCount, 5);
        keyClick(Qt.Key_Down);
        closeEnough(harness.frame.y + harness.frame.height,
                    oldBottom + 4);
        compare(harness.themeVars.commitCount, 6);
        verifyInsideSafeRect(harness.frame, harness.viewport);
    }

    function test_malformed_theme_values_use_default_geometry() {
        const baseline = createHarness(1000, 700);
        const malformedString = createHarness(1000, 700,
                                               "gameplayLeaderboard", {
            "stored": true,
            "x": "0.1",
            "y": 0.1,
            "width": 0.4,
            "height": 0.4
        });
        const malformedBoolean = createHarness(1000, 700,
                                                "gameplayLeaderboard", {
            "stored": true,
            "x": false,
            "y": 0.1,
            "width": 0.4,
            "height": 0.4
        });
        for (const harness of [malformedString, malformedBoolean]) {
            closeEnough(harness.frame.x, baseline.frame.x);
            closeEnough(harness.frame.y, baseline.frame.y);
            closeEnough(harness.frame.width, baseline.frame.width);
            closeEnough(harness.frame.height, baseline.frame.height);
            compare(harness.themeVars.writeCount, 0);
        }
    }

    function test_profile_theme_vars_replacement_reloads_geometry() {
        const harness = createHarness(1000, 700);
        const originalX = harness.frame.x;
        const profileB = createTemporaryObject(themeVarsComponent, testCase, {
            "arenaOverlayK7XNormalized": 0.1,
            "arenaOverlayK7YNormalized": 0.1,
            "arenaOverlayK7WidthNormalized": 0.4,
            "arenaOverlayK7HeightNormalized": 0.4
        });
        verify(profileB !== null);
        harness.frame.themeVars = profileB;
        wait(1);
        closeEnough(harness.frame.x, 100);
        closeEnough(harness.frame.y, 70);

        const profileC = createTemporaryObject(themeVarsComponent, testCase, {
            "arenaOverlayK7XNormalized": 0.2,
            "arenaOverlayK7YNormalized": 0.2,
            "arenaOverlayK7WidthNormalized": 0.3,
            "arenaOverlayK7HeightNormalized": 0.3
        });
        verify(profileC !== null);
        harness.frame.themeVars = profileC;
        wait(1);
        closeEnough(harness.frame.x, 200);
        closeEnough(harness.frame.y, 140);

        harness.frame.themeVars = harness.themeVars;
        wait(1);
        closeEnough(harness.frame.x, originalX);
    }
}
