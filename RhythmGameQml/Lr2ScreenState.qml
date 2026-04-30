pragma ValueTypeBehavior: Addressable

import QtQuick

QtObject {
    id: root

    property string explicitKey: ""
    property string csvPath: ""
    property bool hostEnabled: false
    property bool hostVisible: false
    property bool stackActive: false
    property int globalSkinTime: 0
    property int startInput: 0
    property bool selectSearchFocused: false
    property int readmeMode: 0

    readonly property string effectiveKey: explicitKey || inferKey(csvPath)
    readonly property bool updatesActive: hostEnabled && hostVisible && stackActive
    readonly property bool acceptsInput: effectiveKey !== "select" || globalSkinTime >= startInput
    readonly property bool selectPointerInputReady: updatesActive && effectiveKey === "select" && acceptsInput
    readonly property bool selectInputReady: selectPointerInputReady && !selectSearchFocused
    readonly property bool selectScrollReady: selectInputReady && readmeMode === 0
    readonly property bool selectPointerScrollReady: selectPointerInputReady && readmeMode === 0
    readonly property bool selectNavigationReady: selectInputReady && readmeMode === 0

    function inferKey(path) {
        let normalized = (path || "").replace(/\\/g, "/").toLowerCase();
        if (normalized.indexOf("/select/") !== -1
                || normalized.endsWith("select.lr2skin")
                || normalized.endsWith("select.csv")) {
            return "select";
        }
        if (normalized.indexOf("/decide/") !== -1
                || normalized.endsWith("decide.lr2skin")
                || normalized.endsWith("decide.csv")) {
            return "decide";
        }
        if (normalized.indexOf("/courseresult/") !== -1
                || normalized.endsWith("courseresult.lr2skin")
                || normalized.endsWith("courseresult.csv")) {
            return "courseResult";
        }
        if (normalized.indexOf("/result/") !== -1
                || normalized.endsWith("result.lr2skin")
                || normalized.endsWith("result.csv")) {
            return "result";
        }
        return "";
    }

    function isGameplayScreen() {
        switch (root.effectiveKey) {
        case "k5":
        case "k7":
        case "k10":
        case "k14":
        case "k5battle":
        case "k7battle":
        case "k10battle":
        case "k14battle":
            return true;
        default:
            return false;
        }
    }

    function isResultScreen() {
        return root.effectiveKey === "result" || root.effectiveKey === "courseResult";
    }
}
