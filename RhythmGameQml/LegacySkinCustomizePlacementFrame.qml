pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var themeVars
    required property Item viewport
    required property Item moveHandle
    default property alias contentData: contentHost.data
    property size minimumContentSize: Qt.size(320, 240)
    property string propertyPrefix: "legacySkinCustomizePanel"
    readonly property real viewportScale: viewport
        ? Math.min(viewport.width / 1920, viewport.height / 1080) : 1
    readonly property real effectiveContentScale: viewportScale > 0
        ? viewportScale : 1
    readonly property size effectiveMinimumPixelSize: Qt.size(
        minimumContentSize.width * effectiveContentScale,
        minimumContentSize.height * effectiveContentScale)
    readonly property real resizeEdgeInset: 8 * effectiveContentScale
    readonly property bool moveInteractionActive: moveHandler.active

    property rect resolvedPixelRect: Qt.rect(0, 0, 1, 1)
    property var sourcePlacement: ({ "stored": false })
    property rect moveStartRect: Qt.rect(0, 0, 1, 1)
    property rect resizeStartRect: Qt.rect(0, 0, 1, 1)
    property int resizeInteractionCount: 0

    x: resolvedPixelRect.x
    y: resolvedPixelRect.y
    width: resolvedPixelRect.width
    height: resolvedPixelRect.height

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Skin customization panel placement")
    Accessible.description: qsTr("Drag the header to move. Drag an edge or corner to resize.")

    function bounded(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function safeMargin(length) {
        return Math.min(16 * effectiveContentScale,
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
        const safe = safePixelRect();
        const requestedWidth = Math.min(440 * effectiveContentScale,
                                        Math.max(320 * effectiveContentScale,
                                                 viewport.width * 0.32));
        const width = Math.min(safe.width, requestedWidth);
        const height = safe.height;
        return clampPixelRect(Qt.rect(safe.x + safe.width - width,
                                      safe.y, width, height));
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

    function hasUsableViewport() {
        return !!viewport && viewport.width > 0 && viewport.height > 0;
    }

    function canPersistPlacement() {
        return hasUsableViewport() && !!themeVars
                && propertyPrefix.length > 0;
    }

    function propertyKey(suffix) {
        return propertyPrefix + suffix;
    }

    function placementFromThemeVars() {
        if (!canPersistPlacement()) {
            return ({ "stored": false });
        }
        return {
            "stored": true,
            "x": themeVars[propertyKey("XNormalized")],
            "y": themeVars[propertyKey("YNormalized")],
            "width": themeVars[propertyKey("WidthNormalized")],
            "height": themeVars[propertyKey("HeightNormalized")]
        };
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
        if (!validStoredPlacement(record)) {
            return defaultPixelRect();
        }
        return clampPixelRect(Qt.rect(Number(record.x) * viewport.width,
                                      Number(record.y) * viewport.height,
                                      Number(record.width) * viewport.width,
                                      Number(record.height) * viewport.height));
    }

    function reloadPlacement() {
        if (!hasUsableViewport()) {
            return;
        }
        const loaded = placementFromThemeVars();
        sourcePlacement = validStoredPlacement(loaded)
                ? loaded : ({ "stored": false });
        resolvedPixelRect = pixelRectForPlacement(sourcePlacement);
    }

    function normalizedResolvedPlacement() {
        if (!hasUsableViewport()) {
            return null;
        }
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
        if (!normalized) {
            return;
        }
        sourcePlacement = normalized;
        if (!canPersistPlacement()) {
            return;
        }
        themeVars[propertyKey("XNormalized")] = normalized.x;
        themeVars[propertyKey("YNormalized")] = normalized.y;
        themeVars[propertyKey("WidthNormalized")] = normalized.width;
        themeVars[propertyKey("HeightNormalized")] = normalized.height;
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
        if (horizontalEdge < 0) {
            left = bounded(start.x + x, safe.x, right - minimumWidth);
        } else if (horizontalEdge > 0) {
            right = bounded(start.x + start.width + x,
                            left + minimumWidth, safe.x + safe.width);
        }
        if (verticalEdge < 0) {
            top = bounded(start.y + y, safe.y, bottom - minimumHeight);
        } else if (verticalEdge > 0) {
            bottom = bounded(start.y + start.height + y,
                             top + minimumHeight, safe.y + safe.height);
        }
        resolvedPixelRect = Qt.rect(left, top, right - left, bottom - top);
    }

    function beginResize(horizontalEdge, verticalEdge) {
        if (resizeInteractionCount === 0) {
            resizeStartRect = resolvedPixelRect;
        }
        resizeInteractionCount += 1;
    }

    function updateResize(x, y, horizontalEdge, verticalEdge) {
        resizeFrom(resizeStartRect, x, y, horizontalEdge, verticalEdge);
    }

    function endResize() {
        resizeInteractionCount = Math.max(0, resizeInteractionCount - 1);
        if (resizeInteractionCount === 0) {
            commitResolvedPlacement();
        }
    }

    function resizeByHandle(x, y, horizontalEdge, verticalEdge) {
        resizeFrom(resolvedPixelRect, x, y, horizontalEdge, verticalEdge);
        commitResolvedPlacement();
    }

    onThemeVarsChanged: reloadPlacement()
    onViewportChanged: reloadPlacement()
    onMinimumContentSizeChanged: reloadPlacement()
    onPropertyPrefixChanged: reloadPlacement()

    Component.onCompleted: reloadPlacement()

    Connections {
        target: root.viewport
        enabled: root.viewport !== null

        function onWidthChanged() {
            root.resolvedPixelRect =
                root.pixelRectForPlacement(root.sourcePlacement);
        }

        function onHeightChanged() {
            root.resolvedPixelRect =
                root.pixelRectForPlacement(root.sourcePlacement);
        }
    }

    Item {
        id: contentHost

        width: root.width / root.effectiveContentScale
        height: root.height / root.effectiveContentScale
        scale: root.effectiveContentScale
        transformOrigin: Item.TopLeft
    }

    DragHandler {
        id: moveHandler

        parent: root.moveHandle
        target: null
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

    HoverHandler {
        parent: root.moveHandle
        cursorShape: root.moveInteractionActive
            ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    }

    component ResizeHandle: OverlayResizeHandle {
        interactionEnabled: true
        metricScale: root.effectiveContentScale

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
        accessibleName: qsTr("Resize customization panel from top left")
        horizontalEdge: -1
        verticalEdge: -1
        x: -width / 2
        y: -height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from top")
        verticalEdge: -1
        width: Math.max(16 * root.effectiveContentScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: -height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from top right")
        horizontalEdge: 1
        verticalEdge: -1
        x: root.width - width / 2
        y: -height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from right")
        horizontalEdge: 1
        height: Math.max(16 * root.effectiveContentScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: root.width - width / 2
        y: root.resizeEdgeInset
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from bottom right")
        horizontalEdge: 1
        verticalEdge: 1
        x: root.width - width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from bottom")
        verticalEdge: 1
        width: Math.max(16 * root.effectiveContentScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: root.height - height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from bottom left")
        horizontalEdge: -1
        verticalEdge: 1
        x: -width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        accessibleName: qsTr("Resize customization panel from left")
        horizontalEdge: -1
        height: Math.max(16 * root.effectiveContentScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: -width / 2
        y: root.resizeEdgeInset
    }
}
