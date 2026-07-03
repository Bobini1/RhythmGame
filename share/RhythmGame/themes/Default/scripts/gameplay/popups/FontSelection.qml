import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import "../../common"
import "../../common/helpers.js" as Helpers

Item {
    id: fontSelection

    required property string propertyId
    required property var src
    property string label: Helpers.capitalizeFirstLetter(propertyId)
    property bool monospaceOnly: false
    property bool tabularDigitsOnly: false
    readonly property string currentValue: src[propertyId] || "file:NotoSansJP-VariableFont_wght.ttf"
    readonly property var bundledFonts: Rg.fileQuery.getSelectableFontFilesForDirectory(root.rootUrl + "../common/fonts/", monospaceOnly, tabularDigitsOnly)
    readonly property var systemFonts: Rg.fileQuery.getSystemFontFamilies(monospaceOnly, tabularDigitsOnly)
    readonly property var fontChoices: {
        let choices = [];
        for (let fontFile of bundledFonts) {
            choices.push({
                label: displayNameForValue(fontFile),
                detail: fontDetail(qsTr("Theme font"), qsTr("Theme monospace font"), qsTr("Theme tabular-digit font")),
                value: "file:" + fontFile
            });
        }
        for (let family of systemFonts) {
            choices.push({
                label: family,
                detail: fontDetail(qsTr("System font"), qsTr("System monospace font"), qsTr("System tabular-digit font")),
                value: "system:" + family
            });
        }
        return choices;
    }
    readonly property var filteredFontChoices: {
        let needle = searchField.text.trim().toLocaleLowerCase();
        if (needle.length === 0) {
            return fontChoices;
        }
        return fontChoices.filter(choice => choice.label.toLocaleLowerCase().indexOf(needle) !== -1);
    }

    function fontDetail(regularDetail, monospaceDetail, tabularDetail) {
        if (fontSelection.monospaceOnly) {
            return monospaceDetail;
        }
        if (fontSelection.tabularDigitsOnly) {
            return tabularDetail;
        }
        return regularDetail;
    }

    function displayNameForValue(value) {
        if (!value) {
            return qsTr("None");
        }
        if (value.indexOf("system:") === 0) {
            return value.slice(7);
        }
        if (value.indexOf("file:") === 0) {
            value = value.slice(5);
        }
        return value.replace(/\.[^/.]+$/, "").replace(/-/g, " ");
    }

    function displayNameForLoadedFont(value, family) {
        if (value.indexOf("file:") === 0 && family.length > 0) {
            return family;
        }
        return displayNameForValue(value);
    }

    function closeTransientPopups() {
        fontDialog.close();
    }

    Component.onDestruction: closeTransientPopups()

    height: fontControls.implicitHeight + 16
    width: ListView.view ? ListView.view.width : 414

    PopupEditorColors {
        id: popupColors
    }

    ThemeFont {
        id: selectedFont
        fileName: fontSelection.currentValue
    }

    Column {
        id: fontControls

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 8

        Text {
            id: propertyLabel

            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            text: fontSelection.label
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            width: parent.width
        }

        Button {
            activeFocusOnTab: true
            palette.button: popupColors.control
            palette.buttonText: popupColors.text
            palette.highlight: popupColors.accent
            palette.highlightedText: popupColors.highlightedText
            palette.text: popupColors.text
            palette.windowText: popupColors.text
            text: fontSelection.displayNameForLoadedFont(fontSelection.currentValue, selectedFont.fontFamily)
            width: parent.width
            onClicked: fontDialog.open()
        }

        Rectangle {
            border.color: popupColors.controlBorder
            border.width: 1
            color: popupColors.preview
            height: 32
            radius: 4
            width: parent.width

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                color: popupColors.text
                elide: Text.ElideRight
                font.family: selectedFont.fontFamily
                font.italic: selectedFont.italic
                font.pixelSize: 20
                font.weight: selectedFont.fontWeight
                font.variableAxes: selectedFont.variableAxes
                fontSizeMode: Text.HorizontalFit
                horizontalAlignment: Text.AlignHCenter
                minimumPixelSize: 8
                text: "RhythmGame 12345 Aa"
                textFormat: Text.PlainText
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Dialog {
        id: fontDialog

        anchors.centerIn: Overlay.overlay
        height: Math.min(620, Overlay.overlay ? Overlay.overlay.height - 80 : 620)
        modal: true
        palette.base: popupColors.preview
        palette.button: popupColors.control
        palette.buttonText: popupColors.text
        palette.highlight: popupColors.accent
        palette.highlightedText: popupColors.highlightedText
        palette.placeholderText: popupColors.mutedText
        palette.text: popupColors.text
        palette.window: popupColors.panel
        palette.windowText: popupColors.text
        standardButtons: Dialog.Close
        title: fontSelection.label
        width: Math.min(700, Overlay.overlay ? Overlay.overlay.width - 80 : 700)

        contentItem: ColumnLayout {
            clip: true
            spacing: 8

            TextField {
                id: searchField

                Layout.fillWidth: true
                palette.base: popupColors.control
                palette.button: popupColors.control
                palette.buttonText: popupColors.text
                palette.highlight: popupColors.accent
                palette.highlightedText: popupColors.highlightedText
                palette.placeholderText: popupColors.mutedText
                palette.text: popupColors.text
                palette.windowText: popupColors.text
                placeholderText: qsTr("Search fonts")
                selectByMouse: true
            }

            ListView {
                id: fontList

                Layout.fillHeight: true
                Layout.fillWidth: true
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: fontSelection.filteredFontChoices
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                delegate: ItemDelegate {
                    id: fontChoice

                    required property var modelData

                    highlighted: fontSelection.currentValue === modelData.value
                    palette.base: popupColors.preview
                    palette.button: popupColors.control
                    palette.highlight: popupColors.accent
                    palette.highlightedText: popupColors.highlightedText
                    palette.placeholderText: popupColors.mutedText
                    palette.text: popupColors.text
                    palette.windowText: popupColors.text
                    width: fontList.width
                    onClicked: {
                        fontSelection.src[fontSelection.propertyId] = modelData.value;
                        fontDialog.close();
                    }

                    ThemeFont {
                        id: previewFont

                        fileName: fontChoice.modelData.value
                    }

                    contentItem: Column {
                        spacing: 2

                        Text {
                            color: fontChoice.highlighted ? fontChoice.palette.highlightedText : fontChoice.palette.text
                            elide: Text.ElideRight
                            font.family: previewFont.fontFamily
                            font.italic: previewFont.italic
                            font.pixelSize: 18
                            font.weight: previewFont.fontWeight
                            font.variableAxes: previewFont.variableAxes
                            text: fontSelection.displayNameForLoadedFont(fontChoice.modelData.value, previewFont.fontFamily)
                            textFormat: Text.PlainText
                        }

                        Text {
                            color: fontChoice.highlighted ? fontChoice.palette.highlightedText : fontChoice.palette.placeholderText
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            text: fontChoice.modelData.detail
                            textFormat: Text.PlainText
                        }
                    }
                }
            }
        }

        onOpened: {
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }
}
