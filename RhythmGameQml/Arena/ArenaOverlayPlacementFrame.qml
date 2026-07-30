pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    required property var themeVars
    required property Item viewport
    required property string placementKind
    required property string layoutVariant
    readonly property bool forcedVisible: true
    property size minimumPixelSize: Qt.size(280, 160)
    property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)
    property Item moveHandle: null
    property bool customizeMode: false
    property string customizationLabel: qsTr("Arena panel")
    property bool defaultExpanded: false
    property bool visibilityCustomizable: true
    property bool overlayVisible: true
    property bool expanded: defaultExpanded
    property bool chatSelected: false
    default property alias contentData: contentHost.data
    readonly property real viewportScale: viewport
        ? Math.min(viewport.width / 1920,
                   viewport.height / 1080) : 1
    readonly property real effectiveContentScale: viewportScale > 0
        ? viewportScale : 1
    readonly property size effectiveMinimumPixelSize: Qt.size(
        minimumPixelSize.width * viewportScale,
        minimumPixelSize.height * viewportScale)
    readonly property bool interactionActive: moveHandler.active
                                              || resizeInteractionCount > 0

    signal interactionStateChanged(bool active)
    signal placementCommitted()
    signal presentationStateReloaded()
    signal overlayVisibilityCommitted(bool visible)

    property rect resolvedPixelRect: Qt.rect(0, 0, 1, 1)
    property var sourcePlacement: ({ "stored": false })
    property rect moveStartRect: Qt.rect(0, 0, 1, 1)
    property rect resizeStartRect: Qt.rect(0, 0, 1, 1)
    property int resizeInteractionCount: 0
    property bool presentationStateLoaded: false
    readonly property real resizeEdgeInset: 8 * viewportScale

    x: resolvedPixelRect.x
    y: resolvedPixelRect.y
    width: resolvedPixelRect.width
    height: resolvedPixelRect.height
    visible: forcedVisible

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Arena overlay placement")
    Accessible.description: qsTr("Drag to move. Drag an edge or corner to resize.")

    onInteractionActiveChanged: interactionStateChanged(interactionActive)

    function bounded(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function safeMargin(length) {
        return Math.min(24 * viewportScale,
                        Math.max(0, (length - 1) / 2));
    }

    function safePixelRect() {
        const marginX = safeMargin(viewport ? viewport.width : 0);
        const marginY = safeMargin(viewport ? viewport.height : 0);
        const viewportWidth = viewport ? viewport.width : 0;
        const viewportHeight = viewport ? viewport.height : 0;
        return Qt.rect(marginX, marginY,
                       Math.max(0, viewportWidth - 2 * marginX),
                       Math.max(0, viewportHeight - 2 * marginY));
    }

    function defaultPixelRect() {
        if (validDefaultPixelRectHint())
            return clampPixelRect(defaultPixelRectHint);
        const viewportWidth = viewport ? viewport.width : 0;
        const viewportHeight = viewport ? viewport.height : 0;
        const safe = safePixelRect();
        const result = placementKind === "resultStandings";
        const requestedWidth = Math.max((result ? 360 : 320)
                                        * viewportScale,
                                        Math.min((result ? 560 : 420)
                                                 * viewportScale,
                                                 viewportWidth * (result ? 0.40 : 0.30)));
        const requestedHeight = Math.max((result ? 260 : 240)
                                         * viewportScale,
                                         viewportHeight * (result ? 0.60 : 0.44));
        const width = Math.min(safe.width, requestedWidth);
        const height = Math.min(safe.height, requestedHeight);
        const requestedX = Math.max(0, viewportWidth
                                    - 24 * viewportScale - width);
        const requestedY = Math.min(24 * viewportScale,
                                    Math.max(0, viewportHeight - height));
        return clampPixelRect(Qt.rect(requestedX, requestedY, width, height));
    }

    function validDefaultPixelRectHint() {
        return Number.isFinite(defaultPixelRectHint.x)
                && Number.isFinite(defaultPixelRectHint.y)
                && Number.isFinite(defaultPixelRectHint.width)
                && Number.isFinite(defaultPixelRectHint.height)
                && defaultPixelRectHint.width > 0
                && defaultPixelRectHint.height > 0;
    }

    function clampPixelRect(candidate) {
        const safe = safePixelRect();
        const minimumWidth = Math.min(safe.width,
                                      effectiveMinimumPixelSize.width);
        const minimumHeight = Math.min(safe.height,
                                       effectiveMinimumPixelSize.height);
        const width = Math.min(safe.width,
                               Math.max(minimumWidth, candidate.width));
        const height = Math.min(safe.height,
                                Math.max(minimumHeight, candidate.height));
        const x = bounded(candidate.x, safe.x, safe.x + safe.width - width);
        const y = bounded(candidate.y, safe.y, safe.y + safe.height - height);
        return Qt.rect(x, y, width, height);
    }

    function validStoredPlacement(record) {
        return record && record.stored === true
                && typeof record.x === "number"
                && typeof record.y === "number"
                && typeof record.width === "number"
                && typeof record.height === "number"
                && Number.isFinite(record.x)
                && Number.isFinite(record.y)
                && Number.isFinite(record.width)
                && Number.isFinite(record.height)
                && record.x >= 0 && record.y >= 0
                && record.width > 0 && record.height > 0
                && record.x + record.width <= 1.000001
                && record.y + record.height <= 1.000001;
    }

    function pixelRectForPlacement(record) {
        if (!validStoredPlacement(record))
            return defaultPixelRect();
        return clampPixelRect(Qt.rect(Number(record.x) * viewport.width,
                                      Number(record.y) * viewport.height,
                                      Number(record.width) * viewport.width,
                                      Number(record.height) * viewport.height));
    }

    function canLoadPlacement() {
        const result = placementKind === "resultStandings";
        const gameplay = placementKind === "gameplayLeaderboard";
        const select = placementKind === "selectRoom";
        return !!themeVars && !!viewport && propertyPrefix().length > 0
                && ((result && layoutVariant === "result")
                    || (gameplay && layoutVariant !== "result"
                        && layoutVariant !== "select")
                    || (select && layoutVariant === "select"));
    }

    function propertyPrefix() {
        switch (layoutVariant) {
        case "k5": return "arenaOverlayK5";
        case "k7": return "arenaOverlayK7";
        case "k10": return "arenaOverlayK10";
        case "k14": return "arenaOverlayK14";
        case "result": return "arenaOverlayResult";
        case "select": return "arenaOverlaySelect";
        default: return "";
        }
    }

    function propertyKey(suffix) {
        return propertyPrefix() + suffix;
    }

    function placementFromThemeVars() {
        if (!canLoadPlacement())
            return ({ "stored": false });
        return {
            "stored": true,
            "x": themeVars[propertyKey("XNormalized")],
            "y": themeVars[propertyKey("YNormalized")],
            "width": themeVars[propertyKey("WidthNormalized")],
            "height": themeVars[propertyKey("HeightNormalized")]
        };
    }

    function storedBoolean(suffix, fallback) {
        if (!canLoadPlacement())
            return fallback;
        const value = themeVars[propertyKey(suffix)];
        return typeof value === "boolean" ? value : fallback;
    }

    function reloadPresentationState() {
        const storedVisible = storedBoolean("Visible", true);
        overlayVisible = visibilityCustomizable ? storedVisible : true;
        if (!visibilityCustomizable && storedVisible === false)
            commitBoolean("Visible", true);
        expanded = storedBoolean("Expanded", defaultExpanded);
        chatSelected = storedBoolean("ChatSelected", false);
        presentationStateLoaded = canLoadPlacement();
        presentationStateReloaded();
    }

    function commitBoolean(suffix, value) {
        if (!canLoadPlacement())
            return;
        themeVars[propertyKey(suffix)] = !!value;
    }

    function setOverlayVisible(value) {
        const next = visibilityCustomizable ? !!value : true;
        if (overlayVisible === next)
            return;
        overlayVisible = next;
        commitBoolean("Visible", next);
        overlayVisibilityCommitted(next);
    }

    function setExpanded(value) {
        const next = !!value;
        if (expanded === next)
            return;
        expanded = next;
        commitBoolean("Expanded", next);
    }

    function setChatSelected(value) {
        const next = !!value;
        if (chatSelected === next)
            return;
        chatSelected = next;
        commitBoolean("ChatSelected", next);
    }

    function restoreChatSelection(session) {
        if (session)
            session.setChatOpen(overlayVisible && chatSelected);
    }

    function reloadPlacement() {
        if (!canLoadPlacement())
            return;
        const loaded = placementFromThemeVars();
        sourcePlacement = validStoredPlacement(loaded)
                ? loaded : ({ "stored": false });
        resolvedPixelRect = pixelRectForPlacement(sourcePlacement);
    }

    function normalizedResolvedPlacement() {
        if (!viewport || viewport.width <= 0 || viewport.height <= 0)
            return null;
        return {
            "stored": true,
            "x": bounded(resolvedPixelRect.x / viewport.width, 0, 1),
            "y": bounded(resolvedPixelRect.y / viewport.height, 0, 1),
            "width": bounded(resolvedPixelRect.width / viewport.width, 0, 1),
            "height": bounded(resolvedPixelRect.height / viewport.height, 0, 1)
        };
    }

    function commitResolvedPlacement() {
        const normalized = normalizedResolvedPlacement();
        if (!normalized || !canLoadPlacement())
            return;
        themeVars[propertyKey("XNormalized")] = normalized.x;
        themeVars[propertyKey("YNormalized")] = normalized.y;
        themeVars[propertyKey("WidthNormalized")] = normalized.width;
        themeVars[propertyKey("HeightNormalized")] = normalized.height;
        sourcePlacement = normalized;
        placementCommitted();
    }

    function resizeFrom(start, x, y, horizontalEdge, verticalEdge) {
        const safe = safePixelRect();
        const minimumWidth = Math.min(safe.width,
                                      effectiveMinimumPixelSize.width);
        const minimumHeight = Math.min(safe.height,
                                       effectiveMinimumPixelSize.height);
        let left = start.x;
        let top = start.y;
        let right = start.x + start.width;
        let bottom = start.y + start.height;
        if (horizontalEdge < 0)
            left = bounded(start.x + x, safe.x, right - minimumWidth);
        else if (horizontalEdge > 0)
            right = bounded(start.x + start.width + x,
                            left + minimumWidth, safe.x + safe.width);
        if (verticalEdge < 0)
            top = bounded(start.y + y, safe.y, bottom - minimumHeight);
        else if (verticalEdge > 0)
            bottom = bounded(start.y + start.height + y,
                             top + minimumHeight, safe.y + safe.height);
        resolvedPixelRect = Qt.rect(left, top, right - left, bottom - top);
    }

    function beginResize(horizontalEdge, verticalEdge) {
        if (resizeInteractionCount === 0)
            resizeStartRect = resolvedPixelRect;
        resizeInteractionCount += 1;
    }

    function updateResize(x, y, horizontalEdge, verticalEdge) {
        resizeFrom(resizeStartRect, x, y, horizontalEdge, verticalEdge);
    }

    function endResize() {
        resizeInteractionCount = Math.max(0, resizeInteractionCount - 1);
        if (resizeInteractionCount === 0)
            commitResolvedPlacement();
    }

    function resizeByHandle(x, y, horizontalEdge, verticalEdge) {
        const start = resolvedPixelRect;
        resizeFrom(start, x, y, horizontalEdge, verticalEdge);
        commitResolvedPlacement();
    }

    onThemeVarsChanged: {
        reloadPlacement();
        reloadPresentationState();
    }
    onViewportChanged: {
        reloadPlacement();
        reloadPresentationState();
    }
    onPlacementKindChanged: reloadPlacement()
    onLayoutVariantChanged: {
        reloadPlacement();
        reloadPresentationState();
    }
    onMinimumPixelSizeChanged: reloadPlacement()
    onDefaultPixelRectHintChanged: reloadPlacement()
    onDefaultExpandedChanged: reloadPresentationState()
    onVisibilityCustomizableChanged: reloadPresentationState()

    Component.onCompleted: {
        reloadPlacement();
        reloadPresentationState();
    }

    Connections {
        target: root.viewport
        enabled: root.viewport !== null

        function onWidthChanged() {
            root.resolvedPixelRect = root.pixelRectForPlacement(root.sourcePlacement);
        }

        function onHeightChanged() {
            root.resolvedPixelRect = root.pixelRectForPlacement(root.sourcePlacement);
        }
    }

    Connections {
        target: root.themeVars
        enabled: root.themeVars !== null && root.themeVars !== undefined

        function onValueChanged(key, value) {
            if (key === root.propertyKey("Visible")) {
                root.overlayVisible = root.visibilityCustomizable
                    ? (typeof value === "boolean" ? value : true) : true;
            } else if (key === root.propertyKey("Expanded")) {
                root.expanded = typeof value === "boolean"
                    ? value : root.defaultExpanded;
            } else if (key === root.propertyKey("ChatSelected")) {
                root.chatSelected = typeof value === "boolean" ? value : false;
            }
        }
    }

    Item {
        id: contentHost

        width: root.width / root.effectiveContentScale
        height: root.height / root.effectiveContentScale
        scale: root.effectiveContentScale
        transformOrigin: Item.TopLeft
        visible: root.overlayVisible
    }

    Rectangle {
        id: customizationChrome

        anchors.fill: parent
        border.color: root.overlayVisible ? "#b8d7f2" : "#ffe38a"
        border.width: Math.max(1, 2 * root.viewportScale)
        color: root.overlayVisible ? "transparent" : "#b018202a"
        radius: Math.max(2, 5 * root.viewportScale)
        visible: root.customizeMode
        z: 1000

        Label {
            anchors.centerIn: parent
            color: "#ffe38a"
            font.bold: true
            font.pixelSize: Math.max(12, 16 * root.viewportScale)
            text: qsTr("%1 is hidden").arg(root.customizationLabel)
            visible: !root.overlayVisible
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            enabled: root.visibilityCustomizable

            onClicked: visibilityMenu.popup()
        }
    }

    Menu {
        id: visibilityMenu

        MenuItem {
            checkable: true
            checked: root.overlayVisible
            enabled: root.visibilityCustomizable
            text: qsTr("Show Arena panel")
            visible: root.visibilityCustomizable

            onTriggered: root.setOverlayVisible(!root.overlayVisible)
        }
    }

    DragHandler {
        id: moveHandler

        parent: root.moveHandle || root
        target: null
        enabled: true
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (active) {
                root.moveStartRect = root.resolvedPixelRect;
            } else {
                root.commitResolvedPlacement();
            }
        }
        onTranslationChanged: {
            if (active) {
                root.resolvedPixelRect = root.clampPixelRect(
                                    Qt.rect(root.moveStartRect.x
                                            + activeTranslation.x,
                                            root.moveStartRect.y
                                            + activeTranslation.y,
                                    root.moveStartRect.width,
                                    root.moveStartRect.height));
            }
        }
    }

    component ResizeHandle: ArenaOverlayResizeHandle {
        required property string handleObjectName

        objectName: handleObjectName
        interactionEnabled: true
        metricScale: root.viewportScale

        onInteractionStarted: function(horizontalEdge, verticalEdge) {
            root.beginResize(horizontalEdge, verticalEdge);
        }
        onInteractionDelta: function(x, y, horizontalEdge, verticalEdge) {
            root.updateResize(x, y, horizontalEdge, verticalEdge);
        }
        onInteractionEnded: root.endResize()
        onKeyboardResizeRequested: function(x, y, horizontalEdge,
                                            verticalEdge) {
            root.resizeByHandle(x, y, horizontalEdge, verticalEdge);
        }
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTopLeft"
        accessibleName: qsTr("Resize Arena overlay from top left")
        horizontalEdge: -1
        verticalEdge: -1
        x: -width / 2
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTop"
        accessibleName: qsTr("Resize Arena overlay from top")
        verticalEdge: -1
        width: Math.max(16 * root.viewportScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTopRight"
        accessibleName: qsTr("Resize Arena overlay from top right")
        horizontalEdge: 1
        verticalEdge: -1
        x: root.width - width / 2
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeRight"
        accessibleName: qsTr("Resize Arena overlay from right")
        horizontalEdge: 1
        height: Math.max(16 * root.viewportScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: root.width - width / 2
        y: root.resizeEdgeInset
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottomRight"
        accessibleName: qsTr("Resize Arena overlay from bottom right")
        horizontalEdge: 1
        verticalEdge: 1
        x: root.width - width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottom"
        accessibleName: qsTr("Resize Arena overlay from bottom")
        verticalEdge: 1
        width: Math.max(16 * root.viewportScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottomLeft"
        accessibleName: qsTr("Resize Arena overlay from bottom left")
        horizontalEdge: -1
        verticalEdge: 1
        x: -width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeLeft"
        accessibleName: qsTr("Resize Arena overlay from left")
        horizontalEdge: -1
        height: Math.max(16 * root.viewportScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: -width / 2
        y: root.resizeEdgeInset
    }
}
