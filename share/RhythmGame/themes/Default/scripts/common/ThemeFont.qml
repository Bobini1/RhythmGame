import QtQuick
import RhythmGameQml

QtObject {
    id: themeFont

    property string fileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    property string fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    property string fallbackFamily: ""

    readonly property string selectedFont: fileName.length > 0 ? fileName : fallbackFileName
    readonly property bool systemFont: selectedFont.indexOf("system:") === 0
    readonly property string systemFontFamily: systemFont ? selectedFont.slice(7) : ""
    readonly property bool fileFont: selectedFont.indexOf("file:") === 0
    readonly property string selectedFileName: fileFont ? selectedFont.slice(5) : ""
    readonly property string fallbackSelectedFileName: fallbackFileName.indexOf("file:") === 0 ? fallbackFileName.slice(5) : ""
    readonly property string loaderFileName: selectedFileName.length > 0
        ? selectedFileName
        : (fallbackSelectedFileName.length > 0
            ? fallbackSelectedFileName
            : "NotoSans-VariableFont_wdth,wght.ttf")
    readonly property string styleFileName: systemFont ? fallbackSelectedFileName : selectedFileName
    readonly property string fontFamily: systemFont
        ? systemFontFamily
        : (selectedFontLoader.status === FontLoader.Ready && selectedFontLoader.name.length > 0
            ? selectedFontLoader.name
            : fallbackFamily)
    readonly property string normalizedFileName: styleFileName.toLowerCase()
    readonly property bool italic: normalizedFileName.indexOf("italic") !== -1
    readonly property int boldFontWeight: Math.max(fontWeight, Font.Bold)
    readonly property var variableAxes: ({ "wght": fontWeight })
    readonly property var boldVariableAxes: ({ "wght": boldFontWeight })
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

    readonly property string uiFallbackFamily: {
        if (cjkFontLoader.status !== FontLoader.Ready) {
            return "";
        }
        switch (Rg.languages.selectedScript) {
        case "Hans":
            return "Noto Sans CJK SC";
        case "Hant":
            return Rg.languages.selectedTerritory === "HK" || Rg.languages.selectedTerritory === "MO"
                ? "Noto Sans CJK HK"
                : "Noto Sans CJK TC";
        case "Jpan":
            return "Noto Sans CJK JP";
        case "Kore":
            return "Noto Sans CJK KR";
        default:
            return "";
        }
    }
    readonly property string songMetadataFallbackFamily: cjkFontLoader.status === FontLoader.Ready
        ? "Noto Sans CJK JP"
        : ""

    property FontLoader selectedFontLoader: FontLoader {
        source: Qt.resolvedUrl("fonts/" + themeFont.loaderFileName)
    }
    property FontLoader cjkFontLoader: FontLoader {
        source: Qt.resolvedUrl("fallbackFonts/NotoSansCJK-VF.otf.ttc")

        onStatusChanged: themeFont.configureUiFallbackFont()
    }

    onUiFallbackFamilyChanged: configureUiFallbackFont()

    function configureUiFallbackFont() {
        if (cjkFontLoader.status === FontLoader.Ready) {
            FontResolver.setLocaleFallbackFont(Rg.languages.selectedScript, uiFallbackFamily);
        }
    }

    function requestedFont(properties) {
        const resolvedProperties = Object.assign({}, properties || {});
        resolvedProperties.family = themeFont.fontFamily;
        return Qt.font(resolvedProperties);
    }

    function fontWithFallback(font, fallbackFamily, fallbackFirst) {
        const families = [];
        if (fallbackFirst && fallbackFamily.length > 0) {
            families.push(fallbackFamily);
        }
        families.push(themeFont.fontFamily);
        if (fallbackFamily.length > 0) {
            if (!fallbackFirst) {
                families.push(fallbackFamily);
            }
        }
        return FontResolver.resolve(font, families);
    }

    function uiFont(properties) {
        return fontWithFallback(requestedFont(properties), uiFallbackFamily, false);
    }

    function songMetadataFont(properties, text) {
        const font = requestedFont(properties);
        const metadata = text || "";
        const fallbackFirst = FontResolver.containsCjkScript(metadata)
            && !FontResolver.supportsCjkCharacters(font, metadata);
        return fontWithFallback(font, songMetadataFallbackFamily, fallbackFirst);
    }
}
