import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

Popup {
    id: popup

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    implicitHeight: headerHeight + scrollHeight
    implicitWidth: 500
    padding: 0

    property string panelTitle: ""
    property string panelSubtitle: ""
    property alias model: column.model
    readonly property color editorControlColor: popupColors.control
    readonly property real visualMargin: 18
    readonly property real nearGap: 14
    readonly property real contentBottomPadding: 28
    readonly property real headerHeight: (headerLoader.active ? headerLoader.implicitHeight + 1 : 0)
    readonly property real maxPopupHeight: Math.max(240, contentContainer.height / scaledRoot.scale - visualMargin * 2)
    readonly property real scrollHeight: Math.min(Math.max(90, column.contentHeight + contentBottomPadding), Math.max(120, maxPopupHeight - headerHeight))

    PopupEditorColors {
        id: popupColors
    }

    palette.window: popupColors.panel
    palette.windowText: popupColors.text
    palette.base: popupColors.control
    palette.alternateBase: popupColors.preview
    palette.text: popupColors.text
    palette.button: popupColors.control
    palette.buttonText: popupColors.text
    palette.brightText: popupColors.text
    palette.mid: popupColors.controlBorder
    palette.dark: popupColors.controlDisabled
    palette.shadow: "#000000"
    palette.highlight: popupColors.accent
    palette.highlightedText: popupColors.highlightedText
    palette.placeholderText: popupColors.mutedText

    function closeTransientPopups() {
        for (let child of column.contentItem.children) {
            if (child.closeTransientPopups) {
                child.closeTransientPopups();
            }
        }
    }

    function setPosition(globalPos) {
        let scaledWidth = width * scale;
        let scaledHeight = height * scale;
        let scaledMargin = visualMargin * scale;
        let scaledGap = nearGap * scale;
        let roomRight = contentContainer.width - globalPos.x - scaledMargin;
        let roomLeft = globalPos.x - scaledMargin;
        let preferredX = roomRight >= scaledWidth + scaledGap || roomRight >= roomLeft
                ? globalPos.x + scaledGap
                : globalPos.x - scaledWidth - scaledGap;

        x = Math.max(scaledMargin, Math.min(preferredX, contentContainer.width - scaledWidth - scaledMargin));
        y = Math.max(scaledMargin, Math.min(globalPos.y - 40 * scale, contentContainer.height - scaledHeight - scaledMargin));
    }

    onAboutToHide: closeTransientPopups()

    background: Rectangle {
        border.color: popupColors.panelBorder
        border.width: 1
        color: popupColors.panel
        radius: 8
    }

    transformOrigin: Item.TopLeft
    scale: scaledRoot.scale
    contentItem: ColumnLayout {
        spacing: 0

        Loader {
            id: headerLoader

            Layout.fillWidth: true
            active: popup.panelTitle.length > 0 || popup.panelSubtitle.length > 0
            sourceComponent: Component {
                Column {
                    padding: 14
                    spacing: 3

                    Text {
                        color: popupColors.text
                        elide: Text.ElideRight
                        font.bold: true
                        font.pixelSize: 18
                        text: popup.panelTitle
                        textFormat: Text.PlainText
                        width: parent.width - parent.padding * 2
                    }

                    Text {
                        color: popupColors.mutedText
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        text: popup.panelSubtitle
                        textFormat: Text.PlainText
                        visible: text.length > 0
                        width: parent.width - parent.padding * 2
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: headerLoader.active ? 1 : 0
            Layout.fillWidth: true
            color: popupColors.divider
        }

        ScrollView {
            id: scrollView

            Layout.fillWidth: true
            Layout.preferredHeight: popup.scrollHeight
            clip: true
            padding: 8

            ListView {
                id: column

                boundsBehavior: Flickable.StopAtBounds
                clip: true
                implicitHeight: contentHeight
                width: scrollView.availableWidth

                ScrollBar.vertical: ScrollBar {
                    policy: column.contentHeight > column.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }
            }
        }
    }
}
