import QtQml

QtObject {
    readonly property real scale: 2
    readonly property real bodyPixelSize: scaled(13)
    readonly property real supportingPixelSize: scaled(11)

    function scaled(basePixelSize: real): real {
        return basePixelSize * scale;
    }
}
