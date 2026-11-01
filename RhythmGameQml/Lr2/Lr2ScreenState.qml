pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: root

    required property string screenKey
    property bool hostEnabled: false
    property bool hostVisible: false
    property bool stackActive: false
    property int globalSkinTime: 0
    property int startInput: 0
    property bool selectSearchFocused: false
    property int readmeMode: 0

    readonly property bool gameplayScreen: isGameplayKey(root.screenKey)
    readonly property bool resultScreen: root.screenKey === "result" || root.screenKey === "courseResult"
    readonly property bool updatesActive: hostEnabled && hostVisible && stackActive
    readonly property bool gatesInputByStartInput: root.screenKey === "select"
        || root.screenKey === "decide"
        || resultScreen
    readonly property bool acceptsInput: !gatesInputByStartInput || globalSkinTime >= startInput
    readonly property bool selectPointerInputReady: updatesActive && root.screenKey === "select" && acceptsInput
    readonly property bool selectInputReady: selectPointerInputReady && !selectSearchFocused
    readonly property bool selectScrollReady: selectInputReady && readmeMode === 0
    readonly property bool selectPointerScrollReady: selectPointerInputReady && readmeMode === 0
    readonly property bool selectNavigationReady: selectInputReady && readmeMode === 0

    function isGameplayKey(key: var) : var {
        switch (key) {
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

}
