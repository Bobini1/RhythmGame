pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: sceneRoot

    required property var screenRoot
    required property var skinModel
    required property var playContext
    required property var selectContext

    readonly property var root: screenRoot
    readonly property real skinW: root.skinW
    readonly property real skinH: root.skinH
    readonly property real skinScale: root.skinScale

    function hoverPointInSkinCoordinates() {
        return selectHoverHandler.hovered ? selectHoverHandler.point.position : null;
    }

    Item {
        id: skinViewport
        anchors.fill: parent

        // Children stay in skin coordinates; this transform performs the same
        // global non-uniform stretch LR2 applies at presentation time.
        Item {
            id: skinContainer
            width: skinW
            height: skinH
            transform: Scale {
                origin.x: 0
                origin.y: 0
                xScale: root.skinVisualScaleX
                yScale: root.skinVisualScaleY
            }

            MouseArea {
                id: selectBlankMouseArea
                anchors.fill: parent
                enabled: root.selectPointerInputReady()
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                z: -100000
                onClicked: {
                    root.clearSelectSearchFocus();
                }
            }

            HoverHandler {
                id: selectHoverHandler
                enabled: root.selectHoverTracking && root.selectHoverElementCount > 0
                target: null
                onHoveredChanged: root.scheduleSelectHoverRefresh()
            }

            Timer {
                id: selectHoverSampler
                interval: 8
                repeat: true
                running: root.selectHoverTracking
                    && root.selectHoverElementCount > 0
                    && selectHoverHandler.hovered
                onTriggered: root.refreshSelectHoverCache()
            }

            Repeater {
                id: skinRepeater
                model: skinModel

                delegate: Loader {
                    id: elemLoader
                    x: 0; y: 0
                    width: skinW * skinScale
                    height: skinH * skinScale
                    z: root.elementZ(model.type, index, model.src, model.dsts)
                    readonly property bool usesActiveOptions: root.dstsUseActiveOptions(model.dsts)
                    readonly property bool usesTimers: root.elementUsesTimers(model.src, model.dsts)
                    readonly property bool usesSkinTime: root.elementUsesSkinTime(model.src, model.dsts)
                    readonly property int dstTimer: model.dsts && model.dsts.length > 0
                        ? (model.dsts[0].timer || 0)
                        : 0
                    readonly property bool usesSelectHeldButtonTimer: root.isSelectHeldButtonTimer(dstTimer)
                    readonly property bool usesLiveDstClock: root.elementUsesLiveDstClock(model.dsts)
                    readonly property bool usesLiveSourceClock: root.elementUsesLiveSourceClock(model.src)
                    readonly property bool usesLiveSelectClock: usesLiveDstClock || usesLiveSourceClock
                    readonly property bool usesSpriteStateOverride: root.elementUsesSpriteStateOverride(model.src)
                    readonly property bool usesSpriteForceHidden: root.elementUsesSpriteForceHidden(model.src)
                    readonly property bool usesButtonFrameOverride: root.elementUsesButtonFrameOverride(model.src)
                    readonly property var elementActiveOptions: usesActiveOptions
                        ? root.runtimeActiveOptions
                        : root.emptyActiveOptions
                    readonly property var elementTimers: usesTimers
                        ? root.timers
                        : root.zeroTimers
                    readonly property bool usesElementSkinTime: usesSkinTime
                        && model.type !== 0
                        && model.type !== 3
                        && model.type !== 4
                        && model.type !== 5
                        && model.type !== 8
                        && model.type !== 9
                    readonly property int elementSkinTime: usesElementSkinTime
                        ? (usesLiveSelectClock ? root.selectSourceSkinTime : root.renderSkinTime)
                        : 0

                    Component.onCompleted: {
                        root.registerSelectHoverElement(index, model.src, model.dsts, usesSpriteForceHidden);
                    }
                    Component.onDestruction: {
                        root.unregisterSelectHoverElement(index);
                    }

                    sourceComponent: {
                        if (model.type === 0) {
                            return model.src && model.src.mouseCursor ? undefined : imageComponent;
                        } else if (model.type === 1) {
                            return numberComponent;
                        } else if (model.type === 2) {
                            return model.src && model.src.readme ? readmeTextComponent : textComponent;
                        } else if (model.type === 3) {
                            return barImageComponent;
                        } else if (model.type === 4) {
                            return barTextComponent;
                        } else if (model.type === 5) {
                            return barNumberComponent;
                        } else if (model.type === 6) {
                            return barGraphComponent;
                        } else if (model.type === 7) {
                            return bgaComponent;
                        } else if (model.type === 8) {
                            return playNotesComponent;
                        } else if (model.type === 9) {
                            return grooveGaugeComponent;
                        } else if (model.type === 10) {
                            return resultChartComponent;
                        } else if (model.type === 11) {
                            return noteChartComponent;
                        } else if (model.type === 12) {
                            return bpmChartComponent;
                        }
                        return undefined;
                    }

                    Component {
                        id: imageComponent
                        Item {
                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property int selectHeldSkinClock: elemLoader.usesSelectHeldButtonTimer
                                ? (root.hasSelectHeldButtonTimers
                                    ? root.selectHeldButtonSkinTime
                                    : root.currentSelectHeldButtonSkinTime())
                                : 0
                            readonly property int spriteSkinClock: elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (elemLoader.usesLiveDstClock
                                        ? root.selectSourceSkinTime
                                        : root.renderSkinTime))
                                : 0
                            readonly property int spriteSourceSkinClock: elemLoader.usesSkinTime
                                ? (elemLoader.usesSelectHeldButtonTimer
                                    ? selectHeldSkinClock
                                    : (elemLoader.usesLiveSourceClock
                                        ? root.selectSourceSkinTime
                                        : spriteSkinClock))
                                : 0

                            Component.onCompleted: root.observeSelectSortButton(model.src)

                            Lr2SpriteRenderer {
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: root.imageSetSourceFor(model.src)
                                skinTime: parent.spriteSkinClock
                                sourceSkinTime: parent.spriteSourceSkinClock
                                activeOptions: elemLoader.elementActiveOptions
                                timers: elemLoader.elementTimers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                mediaActive: root.enabled
                                transColor: skinModel.transColor
                                colorKeyEnabled: skinModel.hasTransColor
                                frameOverride: elemLoader.usesButtonFrameOverride ? root.buttonFrame(model.src) : -1
                                stateOverride: elemLoader.usesSpriteStateOverride
                                    ? root.spriteStateOverride(model.src, model.dsts, parent.spriteSkinClock)
                                    : null
                                forceHidden: elemLoader.usesSpriteForceHidden
                                    ? root.spriteForceHidden(model.src, index)
                                    : false
                                scratchAngle1: playContext.scratchAngle1
                                scratchAngle2: playContext.scratchAngle2
                            }

                            readonly property int effectiveButtonId: root.elementButtonId(model.src)
                            readonly property var buttonState: effectiveButtonId > 0
                                ? Lr2Timeline.getCurrentState(
                                    model.dsts,
                                    root.renderSkinTime,
                                    elemLoader.elementTimers,
                                    elemLoader.elementActiveOptions)
                                : null

                            MouseArea {
                                id: lr2ButtonMouseArea
                                enabled: root.effectiveScreenKey === "select"
                                    && root.acceptsInput
                                    && model.src
                                    && parent.effectiveButtonId > 0
                                    && root.elementButtonClickEnabled(model.src)
                                    && root.elementButtonPanelMatches(model.src)
                                    && !!parent.buttonState
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                preventStealing: true
                                x: parent.buttonState ? Math.min(parent.buttonState.x, parent.buttonState.x + parent.buttonState.w) * skinScale : 0
                                y: parent.buttonState ? Math.min(parent.buttonState.y, parent.buttonState.y + parent.buttonState.h) * skinScale : 0
                                width: parent.buttonState ? Math.abs(parent.buttonState.w) * skinScale : 0
                                height: parent.buttonState ? Math.abs(parent.buttonState.h) * skinScale : 0
                                onPressed: (mouse) => {
                                    mouse.accepted = true;
                                }
                                onClicked: (mouse) => {
                                    let delta = root.buttonMouseDelta(model.src, mouse.x, width);
                                    root.handleLr2Button(
                                        parent.effectiveButtonId,
                                        delta,
                                        root.elementButtonPanel(model.src),
                                        undefined,
                                        root.elementSourceFrameCount(model.src));
                                    mouse.accepted = true;
                                }
                            }
                        }
                    }

                    Component {
                        id: bgaComponent
                        Lr2BgaRenderer {
                            dsts: model.dsts
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            chart: root.chart
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.isGameplayScreen() && root.lr2BgaEnabled()
                            poorVisible: root.gameplayPoorBgaVisible()
                        }
                    }

                    Component {
                        id: playNotesComponent
                        Lr2PlayNoteField {
                            anchors.fill: parent
                            screenRoot: root
                            skinModel: root.skinModelRef
                            skinScale: skinScale
                            renderSkinTime: root.renderSkinTime
                            runtimeActiveOptions: root.noteFieldUsesActiveOptions()
                                ? root.runtimeActiveOptions
                                : root.emptyActiveOptions
                            timers: root.noteFieldUsesTimers()
                                ? root.timers
                                : root.zeroTimers
                            transColor: root.skinModelRef ? root.skinModelRef.transColor : "black"
                            enabled: root.enabled && root.isGameplayScreen()
                        }
                    }

                    Component {
                        id: grooveGaugeComponent
                        Lr2GrooveGaugeRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.renderSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            screenRoot: root
                            scaleOverride: skinScale
                            mediaActive: root.enabled && root.isGameplayScreen()
                            transColor: root.skinModelRef ? root.skinModelRef.transColor : "black"
                        }
                    }

                    Component {
                        id: resultChartComponent
                        Lr2ResultChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            screenRoot: root
                        }
                    }

                    Component {
                        id: noteChartComponent
                        Lr2NoteChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: bpmChartComponent
                        Lr2BpmChartRenderer {
                            anchors.fill: parent
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            chart: root.visualSelectChart
                        }
                    }

                    Component {
                        id: numberComponent
                        Lr2NumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            scaleOverride: skinScale
                            value: root.numberValue(model.src)
                            forceHidden: root.numberForceHidden(model.src)
                            animationRevision: root.numberAnimationRevision(model.src)
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                        }
                    }

                    Component {
                        id: textComponent
                        Item {
                            id: textDelegateRoot

                            width: skinW * skinScale
                            height: skinH * skinScale
                            readonly property string resolvedText: root.resolveText(model.src ? model.src.st : -1)
                            readonly property var searchTextState: root.selectSearchTextState(model.src, model.dsts)
                            readonly property bool isSearchText: root.isSelectSearchText(model.src)
                            readonly property string searchFontPath: model.src ? model.src.fontPath : ""
                            readonly property int searchAlignment: model.src ? model.src.align : 0
                            readonly property bool searchTextEditing: isSearchText
                                && searchInputLoader.item
                                && searchInputLoader.item.activeFocus
                            readonly property string searchEditingText: searchInputLoader.item
                                ? searchInputLoader.item.text
                                : selectContext.searchText
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

                            function restartSearchCursorBlink() {
                                searchCursorOn = true;
                                searchCursorBlinkTimer.restart();
                            }

                            Timer {
                                id: searchCursorBlinkTimer
                                interval: 500
                                repeat: true
                                running: parent.searchTextEditing
                                onTriggered: parent.searchCursorOn = !parent.searchCursorOn
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
                                fontPath: textDelegateRoot.searchFontPath
                                text: searchInputLoader.parent.searchEditingText
                            }

                            Lr2BitmapFontText {
                                id: searchCursorMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchCursorPosition)
                            }

                            Lr2BitmapFontText {
                                id: searchSelectionStartMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchSelectionStart)
                            }

                            Lr2BitmapFontText {
                                id: searchSelectionEndMeasure
                                x: -10000
                                y: -10000
                                width: 1
                                height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                opacity: 0
                                fontPath: textDelegateRoot.searchFontPath
                                text: root.textPrefix(
                                    searchInputLoader.parent.searchEditingText,
                                    searchInputLoader.parent.searchSelectionEnd)
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
                            readonly property real searchTextOriginY: searchInputLoader.item
                                ? searchInputLoader.item.y
                                : 0
                            property int searchDragAnchor: 0

                            function searchCursorPositionAt(parentX) {
                                const text = searchEditingText || "";
                                if (text.length <= 0 || searchTextScaleX <= 0) {
                                    return 0;
                                }

                                const sourceX = Math.max(
                                    0,
                                    (parentX - searchTextOriginX) / searchTextScaleX);
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

                            Repeater {
                                id: searchPrefixMeasureRepeater
                                model: searchInputLoader.parent.isSearchText
                                    ? searchInputLoader.parent.searchEditingText.length + 1
                                    : 0

                                Lr2BitmapFontText {
                                    x: -10000
                                    y: -10000
                                    width: 1
                                    height: searchInputLoader.item ? searchInputLoader.item.height : 1
                                    opacity: 0
                                    fontPath: textDelegateRoot.searchFontPath
                                    text: root.textPrefix(
                                        textDelegateRoot.searchEditingText,
                                        index)
                                }
                            }

                            Rectangle {
                                z: 1
                                visible: parent.searchHasSelection && searchInputLoader.item
                                x: parent.searchTextOriginX
                                    + searchSelectionStartMeasure.naturalWidth * parent.searchTextScaleX
                                y: parent.searchTextOriginY
                                width: Math.max(
                                    skinScale,
                                    (searchSelectionEndMeasure.naturalWidth
                                        - searchSelectionStartMeasure.naturalWidth)
                                        * parent.searchTextScaleX)
                                height: searchInputLoader.item ? searchInputLoader.item.height : 0
                                color: Qt.rgba(0.45, 0.72, 1.0, 0.45)
                            }

                            Lr2TextRenderer {
                                z: 2
                                anchors.fill: parent
                                dsts: model.dsts
                                srcData: model.src
                                skinTime: elemLoader.elementSkinTime
                                activeOptions: elemLoader.elementActiveOptions
                                timers: elemLoader.elementTimers
                                chart: root.renderChart
                                scaleOverride: skinScale
                                resolvedText: parent.resolvedText
                            }

                            Loader {
                                id: searchInputLoader
                                z: 4
                                active: parent.isSearchText
                                sourceComponent: TextInput {
                                    id: searchInput

                                    property bool syncing: false
                                    readonly property var textState: searchInputLoader.parent.searchTextState

                                    function syncFromContext() {
                                        if (text === selectContext.searchText) {
                                            return;
                                        }
                                        syncing = true;
                                        text = selectContext.searchText;
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
                                        root.selectSearchInputItem = searchInput;
                                        syncFromContext();
                                    }

                                    Component.onDestruction: {
                                        if (root.selectSearchInputItem === searchInput) {
                                            root.selectSearchInputItem = null;
                                        }
                                    }

                                    onActiveFocusChanged: {
                                        if (activeFocus) {
                                            syncFromContext();
                                            cursorPosition = text.length;
                                        }
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onCursorPositionChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onSelectionStartChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onSelectionEndChanged: {
                                        searchInputLoader.parent.restartSearchCursorBlink();
                                    }

                                    onTextEdited: {
                                        if (!syncing && selectContext.searchText !== text) {
                                            selectContext.searchText = text;
                                            selectContext.touch();
                                        }
                                    }

                                    Keys.onReturnPressed: (event) => {
                                        event.accepted = true;
                                        root.submitSelectSearch();
                                        root.clearSelectSearchFocus();
                                    }

                                    Keys.onEnterPressed: (event) => {
                                        event.accepted = true;
                                        root.submitSelectSearch();
                                        root.clearSelectSearchFocus();
                                    }

                                    Keys.onEscapePressed: (event) => {
                                        event.accepted = true;
                                        root.resetSelectSearch();
                                    }

                                    Connections {
                                        target: selectContext
                                        function onSearchTextChanged() {
                                            const oldPosition = searchInput.cursorPosition;
                                            searchInput.syncFromContext();
                                            searchInput.cursorPosition = Math.min(
                                                oldPosition,
                                                searchInput.text.length);
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                z: 5
                                visible: parent.searchCursorVisible && searchInputLoader.item
                                x: parent.searchTextOriginX
                                    + searchCursorMeasure.naturalWidth * parent.searchTextScaleX
                                y: parent.searchTextOriginY
                                width: Math.max(1, skinScale)
                                height: searchInputLoader.item ? searchInputLoader.item.height : 0
                                color: "white"
                            }

                            MouseArea {
                                id: searchEditMouseArea
                                z: 6
                                enabled: !!parent.searchTextState
                                acceptedButtons: Qt.LeftButton
                                preventStealing: true
                                x: parent.searchTextState ? Math.min(parent.searchTextState.x, parent.searchTextState.x + parent.searchTextState.w) * skinScale : 0
                                y: parent.searchTextState ? Math.min(parent.searchTextState.y, parent.searchTextState.y + parent.searchTextState.h) * skinScale : 0
                                width: parent.searchTextState ? Math.abs(parent.searchTextState.w) * skinScale : 0
                                height: parent.searchTextState ? Math.abs(parent.searchTextState.h) * skinScale : 0
                                onPressed: (mouse) => {
                                    mouse.accepted = true;
                                    parent.moveSearchCursorTo(x + mouse.x, false);
                                }
                                onPositionChanged: (mouse) => {
                                    if (pressed) {
                                        mouse.accepted = true;
                                        parent.moveSearchCursorTo(x + mouse.x, true);
                                    }
                                }
                            }
                        }
                    }

                    Component {
                        id: readmeTextComponent
                        Item {
                            id: readmeTextDelegateRoot
                            width: skinW * skinScale
                            height: skinH * skinScale
                            clip: true
                            readonly property var readmeSrc: model.src
                            readonly property var readmeDsts: model.dsts

                            Component.onCompleted: {
                                if (readmeSrc && readmeSrc.readme && readmeSrc.readmeId === 0) {
                                    root.lr2ReadmeLineSpacing = Math.max(1, readmeSrc.readmeLineSpacing || 18);
                                    root.clampReadmeOffsets();
                                }
                            }

                            Repeater {
                                model: root.readmeLinesForSource(readmeTextDelegateRoot.readmeSrc)

                                Lr2TextRenderer {
                                    anchors.fill: parent
                                    dsts: readmeTextDelegateRoot.readmeDsts
                                    srcData: readmeTextDelegateRoot.readmeSrc
                                    skinTime: root.renderSkinTime
                                    activeOptions: elemLoader.elementActiveOptions
                                    timers: elemLoader.elementTimers
                                    chart: root.renderChart
                                    scaleOverride: skinScale
                                    offsetX: root.lr2ReadmeOffsetX
                                    offsetY: root.lr2ReadmeOffsetY
                                        + index * (readmeTextDelegateRoot.readmeSrc
                                            ? readmeTextDelegateRoot.readmeSrc.readmeLineSpacing
                                            : root.lr2ReadmeLineSpacing)
                                    resolvedText: modelData
                                }
                            }
                        }
                    }

                    Component {
                        id: barImageComponent
                        Lr2BarSpriteRenderer {
                            readonly property bool sourceAnimates: root.sourceHasFrameAnimation(model.src)
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            sourceSkinTime: root.effectiveScreenKey === "select"
                                ? (sourceAnimates ? root.selectSourceSkinTime : root.renderSkinTime)
                                : root.renderSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barLampVariants: skinModel.barLampVariants
                            barBaseStates: root.cachedBarBaseStates
                            barDrawXs: root.cachedBarDrawXs
                            barDrawYs: root.cachedBarDrawYs
                            barCells: root.cachedBarRowCells
                            barScrollOffset: root.barSpriteScrollOffset(model.src)
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                            transColor: skinModel.transColor
                            colorKeyEnabled: skinModel.hasTransColor
                        }
                    }

                    Component {
                        id: barTextComponent
                        Lr2BarTextRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barDrawXs: root.cachedBarDrawXs
                            barDrawYs: root.cachedBarDrawYs
                            barCells: selectContext.visibleBarTextCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                        }
                    }

                    Component {
                        id: barNumberComponent
                        Lr2BarNumberRenderer {
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: root.barSkinTime
                            activeOptions: root.barActiveOptions
                            timers: root.barTimers
                            scaleOverride: skinScale
                            selectContext: root.selectContextRef
                            barRows: skinModel.barRows
                            barBaseStates: root.cachedBarBaseStates
                            barDrawXs: root.cachedBarDrawXs
                            barDrawYs: root.cachedBarDrawYs
                            barCells: root.cachedBarRowCells
                            fastBarScrollActive: root.fastBarScrollActive
                            fastBarScrollX: root.fastBarScrollX
                            fastBarScrollY: root.fastBarScrollY
                            selectedFastBarDrawX: root.selectedFastBarDrawX
                            selectedFastBarDrawY: root.selectedFastBarDrawY
                            barCenter: skinModel.barCenter
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                        }
                    }

                    Component {
                        id: barGraphComponent
                        Lr2BarGraphRenderer {
                            readonly property bool sourceAnimates: root.sourceHasFrameAnimation(model.src)
                            dsts: model.dsts
                            srcData: model.src
                            skinTime: elemLoader.elementSkinTime
                            sourceSkinTime: root.effectiveScreenKey === "select" && sourceAnimates
                                ? root.selectSourceSkinTime
                                : root.renderSkinTime
                            activeOptions: elemLoader.elementActiveOptions
                            timers: elemLoader.elementTimers
                            chart: root.renderChart
                            scaleOverride: skinScale
                            colorKeyEnabled: skinModel.hasTransColor
                            transColor: skinModel.transColor
                            value: root.resolveBarGraph(model.src ? model.src.graphType : 0)
                            animateValue: root.effectiveScreenKey === "select"
                                && model.src
                                && model.src.graphType >= 5
                                && model.src.graphType <= 9
                        }
                    }
                }
            }

            Text {
                visible: root.effectiveScreenKey === "select" && root.clearStatusIsBest()
                text: "BEST"
                x: 80 * skinScale
                y: 459 * skinScale
                z: 100200
                color: "white"
                font.family: "Arial"
                font.pixelSize: 6 * skinScale
                font.bold: true
                renderType: Text.NativeRendering
            }

            MouseArea {
                id: barRowsMouseArea
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select"
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                propagateComposedEvents: true
                z: -90000
                property int pressedRow: -1

                function rowAt(mouse) {
                    if (!root.selectPointerScrollReady()) {
                        return -1;
                    }
                    let states = root.cachedBarBaseStates || [];
                    let px = mouse.x / skinScale;
                    let py = mouse.y / skinScale;
                    for (let row = root.barClickEnd(); row >= root.barClickStart(); --row) {
                        let state = row >= 0 && row < states.length ? states[row] : null;
                        if (!state) {
                            continue;
                        }
                        let left = Math.min(state.x, state.x + state.w);
                        let right = Math.max(state.x, state.x + state.w);
                        let top = Math.min(state.y, state.y + state.h);
                        let bottom = Math.max(state.y, state.y + state.h);
                        if (px >= left && px <= right && py >= top && py <= bottom) {
                            return row;
                        }
                    }
                    return -1;
                }

                onPressed: (mouse) => {
                    root.clearSelectSearchFocus();
                    pressedRow = rowAt(mouse);
                    if (pressedRow < 0) {
                        mouse.accepted = false;
                    }
                }
                onClicked: (mouse) => {
                    let row = pressedRow >= 0 ? pressedRow : rowAt(mouse);
                    pressedRow = -1;
                    if (row < 0) {
                        mouse.accepted = false;
                        return;
                    }
                    root.handleBarRowClick(row, mouse);
                }
                onDoubleClicked: (mouse) => {
                    let row = pressedRow >= 0 ? pressedRow : rowAt(mouse);
                    pressedRow = -1;
                    if (row < 0 || mouse.button !== Qt.LeftButton) {
                        mouse.accepted = false;
                        return;
                    }
                    selectContext.selectVisibleRow(row, skinModel.barCenter);
                    root.selectGoForward(selectContext.current);
                }
                onCanceled: pressedRow = -1
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }

            Repeater {
                model: skinModel

                MouseArea {
                    id: sliderMouseArea
                    readonly property bool selectScroll: root.isSelectScrollSlider(model.src)
                    readonly property bool genericSlider: root.isLr2GenericSlider(model.src)
                    readonly property bool sliderActive: selectScroll || genericSlider
                    readonly property bool usesLiveSelectClock: sliderActive
                        && root.elementUsesLiveSelectClock(model.src, model.dsts)
                    readonly property int sliderSkinClock: sliderActive
                        ? (usesLiveSelectClock ? root.selectSourceSkinTime : root.renderSkinTime)
                        : 0
                    readonly property var trackState: !sliderActive
                        ? null
                        : (selectScroll
                            ? root.selectScrollSliderTrackState(model.src, model.dsts, sliderSkinClock)
                            : root.lr2GenericSliderTrackState(model.src, model.dsts, sliderSkinClock))
                    enabled: root.selectPointerScrollReady() && !!trackState
                    acceptedButtons: Qt.LeftButton
                    preventStealing: true
                    x: trackState ? Math.min(trackState.x, trackState.x + trackState.w) * skinScale : 0
                    y: trackState ? Math.min(trackState.y, trackState.y + trackState.h) * skinScale : 0
                    width: trackState ? Math.abs(trackState.w) * skinScale : 0
                    height: trackState ? Math.abs(trackState.h) * skinScale : 0
                    z: 100300 + index
                    function updateSlider(mouse) {
                        let pointerX = (x + mouse.x) / skinScale;
                        let pointerY = (y + mouse.y) / skinScale;
                        if (selectScroll) {
                            root.setSelectScrollFromSliderTrack(model.src,
                                                                trackState,
                                                                pointerX,
                                                                pointerY);
                        } else if (genericSlider) {
                            root.setLr2GenericSliderFromTrack(model.src,
                                                              trackState,
                                                              pointerX,
                                                              pointerY);
                        }
                    }
                    onPressed: (mouse) => {
                        root.clearSelectSearchFocus();
                        root.selectSliderFixedPoint = -1;
                        if (selectScroll) {
                            root.selectScrollStartSkinTime = root.renderSkinTime;
                            selectContext.beginScrollFixedPointDrag();
                        }
                        updateSlider(mouse);
                        mouse.accepted = true;
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            updateSlider(mouse);
                        }
                    }
                    onReleased: {
                        if (selectScroll) {
                            selectContext.finishScrollFixedPoint(100);
                            root.selectSliderFixedPoint = -1;
                        }
                    }
                    onCanceled: {
                        if (selectScroll) {
                            selectContext.finishScrollFixedPoint(100);
                            root.selectSliderFixedPoint = -1;
                        }
                    }
                    onWheel: (wheel) => root.handleSelectWheel(wheel)
                }
            }

            Lr2NativeCursor {
                id: nativeCursor
                anchors.fill: parent

                readonly property var cursorSrcData: skinModel.mouseCursor && skinModel.mouseCursor.src
                    ? skinModel.mouseCursor.src
                    : null
                readonly property var cursorDsts: skinModel.mouseCursor && skinModel.mouseCursor.dsts
                    ? skinModel.mouseCursor.dsts
                    : []
                readonly property var cursorState: cursorSrcData
                    ? Lr2Timeline.getCurrentState(cursorDsts,
                                                  root.renderSkinTime,
                                                  root.timers,
                                                  root.runtimeActiveOptions)
                    : null
                readonly property bool wholeTextureSource: cursorSrcData
                    && (cursorSrcData.x < 0 || cursorSrcData.y < 0
                        || cursorSrcData.w < 0 || cursorSrcData.h < 0)
                readonly property bool croppedTextureSource: cursorSrcData
                    && cursorSrcData.w > 0 && cursorSrcData.h > 0
                readonly property string resolvedSource: {
                    if (!cursorSrcData || !cursorSrcData.source) {
                        return "";
                    }
                    let absPath = cursorSrcData.source.replace(/\\/g, "/");
                    if (/^[A-Za-z]:\//.test(absPath)) {
                        return "file:///" + absPath;
                    }
                    if (absPath.startsWith("/")) {
                        return "file://" + absPath;
                    }
                    return absPath;
                }
                readonly property int frameIndex: {
                    if (!cursorSrcData) {
                        return 0;
                    }
                    let timerIdx = cursorSrcData.timer || 0;
                    let fire = root.timers && root.timers[timerIdx] !== undefined
                        ? root.timers[timerIdx]
                        : -1;
                    return Lr2Timeline.getAnimationFrame(cursorSrcData,
                                                         root.renderSkinTime,
                                                         fire);
                }
                readonly property rect clipRect: {
                    if (!cursorSrcData || wholeTextureSource || !croppedTextureSource) {
                        return Qt.rect(0, 0, 0, 0);
                    }

                    let sx = Math.max(0, cursorSrcData.x || 0);
                    let sy = Math.max(0, cursorSrcData.y || 0);
                    let sw = cursorSrcData.w;
                    let sh = cursorSrcData.h;
                    let divX = Math.max(1, cursorSrcData.div_x || 1);
                    let divY = Math.max(1, cursorSrcData.div_y || 1);
                    let cellW = sw / divX;
                    let cellH = sh / divY;
                    let col = frameIndex % divX;
                    let row = Math.floor(frameIndex / divX) % divY;

                    return Qt.rect(sx + col * cellW, sy + row * cellH, cellW, cellH);
                }

                active: root.effectiveScreenKey === "select"
                    && resolvedSource !== ""
                source: resolvedSource
                sourceRect: clipRect
                targetSize: cursorState
                    ? Qt.size(cursorState.w * root.skinVisualScaleX, cursorState.h * root.skinVisualScaleY)
                    : Qt.size(0, 0)
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select"
                acceptedButtons: Qt.NoButton
                z: 100200
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }

            MouseArea {
                id: readmeMouseArea
                anchors.fill: parent
                enabled: root.effectiveScreenKey === "select" && root.lr2ReadmeMode === 1
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                z: 100250

                function updateReadmeMouse(mouse) {
                    const point = readmeMouseArea.mapToItem(skinContainer, mouse.x, mouse.y);
                    root.lr2ReadmeMouseX = point.x / skinScale;
                    root.lr2ReadmeMouseY = point.y / skinScale;
                }

                onPressed: (mouse) => {
                    updateReadmeMouse(mouse);
                    if (mouse.button === Qt.RightButton) {
                        root.closeReadme();
                    } else {
                        root.lr2ReadmeMouseHeld = true;
                    }
                    mouse.accepted = true;
                }
                onPositionChanged: (mouse) => {
                    updateReadmeMouse(mouse);
                    mouse.accepted = true;
                }
                onReleased: (mouse) => {
                    updateReadmeMouse(mouse);
                    root.lr2ReadmeMouseHeld = false;
                    mouse.accepted = true;
                }
                onCanceled: root.lr2ReadmeMouseHeld = false
                onWheel: (wheel) => root.handleSelectWheel(wheel)
            }
        }
    }
}
