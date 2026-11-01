import QtQuick
import QtTest
import "../../share/RhythmGame/themes/Default/scripts/common/TaoQuickCustom"

TestCase {
    id: tests
    name: "GameplayLayoutEditor"
    width: 1100
    height: 900
    visible: true
    when: windowShown

    Component {
        id: fixture
        Item {
            id: surface
            width: 700
            height: 550
            property bool mirrored: false
            property alias box: box
            property alias editor: editor
            property alias settings: box.settings
            property alias replacement: replacement
            property int edits: 0

            QtObject {
                id: initial
                property real savedX: 150
                property real savedY: 120
                property real savedWidth: 200
                property real savedHeight: 100
            }
            QtObject {
                id: replacement
                property real savedX: 190
                property real savedY: 210
                property real savedWidth: 160
                property real savedHeight: 130
            }
            transform: Scale {
                xScale: surface.mirrored ? -1 : 1
                origin.x: surface.width / 2
            }
            Rectangle {
                id: box
                property QtObject settings: initial
                x: settings.savedX
                y: settings.savedY
                width: settings.savedWidth
                height: settings.savedHeight
                color: "steelblue"
                transform: Scale {
                    xScale: surface.mirrored ? -1 : 1
                    origin.x: box.width / 2
                }
                TemplateDragBorder {
                    id: editor
                    anchors.fill: parent
                    anchors.margins: -borderMargin
                    mirrored: surface.mirrored
                    onGeometryEdited: geometry => {
                        ++surface.edits;
                        box.settings.savedX = geometry.x;
                        box.settings.savedY = geometry.y;
                        box.settings.savedWidth = geometry.width;
                        box.settings.savedHeight = geometry.height;
                    }
                }
            }
        }
    }

    function drag(item, x, y, dx, dy) {
        let point = item.mapToItem(tests, x, y);
        mousePress(tests, point.x, point.y);
        for (let step = 1; step <= 5; ++step)
            mouseMove(tests, point.x + dx * step / 5, point.y + dy * step / 5, 5);
        mouseRelease(tests, point.x + dx, point.y + dy);
    }

    function verifyBindings(surface) {
        let edits = surface.edits;
        surface.settings.savedX = 135;
        surface.settings.savedY = 155;
        surface.settings.savedWidth = 185;
        surface.settings.savedHeight = 95;
        compare(surface.box.x, 135);
        compare(surface.box.y, 155);
        compare(surface.box.width, 185);
        compare(surface.box.height, 95);
        surface.settings = surface.replacement;
        compare(surface.box.x, 190);
        compare(surface.box.y, 210);
        compare(surface.box.width, 160);
        compare(surface.box.height, 130);
        compare(surface.edits, edits);
    }

    function test_move_data() {
        let rows = [];
        for (let scale of [0.5, 1, 1.5])
            for (let mirrored of [false, true])
                rows.push({tag: scale + "/" + mirrored, scale, mirrored});
        return rows;
    }

    function test_move(data) {
        let surface = createTemporaryObject(fixture, tests, {
            scale: data.scale, transformOrigin: Item.TopLeft, mirrored: data.mirrored
        });
        verify(surface);
        compare(surface.edits, 0);
        drag(surface.box, 100, 50, 80, 60);
        verify(surface.edits > 0);
        verify(data.mirrored ? surface.box.x < 150 : surface.box.x > 150);
        verify(surface.box.y > 120);
        compare(surface.box.width, 200);
        compare(surface.box.height, 100);
        verifyBindings(surface);
        let oldY = surface.box.y;
        drag(surface.box, 80, 65, 0, 40);
        verify(surface.box.y > oldY);
    }

    function test_resize_data() {
        let rows = [];
        for (let scale of [0.5, 1, 1.5])
            for (let mirrored of [false, true])
                for (let horizontal of [-1, 0, 1])
                    for (let vertical of [-1, 0, 1])
                        if (horizontal || vertical)
                            rows.push({tag: [scale, mirrored, horizontal, vertical].join("/"),
                                scale, mirrored, horizontal, vertical});
        return rows;
    }

    function test_resize(data) {
        let surface = createTemporaryObject(fixture, tests, {
            scale: data.scale, transformOrigin: Item.TopLeft, mirrored: data.mirrored
        });
        verify(surface);
        let x = (data.horizontal + 1) * 100;
        let y = (data.vertical + 1) * 50;
        drag(surface.box, x, y, data.horizontal * 40, data.vertical * 30);
        verify(surface.edits > 0);
        fuzzyCompare(surface.box.width, 200 + Math.abs(data.horizontal) * 40 / data.scale, 3);
        fuzzyCompare(surface.box.height, 100 + Math.abs(data.vertical) * 30 / data.scale, 3);
        if (data.horizontal) {
            let oppositeEdge = data.mirrored ? data.horizontal > 0 : data.horizontal < 0;
            fuzzyCompare(surface.box.x, oppositeEdge ? 350 - surface.box.width : 150, 1);
        }
        if (data.vertical)
            fuzzyCompare(surface.box.y, data.vertical < 0 ? 220 - surface.box.height : 120, 1);
        verifyBindings(surface);
    }

    function test_aspectRatioAndAnchors() {
        let surface = createTemporaryObject(fixture, tests);
        surface.editor.keepAspectRatio = true;
        surface.editor.anchorXCenter = true;
        surface.editor.anchorYBottom = true;
        drag(surface.box, 200, 0, 40, -30);
        verify(surface.edits > 0);
        fuzzyCompare(surface.box.width / surface.box.height, 2, 0.001);
        fuzzyCompare(surface.box.x + surface.box.width / 2, 250, 0.001);
        fuzzyCompare(surface.box.y + surface.box.height, 220, 0.001);
        verifyBindings(surface);
    }

    function test_verticalDrag() {
        let surface = createTemporaryObject(fixture, tests);
        surface.editor.dragAxis = Drag.YAxis;
        drag(surface.box, 100, 50, 80, 60);
        compare(surface.box.x, 150);
        verify(surface.box.y > 120);
        verifyBindings(surface);
    }
}
