import QtQuick

Item {
    id: wrapper

    property alias color: label.color
    property alias font: label.font
    property bool scrolling: false
    property alias text: label.text

    clip: true

    Text {
        id: label

        property bool longText: textMetrics.width > wrapper.width
        property int spacing: 30

        anchors.verticalCenter: parent.verticalCenter
        color: selectItemLoader.isCurrentItem ? "yellow" : "black"
        font.pixelSize: 20
        // align right if there is enough space
        horizontalAlignment: longText ? Text.AlignLeft : Text.AlignRight
        leftPadding: 5
        width: parent.width

        Component {
            id: secondText

            Text {
                color: label.color
                font.pixelSize: 20
                horizontalAlignment: Text.AlignLeft
                leftPadding: 5
                text: label.text
                x: textMetrics.width + label.spacing
            }
        }
        Loader {
            id: loader

            active: label.longText
            sourceComponent: secondText
        }
        PropertyAnimation {
            id: longTextAnimation

            duration: 3000
            from: 0
            loops: Animation.Infinite
            property: "x"
            running: label.longText && selectItemLoader.isCurrentItem
            target: label
            to: -textMetrics.width - label.spacing

            onRunningChanged: {
                if (!running) {
                    label.x = 0;
                }
            }
        }
        TextMetrics {
            id: textMetrics

            font: label.font
            text: label.text
        }
    }
}
