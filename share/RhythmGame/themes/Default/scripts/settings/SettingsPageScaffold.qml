import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root

    property int maximumContentWidth: 1360
    property int minimumContentWidth: 0
    property int contentSpacing: 16
    readonly property real targetContentWidth: Math.min(maximumContentWidth, Math.max(availableWidth, minimumContentWidth))
    default property alias content: contentLayout.data

    contentWidth: Math.max(availableWidth, targetContentWidth)
    contentHeight: contentLayout.implicitHeight
    clip: true

    ColumnLayout {
        id: contentLayout

        x: Math.max(0, (root.availableWidth - width) / 2)
        width: Math.min(root.maximumContentWidth, root.targetContentWidth)
        spacing: root.contentSpacing
    }
}
