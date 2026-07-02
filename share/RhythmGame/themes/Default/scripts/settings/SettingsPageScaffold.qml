import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root

    property int maximumContentWidth: 1360
    property int contentSpacing: 16
    default property alias content: contentLayout.data

    contentWidth: Math.max(contentLayout.implicitWidth, availableWidth)
    contentHeight: contentLayout.implicitHeight
    clip: true

    ColumnLayout {
        id: contentLayout

        x: Math.max(0, (root.availableWidth - width) / 2)
        width: Math.min(root.maximumContentWidth, root.contentWidth)
        spacing: root.contentSpacing
    }
}
