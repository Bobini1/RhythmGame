import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import "../../common"
import ".."

RowLayout {
    id: fontSetting

    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_
    property string path
    property bool monospaceOnly: false
    property bool tabularDigitsOnly: false

    readonly property string defaultSystemFontFamily: Rg.fileQuery.getDefaultSystemFontFamily(monospaceOnly, tabularDigitsOnly)
    readonly property string systemDefault: defaultSystemFontFamily.length > 0 ? "system:" + defaultSystemFontFamily : ""
    readonly property string effectiveDefault: default_ || systemDefault
    readonly property string currentValue: destination[id_] || effectiveDefault
    readonly property var bundledFonts: path.length > 0 ? Rg.fileQuery.getSelectableFontFilesForDirectory(Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].path + "/" + path, monospaceOnly, tabularDigitsOnly) : []
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
        if (fontSetting.monospaceOnly) {
            return monospaceDetail;
        }
        if (fontSetting.tabularDigitsOnly) {
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

    ThemeFont {
        id: selectedFont

        fallbackFileName: fontSetting.effectiveDefault || "file:NotoSansJP-VariableFont_wght.ttf"
        fileName: fontSetting.currentValue
    }

    SettingsLabel {
        id: strLabel
    }

    Button {
        id: fontButton

        Layout.fillWidth: true
        Layout.minimumWidth: 200
        Layout.preferredWidth: 400
        text: fontSetting.displayNameForLoadedFont(fontSetting.currentValue, selectedFont.fontFamily)
        onClicked: fontDialog.open()
    }

    ResetButton {
        destination: fontSetting.destination
        id_: fontSetting.id_
        default_: fontSetting.effectiveDefault

        onClicked: {
            fontSetting.destination[fontSetting.id_] = fontSetting.effectiveDefault
        }
    }

    Dialog {
        id: fontDialog

        anchors.centerIn: Overlay.overlay
        height: Math.min(620, Overlay.overlay ? Overlay.overlay.height - 80 : 620)
        modal: true
        standardButtons: Dialog.Close
        title: fontSetting.name
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
                model: fontSetting.filteredFontChoices
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                delegate: ItemDelegate {
                    id: fontChoice

                    required property var modelData

                    width: fontList.width
                    highlighted: fontSetting.currentValue === modelData.value
                    onClicked: {
                        fontSetting.destination[fontSetting.id_] = modelData.value;
                        fontDialog.close();
                    }

                    ThemeFont {
                        id: previewFont

                        fallbackFileName: fontSetting.effectiveDefault || "file:NotoSansJP-VariableFont_wght.ttf"
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
                            text: fontSetting.displayNameForLoadedFont(fontChoice.modelData.value, previewFont.fontFamily)
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
