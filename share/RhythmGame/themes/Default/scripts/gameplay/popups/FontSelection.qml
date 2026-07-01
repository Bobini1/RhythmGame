import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import "../../common"
import "../../common/helpers.js" as Helpers

Row {
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

    height: Math.max(fontControls.implicitHeight, propertyLabel.height)
    spacing: 10

    ThemeFont {
        id: selectedFont
        fileName: fontSelection.currentValue
    }

    Text {
        id: propertyLabel

        anchors.verticalCenter: parent.verticalCenter
        color: "white"
        font.bold: true
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        text: fontSelection.label
        verticalAlignment: Text.AlignVCenter
        width: 160
    }

    Column {
        id: fontControls

        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
        width: 320

        Button {
            activeFocusOnTab: true
            text: fontSelection.displayNameForLoadedFont(fontSelection.currentValue, selectedFont.fontFamily)
            width: parent.width
            onClicked: fontDialog.open()
        }

        Rectangle {
            border.color: "#80ffffff"
            border.width: 1
            color: "#202020"
            height: 28
            width: parent.width

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                color: "white"
                elide: Text.ElideRight
                font.family: selectedFont.fontFamily
                font.italic: selectedFont.italic
                font.pixelSize: 20
                font.weight: selectedFont.fontWeight
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
        standardButtons: Dialog.Close
        title: fontSelection.label
        width: Math.min(700, Overlay.overlay ? Overlay.overlay.width - 80 : 700)

        contentItem: ColumnLayout {
            clip: true
            spacing: 8

            TextField {
                id: searchField

                Layout.fillWidth: true
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
