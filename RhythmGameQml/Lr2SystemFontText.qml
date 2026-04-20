import QtQuick

Item {
    id: root

    property string text: ""
    property string family: ""
    property color textColor: "white"
    property int alignment: 0
    property int fontSize: 0
    property int fontThickness: 0
    property int fontType: 0
    property int blendMode: 1

    readonly property bool hasEdge: (fontType & 1) !== 0 && blendMode !== 2
    readonly property int basePixelSize: Math.max(1, Math.round(fontSize > 0 ? fontSize : height))
    readonly property int edgeWidth: hasEdge ? 1 : 0
    readonly property color edgeColor: Qt.rgba(0.03, 0.02, 0.05, 1.0)
    readonly property real maxLayerSize: 32760
    readonly property int resolvedFontWeight: fontThickness >= 6
        ? Font.Bold
        : (fontThickness >= 4 ? Font.DemiBold : Font.Normal)
    readonly property real sourceWidth: Math.max(0, fillText.implicitWidth)
    readonly property real scaleY: basePixelSize > 0 && height > 0 ? height / basePixelSize : 1.0
    readonly property real fitScaleX: sourceWidth > width && width > 0 ? width / sourceWidth : 1.0
    readonly property real scaleX: scaleY * fitScaleX
    readonly property real drawnWidth: sourceWidth * scaleX
    readonly property real alignedX: {
        if (alignment === 1) return -drawnWidth * 0.5;
        if (alignment === 2) return -drawnWidth;
        return 0;
    }
    readonly property var edgeOffsets: {
        if (!hasEdge || edgeWidth <= 0) {
            return [];
        }

        let offsets = [];
        let r = edgeWidth;
        let rr = r * r;
        for (let y = -r; y <= r; ++y) {
            for (let x = -r; x <= r; ++x) {
                if ((x !== 0 || y !== 0) && x * x + y * y <= rr) {
                    offsets.push({ x: x, y: y });
                }
            }
        }
        return offsets;
    }

    Item {
        id: textCanvas

        x: root.alignedX
        y: 0
        width: Math.max(1, Math.min(root.sourceWidth, root.maxLayerSize))
        height: Math.max(1, Math.min(fillText.implicitHeight, root.maxLayerSize))
        layer.enabled: true
        layer.smooth: false
        transform: Scale {
            origin.x: 0
            origin.y: 0
            xScale: root.scaleX
            yScale: root.scaleY
        }

        Repeater {
            model: root.edgeOffsets

            delegate: Text {
                x: modelData.x
                y: modelData.y
                text: root.text
                color: root.edgeColor
                font.family: root.family
                font.pixelSize: root.basePixelSize
                font.weight: root.resolvedFontWeight
                font.hintingPreference: Font.PreferFullHinting
                elide: Text.ElideNone
                renderType: Text.NativeRendering
            }
        }

        Text {
            id: fillText

            text: root.text
            color: root.textColor
            font.family: root.family
            font.pixelSize: root.basePixelSize
            font.weight: root.resolvedFontWeight
            font.hintingPreference: Font.PreferFullHinting
            elide: Text.ElideNone
            renderType: Text.NativeRendering
        }
    }
}
