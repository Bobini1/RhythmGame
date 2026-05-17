pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: searchState

    required property var screenRoot
    required property var selectContext

    readonly property var host: screenRoot
    property var inputItem: null
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property bool focused: !!searchState.inputItem && searchState.inputItem.activeFocus

    function isText(src: var) : var {
        return host.effectiveScreenKey === "select"
            && host.acceptsInput
            && src
            && src.st === 30
            && src.edit !== 0
            && host.panelMatches(src.panel || 0);
    }

    function textState(src: var, dsts: var) : var {
        if (!searchState.isText(src)) {
            return null;
        }
        let timer = timelineResolver.firstTimerFor(dsts);
        return timelineResolver.stateFromTimerFire(
            dsts,
            host.renderSkinTime,
            host.skinTimerFireTime(timer),
            host.activeOptionsForElementDsts(dsts));
    }

    function textPrefix(text: var, position: var) : var {
        const source = text || "";
        const clamped = Math.max(0, Math.min(position || 0, source.length));
        return source.substring(0, clamped);
    }

    function hasFocus() : var {
        return searchState.focused;
    }

    function canFocus() : var {
        return host.effectiveScreenKey === "select"
            && host.acceptsInput
            && host.selectPanel <= 0
            && !!searchState.inputItem;
    }

    function focusInput() : var {
        if (!searchState.canFocus()) {
            return;
        }
        searchState.inputItem.syncFromContext();
        searchState.inputItem.cursorPosition = searchState.inputItem.text.length;
        searchState.inputItem.forceActiveFocus();
    }

    function clearFocus() : void {
        if (searchState.hasFocus()) {
            host.forceActiveFocus();
        }
    }

    function reset() : void {
        if (selectContext.searchText.length > 0) {
            selectContext.searchText = "";
            selectContext.touch();
        }
        if (searchState.inputItem && searchState.inputItem.text.length > 0) {
            searchState.inputItem.text = "";
        }
        searchState.clearFocus();
    }

    function submit() : void {
        let query = selectContext.searchText.trim();
        if (query.length > 0) {
            selectContext.search(query);
        }
    }
}
