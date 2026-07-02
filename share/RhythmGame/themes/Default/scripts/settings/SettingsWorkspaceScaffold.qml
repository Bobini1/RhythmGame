import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: root

    property int maximumContentWidth: 1360
    property int contentSpacing: 16
    readonly property real availableWidth: width
    readonly property real availableHeight: height
    default property alias content: contentLayout.data

    boundsBehavior: Flickable.StopAtBounds
    clip: true
    contentWidth: Math.max(contentLayout.implicitWidth, availableWidth)
    contentHeight: height
    flickableDirection: Flickable.HorizontalFlick
    ScrollBar.horizontal: ScrollBar {}

    ColumnLayout {
        id: contentLayout

        x: Math.max(0, (root.availableWidth - width) / 2)
        y: 0
        width: Math.min(root.maximumContentWidth, root.contentWidth)
        height: root.availableHeight
        spacing: root.contentSpacing
    }
}
