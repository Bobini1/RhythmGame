import QtQuick
import QtTest
import "../../RhythmGameQml" as RhythmGame

Item {
    id: scene

    width: 3840
    height: 2160

    QtObject {
        id: themeVars

        property real legacySkinCustomizePanelXNormalized: 0.2
        property real legacySkinCustomizePanelYNormalized: 0.2
        property real legacySkinCustomizePanelWidthNormalized: 0.2
        property real legacySkinCustomizePanelHeightNormalized: 0.3
    }

    Item {
        id: viewport

        anchors.fill: parent

        RhythmGame.LegacySkinCustomizePlacementFrame {
            id: frame

            themeVars: themeVars
            viewport: viewport
            moveHandle: dragHandle

            Rectangle {
                id: dragHandle

                width: parent.width
                height: 80
            }
        }

    }

    TestCase {
        id: testCase

        name: "LegacyOverlayDrag"
        when: windowShown

        function test_dragDistanceMatchesPointerDistanceAtTwoTimesScale() {
            compare(frame.effectiveContentScale, 2);
            const startX = frame.x;

            let touch = touchEvent(dragHandle);
            touch.press(0, dragHandle, 40, 40).commit();
            touch.move(0, dragHandle, 60, 40).commit();
            touch.move(0, dragHandle, 140, 40).commit();
            verify(frame.moveInteractionActive);
            touch.release(0, dragHandle, 140, 40).commit();

            compare(frame.x - startX, 200);
        }
    }
}
