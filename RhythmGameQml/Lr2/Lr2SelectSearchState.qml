pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml

QtObject {
    id: searchState

    required property var screenRoot
    required property var selectContext

    readonly property int searchTextId: 30
    readonly property var host: screenRoot
    property var inputItem: null
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property bool focused: !!searchState.inputItem && searchState.inputItem.activeFocus

    function numberValue(value: var, fallback: var) : var {
        const numeric = Number(value);
        return isNaN(numeric) ? fallback : numeric;
    }

    function sourceTextId(src: var) : var {
        return src ? searchState.numberValue(src.st, -1) : -1;
    }

    function sourceEditMode(src: var) : var {
        return src ? searchState.numberValue(src.edit, 0) : 0;
    }

    function sourcePanel(src: var) : var {
        return src ? searchState.numberValue(src.panel, 0) : 0;
    }

    function isText(src: var) : var {
        return host.effectiveScreenKey === "select"
            && src
            && searchState.sourceTextId(src) === searchState.searchTextId
            && (host.lr2SkinUsesBeatorajaSemantics || searchState.sourceEditMode(src) !== 0)
            && host.panelMatches(searchState.sourcePanel(src));
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

    function clearFocus() : void {
        if (searchState.hasFocus()) {
            host.forceActiveFocus();
        }
    }

    function contains(x: var, y: var) : var {
        const item = searchState.inputItem;
        if (!item || !item.visible || !item.enabled) {
            return false;
        }

        const state = item.textState;
        if (state) {
            const left = Math.min(state.x, state.x + state.w);
            const right = Math.max(state.x, state.x + state.w);
            const top = Math.min(state.y, state.y + state.h);
            const bottom = Math.max(state.y, state.y + state.h);
            return x >= left && x <= right && y >= top && y <= bottom;
        }

        return x >= item.x
            && x <= item.x + item.width
            && y >= item.y
            && y <= item.y + item.height;
    }

    function focusAt(x: var, y: var) : var {
        if (!searchState.contains(x, y)) {
            return false;
        }
        const item = searchState.inputItem;
        if (item.focusAtSkinX) {
            item.focusAtSkinX(x);
        } else {
            item.forceActiveFocus();
        }
        return true;
    }

    function reset() : void {
        if (selectContext.searchText.length > 0) {
            selectContext.searchText = "";
            selectContext.touchSelection();
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
