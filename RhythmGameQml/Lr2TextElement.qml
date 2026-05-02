pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

Item {
    id: textElement

    required property var screenRoot
    required property var selectContext
    property var dsts: []
    property var srcData: null
    property var activeOptions: []
    property var chart: null
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property int timerFire: 0
    property real skinScale: 1
    property int valueRevision: 0

    readonly property var root: screenRoot
    readonly property bool ready: root !== undefined && root !== null
    readonly property bool selectReady: selectContext !== undefined && selectContext !== null

    readonly property var searchTextState: ready ? root.selectSearchTextState(srcData, dsts) : null
    readonly property bool isSearchText: ready && root.isSelectSearchText(srcData)
    readonly property string searchFontPath: srcData ? srcData.fontPath : ""
    readonly property int searchAlignment: srcData ? srcData.align : 0
    readonly property bool searchTextEditing: isSearchText
        && searchInputLoader.item
        && searchInputLoader.item.activeFocus
    readonly property string searchEditingText: searchInputLoader.item
        ? searchInputLoader.item.text
        : (selectReady ? selectContext.searchText : "")
    readonly property int searchCursorPosition: searchInputLoader.item
        ? searchInputLoader.item.cursorPosition
        : 0
    readonly property int searchSelectionStart: searchInputLoader.item
        ? Math.min(searchInputLoader.item.selectionStart, searchInputLoader.item.selectionEnd)
        : 0
    readonly property int searchSelectionEnd: searchInputLoader.item
        ? Math.max(searchInputLoader.item.selectionStart, searchInputLoader.item.selectionEnd)
        : 0
    readonly property bool searchHasSelection: searchTextEditing
        && searchSelectionStart !== searchSelectionEnd
    readonly property bool searchCursorVisible: searchTextEditing
        && !searchHasSelection
        && searchCursorOn
    property bool searchCursorOn: true
    property int searchDragAnchor: 0

    function restartSearchCursorBlink() {
        searchCursorOn = true;
        searchCursorBlinkTimer.restart();
    }

    function searchCursorPositionAt(parentX) {
        const text = searchEditingText || "";
        if (text.length <= 0 || searchTextScaleX <= 0) {
            return 0;
        }

        const sourceX = Math.max(0, (parentX - searchTextOriginX) / searchTextScaleX);
        let previousWidth = 0;
        for (let i = 1; i <= text.length; ++i) {
            const item = searchPrefixMeasureRepeater.itemAt(i);
            const width = item ? item.naturalWidth : previousWidth;
            if (sourceX < previousWidth + (width - previousWidth) / 2) {
                return i - 1;
            }
            previousWidth = width;
        }
        return text.length;
    }

    function moveSearchCursorTo(parentX, selecting) {
        if (!searchInputLoader.item) {
            return;
        }
        const position = searchCursorPositionAt(parentX);
        searchInputLoader.item.syncFromContext();
        searchInputLoader.item.forceActiveFocus();
        if (selecting) {
            searchInputLoader.item.select(searchDragAnchor, position);
        } else {
            searchDragAnchor = position;
            searchInputLoader.item.cursorPosition = position;
            searchInputLoader.item.deselect();
        }
        restartSearchCursorBlink();
    }

    Timer {
        id: searchCursorBlinkTimer
        interval: 500
        repeat: true
        running: textElement.ready
            && textElement.root.screenUpdatesActive
            && textElement.searchTextEditing
        onTriggered: textElement.searchCursorOn = !textElement.searchCursorOn
    }

    onSearchTextEditingChanged: restartSearchCursorBlink()
    onSearchCursorPositionChanged: restartSearchCursorBlink()
    onSearchSelectionStartChanged: restartSearchCursorBlink()
    onSearchSelectionEndChanged: restartSearchCursorBlink()

    Lr2BitmapFontText {
        id: searchFullMeasure
        x: -10000
        y: -10000
        width: searchInputLoader.item ? searchInputLoader.item.width : 1
        height: searchInputLoader.item ? searchInputLoader.item.height : 1
        opacity: 0
        fontPath: textElement.searchFontPath
        text: textElement.searchEditingText
    }

    Lr2BitmapFontText {
        id: searchCursorMeasure
        x: -10000
        y: -10000
        width: 1
        height: searchInputLoader.item ? searchInputLoader.item.height : 1
        opacity: 0
        fontPath: textElement.searchFontPath
        text: textElement.ready
            ? textElement.root.textPrefix(textElement.searchEditingText, textElement.searchCursorPosition)
            : ""
    }

    Lr2BitmapFontText {
        id: searchSelectionStartMeasure
        x: -10000
        y: -10000
        width: 1
        height: searchInputLoader.item ? searchInputLoader.item.height : 1
        opacity: 0
        fontPath: textElement.searchFontPath
        text: textElement.ready
            ? textElement.root.textPrefix(textElement.searchEditingText, textElement.searchSelectionStart)
            : ""
    }

    Lr2BitmapFontText {
        id: searchSelectionEndMeasure
        x: -10000
        y: -10000
        width: 1
        height: searchInputLoader.item ? searchInputLoader.item.height : 1
        opacity: 0
        fontPath: textElement.searchFontPath
        text: textElement.ready
            ? textElement.root.textPrefix(textElement.searchEditingText, textElement.searchSelectionEnd)
            : ""
    }

    readonly property real searchTextScaleY: searchFullMeasure.naturalHeight > 0 && searchInputLoader.item
        ? searchInputLoader.item.height / searchFullMeasure.naturalHeight
        : 1
    readonly property real searchTextFitScaleX: searchFullMeasure.naturalWidth > 0 && searchInputLoader.item
        && searchFullMeasure.naturalWidth > searchInputLoader.item.width
        ? searchInputLoader.item.width / searchFullMeasure.naturalWidth
        : 1
    readonly property real searchTextScaleX: searchTextScaleY * searchTextFitScaleX
    readonly property real searchDrawnWidth: searchFullMeasure.naturalWidth * searchTextScaleX
    readonly property real searchTextOriginX: searchInputLoader.item
        ? searchInputLoader.item.x + (
            searchAlignment === 1
                ? -searchDrawnWidth / 2
                : (searchAlignment === 2 ? -searchDrawnWidth : 0))
        : 0
    readonly property real searchTextOriginY: searchInputLoader.item ? searchInputLoader.item.y : 0

    Repeater {
        id: searchPrefixMeasureRepeater
        model: textElement.isSearchText ? textElement.searchEditingText.length + 1 : 0

        Lr2BitmapFontText {
            x: -10000
            y: -10000
            width: 1
            height: searchInputLoader.item ? searchInputLoader.item.height : 1
            opacity: 0
            fontPath: textElement.searchFontPath
            text: textElement.ready
                ? textElement.root.textPrefix(textElement.searchEditingText, index)
                : ""
        }
    }

    Rectangle {
        z: 1
        visible: textElement.searchHasSelection && searchInputLoader.item
        x: textElement.searchTextOriginX
            + searchSelectionStartMeasure.naturalWidth * textElement.searchTextScaleX
        y: textElement.searchTextOriginY
        width: Math.max(
            skinScale,
            (searchSelectionEndMeasure.naturalWidth - searchSelectionStartMeasure.naturalWidth)
                * textElement.searchTextScaleX)
        height: searchInputLoader.item ? searchInputLoader.item.height : 0
        color: Qt.rgba(0.45, 0.72, 1.0, 0.45)
    }

    Lr2TextRenderer {
        id: textRenderer
        z: 2
        anchors.fill: parent
        dsts: textElement.dsts
        srcData: textElement.srcData
        skinTime: textElement.skinTime
        skinClock: textElement.skinClock
        skinClockMode: textElement.skinClockMode
        activeOptions: textElement.activeOptions
        timerFire: textElement.timerFire
        chart: textElement.chart
        scaleOverride: textElement.skinScale
        resolvedText: textRenderer.hasCurrentState && textElement.ready
            ? textElement.root.resolveText(textElement.srcData ? textElement.srcData.st : -1,
                                           textElement.valueRevision)
            : ""
    }

    Loader {
        id: searchInputLoader
        z: 4
        active: textElement.isSearchText
        sourceComponent: TextInput {
            id: searchInput

            property bool syncing: false
            readonly property var textState: textElement.searchTextState

            function syncFromContext() {
                const context = textElement.selectReady ? textElement.selectContext : null;
                if (!context || text === context.searchText) {
                    return;
                }
                syncing = true;
                text = context.searchText;
                syncing = false;
            }

            x: textState ? Math.min(textState.x, textState.x + textState.w) * skinScale : 0
            y: textState ? Math.min(textState.y, textState.y + textState.h) * skinScale : 0
            width: textState ? Math.abs(textState.w) * skinScale : 0
            height: textState ? Math.abs(textState.h) * skinScale : 0
            visible: !!textState
            enabled: !!textState
            opacity: textState ? textState.a / 255.0 : 0
            clip: true
            activeFocusOnTab: false
            inputMethodHints: Qt.ImhNoPredictiveText

            color: "transparent"
            cursorVisible: false
            selectionColor: "transparent"
            selectedTextColor: "transparent"

            Component.onCompleted: {
                if (textElement.ready) {
                    textElement.root.selectSearchInputItem = searchInput;
                }
                syncFromContext();
            }

            Component.onDestruction: {
                if (textElement.ready && textElement.root.selectSearchInputItem === searchInput) {
                    textElement.root.selectSearchInputItem = null;
                }
            }

            onActiveFocusChanged: {
                if (activeFocus) {
                    syncFromContext();
                    cursorPosition = text.length;
                }
                textElement.restartSearchCursorBlink();
            }

            onCursorPositionChanged: textElement.restartSearchCursorBlink()
            onSelectionStartChanged: textElement.restartSearchCursorBlink()
            onSelectionEndChanged: textElement.restartSearchCursorBlink()

            onTextEdited: {
                const context = textElement.selectReady ? textElement.selectContext : null;
                if (!syncing
                        && context
                        && context.searchText !== text) {
                    context.searchText = text;
                    context.touch();
                }
            }

            Keys.onReturnPressed: (event) => {
                event.accepted = true;
                if (textElement.ready) {
                    textElement.root.submitSelectSearch();
                    textElement.root.clearSelectSearchFocus();
                }
            }

            Keys.onEnterPressed: (event) => {
                event.accepted = true;
                if (textElement.ready) {
                    textElement.root.submitSelectSearch();
                    textElement.root.clearSelectSearchFocus();
                }
            }

            Keys.onEscapePressed: (event) => {
                event.accepted = true;
                if (textElement.ready) {
                    textElement.root.resetSelectSearch();
                }
            }

            Connections {
                target: textElement.selectReady ? textElement.selectContext : null
                function onSearchTextChanged() {
                    const oldPosition = searchInput.cursorPosition;
                    searchInput.syncFromContext();
                    searchInput.cursorPosition = Math.min(oldPosition, searchInput.text.length);
                }
            }
        }
    }

    Rectangle {
        z: 5
        visible: textElement.searchCursorVisible && searchInputLoader.item
        x: textElement.searchTextOriginX
            + searchCursorMeasure.naturalWidth * textElement.searchTextScaleX
        y: textElement.searchTextOriginY
        width: Math.max(1, skinScale)
        height: searchInputLoader.item ? searchInputLoader.item.height : 0
        color: "white"
    }

    MouseArea {
        id: searchEditMouseArea
        z: 6
        enabled: !!textElement.searchTextState
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        x: textElement.searchTextState
            ? Math.min(textElement.searchTextState.x, textElement.searchTextState.x + textElement.searchTextState.w) * skinScale
            : 0
        y: textElement.searchTextState
            ? Math.min(textElement.searchTextState.y, textElement.searchTextState.y + textElement.searchTextState.h) * skinScale
            : 0
        width: textElement.searchTextState ? Math.abs(textElement.searchTextState.w) * skinScale : 0
        height: textElement.searchTextState ? Math.abs(textElement.searchTextState.h) * skinScale : 0
        onPressed: (mouse) => {
            mouse.accepted = true;
            textElement.moveSearchCursorTo(x + mouse.x, false);
        }
        onPositionChanged: (mouse) => {
            if (pressed) {
                mouse.accepted = true;
                textElement.moveSearchCursorTo(x + mouse.x, true);
            }
        }
    }
}
