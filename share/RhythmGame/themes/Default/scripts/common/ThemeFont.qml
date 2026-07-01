import QtQuick

FontLoader {
    id: themeFont

    property string fileName: "file:NotoSansJP-VariableFont_wght.ttf"
    property string fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    property string fallbackFamily: ""

    readonly property string selectedFont: fileName.length > 0 ? fileName : fallbackFileName
    readonly property bool systemFont: selectedFont.indexOf("system:") === 0
    readonly property string systemFontFamily: systemFont ? selectedFont.slice(7) : ""
    readonly property bool fileFont: selectedFont.indexOf("file:") === 0
    readonly property string selectedFileName: fileFont ? selectedFont.slice(5) : ""
    readonly property string fallbackSelectedFileName: fallbackFileName.indexOf("file:") === 0 ? fallbackFileName.slice(5) : ""
    readonly property string styleFileName: systemFont ? fallbackSelectedFileName : selectedFileName
    readonly property string fontFamily: systemFont ? systemFontFamily : (status === FontLoader.Ready && name.length > 0 ? name : fallbackFamily)
    readonly property string normalizedFileName: styleFileName.toLowerCase()
    readonly property bool italic: normalizedFileName.indexOf("italic") !== -1
    readonly property int boldFontWeight: Math.max(fontWeight, Font.Bold)
    readonly property int fontWeight: {
        if (normalizedFileName.indexOf("thin") !== -1) {
            return Font.Thin;
        }
        if (normalizedFileName.indexOf("light") !== -1) {
            return Font.Light;
        }
        if (normalizedFileName.indexOf("black") !== -1) {
            return Font.Black;
        }
        if (normalizedFileName.indexOf("bold") !== -1) {
            return Font.Bold;
        }
        return Font.Normal;
    }

    source: selectedFileName.length > 0 ? Qt.resolvedUrl("fonts/" + selectedFileName) : ""
}
