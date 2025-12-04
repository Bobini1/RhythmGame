import QtQuick

Item {
    id: keyImage

    implicitWidth: 64
    implicitHeight: 32
    property var highlightedKeys: []

    Repeater {
        model: 7
        delegate: Image {
            readonly property bool isHighlighted: keyImage.highlightedKeys.indexOf(index + 1) !== -1
            readonly property bool isOdd: index % 2
            x: (keyImage.width / 8) * index
            y: (isOdd ? 0 : keyImage.height - height)
            width: keyImage.width * 16 / 64
            height: keyImage.height * 20 / 32
            source: root.iniImagesUrl + "option.png/" + (isHighlighted ? "key_highlighted" : "key_normal")
            z: isHighlighted ? 1 : 0
        }
    }
}