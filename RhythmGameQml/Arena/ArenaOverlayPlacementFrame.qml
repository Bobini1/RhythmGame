pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property var themeVars
    required property Item viewport
    required property string placementKind
    required property string layoutVariant
    required property bool customizeMode
    readonly property bool forcedVisible: true
    property size minimumPixelSize: Qt.size(280, 160)
    property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)
    property bool directMoveEnabled: false
    property bool directResizeEnabled: false
    property Item moveHandle: null
    default property alias contentData: contentHost.data
    property size minimumScreenPixelSize: Qt.size(280, 160)
    readonly property real viewportScale: viewport
        ? Math.max(0, Math.min(1, viewport.width / 1920,
                               viewport.height / 1080)) : 1
    readonly property size effectiveMinimumPixelSize: Qt.size(
        Math.max(1, minimumScreenPixelSize.width,
                 minimumPixelSize.width * viewportScale),
        Math.max(1, minimumScreenPixelSize.height,
                 minimumPixelSize.height * viewportScale))
    readonly property bool interactionActive: moveHandler.active
                                              || resizeInteractionCount > 0

    signal requestExitCustomization()
    signal interactionStateChanged(bool active)
    signal placementCommitted()

    property rect resolvedPixelRect: Qt.rect(0, 0, 1, 1)
    property var sourcePlacement: ({ "stored": false })
    property rect moveStartRect: Qt.rect(0, 0, 1, 1)
    property rect resizeStartRect: Qt.rect(0, 0, 1, 1)
    property int resizeInteractionCount: 0
    readonly property real resizeEdgeInset: customizeMode ? 16 : 8

    x: resolvedPixelRect.x
    y: resolvedPixelRect.y
    width: resolvedPixelRect.width
    height: resolvedPixelRect.height
    visible: forcedVisible
    focus: customizeMode
    activeFocusOnTab: customizeMode

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Arena overlay placement")
    Accessible.description: qsTr("Drag to move. Use arrow keys to move, Alt plus arrow keys to resize, and R to reset.")

    onInteractionActiveChanged: interactionStateChanged(interactionActive)

    function bounded(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function safeMargin(length) {
        return Math.min(24, Math.max(0, (length - 1) / 2));
    }

    function safePixelRect() {
        const marginX = safeMargin(viewport ? viewport.width : 0);
        const marginY = safeMargin(viewport ? viewport.height : 0);
        const viewportWidth = viewport ? viewport.width : 0;
        const viewportHeight = viewport ? viewport.height : 0;
        return Qt.rect(marginX, marginY,
                       Math.max(1, viewportWidth - 2 * marginX),
                       Math.max(1, viewportHeight - 2 * marginY));
    }

    function largestAdjacentRect(leaderboard, safe, gap) {
        const candidates = [
            Qt.rect(leaderboard.x + leaderboard.width + gap,
                    safe.y,
                    Math.max(0, safe.x + safe.width
                             - leaderboard.x - leaderboard.width - gap),
                    safe.height),
            Qt.rect(safe.x,
                    safe.y,
                    Math.max(0, leaderboard.x - gap - safe.x),
                    safe.height),
            Qt.rect(safe.x,
                    leaderboard.y + leaderboard.height + gap,
                    safe.width,
                    Math.max(0, safe.y + safe.height
                             - leaderboard.y - leaderboard.height - gap)),
            Qt.rect(safe.x,
                    safe.y,
                    safe.width,
                    Math.max(0, leaderboard.y - gap - safe.y))
        ];
        let largest = Qt.rect(safe.x, safe.y, 1, 1);
        let largestArea = 0;
        for (const candidate of candidates) {
            const area = candidate.width * candidate.height;
            if (candidate.width > 0 && candidate.height > 0
                    && area > largestArea) {
                largest = candidate;
                largestArea = area;
            }
        }
        return largest;
    }

    function adjacentChatRect() {
        const safe = safePixelRect();
        const leaderboard = resolvedPixelRect;
        const gap = 12;
        const targetWidth = Math.min(420, Math.max(320,
                                                   leaderboard.width));
        const targetHeight = Math.min(360, safe.height);
        const clampedY = bounded(leaderboard.y,
                                 safe.y,
                                 safe.y + safe.height - targetHeight);
        const rightWidth = safe.x + safe.width
                         - leaderboard.x - leaderboard.width - gap;
        if (rightWidth >= targetWidth) {
            return Qt.rect(leaderboard.x + leaderboard.width + gap,
                           clampedY, targetWidth, targetHeight);
        }
        const leftWidth = leaderboard.x - gap - safe.x;
        if (leftWidth >= targetWidth) {
            return Qt.rect(leaderboard.x - gap - targetWidth,
                           clampedY, targetWidth, targetHeight);
        }
        const horizontalWidth = Math.min(targetWidth, safe.width);
        const clampedX = bounded(leaderboard.x,
                                 safe.x,
                                 safe.x + safe.width - horizontalWidth);
        const belowHeight = safe.y + safe.height
                          - leaderboard.y - leaderboard.height - gap;
        if (belowHeight >= targetHeight) {
            return Qt.rect(clampedX,
                           leaderboard.y + leaderboard.height + gap,
                           horizontalWidth, targetHeight);
        }
        const aboveHeight = leaderboard.y - gap - safe.y;
        if (aboveHeight >= targetHeight) {
            return Qt.rect(clampedX,
                           leaderboard.y - gap - targetHeight,
                           horizontalWidth, targetHeight);
        }
        return largestAdjacentRect(leaderboard, safe, gap);
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
                                        Math.min(result ? 560 : 420,
                                                 viewportWidth * (result ? 0.40 : 0.30)));
        const requestedHeight = Math.max((result ? 260 : 240)
                                         * viewportScale,
                                         viewportHeight * (result ? 0.60 : 0.44));
        const width = Math.min(safe.width, requestedWidth);
        const height = Math.min(safe.height, requestedHeight);
        const requestedX = Math.max(0, viewportWidth - 24 - width);
        const requestedY = Math.min(24, Math.max(0, viewportHeight - height));
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
        return themeVars && viewport && propertyPrefix().length > 0
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

    function moveBy(x, y, commit) {
        resolvedPixelRect = clampPixelRect(
                    Qt.rect(resolvedPixelRect.x + x,
                            resolvedPixelRect.y + y,
                            resolvedPixelRect.width,
                            resolvedPixelRect.height));
        if (commit)
            commitResolvedPlacement();
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

    function keyboardResize(x, y) {
        const start = resolvedPixelRect;
        resizeFrom(start, x, y, x === 0 ? 0 : 1, y === 0 ? 0 : 1);
        commitResolvedPlacement();
    }

    function resizeByHandle(x, y, horizontalEdge, verticalEdge) {
        const start = resolvedPixelRect;
        resizeFrom(start, x, y, horizontalEdge, verticalEdge);
        commitResolvedPlacement();
    }

    function resetPlacement() {
        if (!canLoadPlacement())
            return;
        themeVars[propertyKey("XNormalized")] = -1;
        themeVars[propertyKey("YNormalized")] = -1;
        themeVars[propertyKey("WidthNormalized")] = -1;
        themeVars[propertyKey("HeightNormalized")] = -1;
        sourcePlacement = ({ "stored": false });
        resolvedPixelRect = defaultPixelRect();
        placementCommitted();
    }

    onThemeVarsChanged: reloadPlacement()
    onViewportChanged: reloadPlacement()
    onPlacementKindChanged: reloadPlacement()
    onLayoutVariantChanged: reloadPlacement()
    onMinimumPixelSizeChanged: reloadPlacement()
    onDefaultPixelRectHintChanged: reloadPlacement()

    Component.onCompleted: reloadPlacement()

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

    Keys.onPressed: event => {
        if (!customizeMode)
            return;
        const step = (event.modifiers & Qt.ShiftModifier) !== 0 ? 16 : 4;
        const resize = (event.modifiers & Qt.AltModifier) !== 0;
        let x = 0;
        let y = 0;
        if (event.key === Qt.Key_Left)
            x = -step;
        else if (event.key === Qt.Key_Right)
            x = step;
        else if (event.key === Qt.Key_Up)
            y = -step;
        else if (event.key === Qt.Key_Down)
            y = step;
        else if (event.key === Qt.Key_R) {
            resetPlacement();
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                   || event.key === Qt.Key_Escape) {
            requestExitCustomization();
            event.accepted = true;
            return;
        } else {
            return;
        }
        if (resize)
            keyboardResize(x, y);
        else
            moveBy(x, y, true);
        event.accepted = true;
    }

    Item {
        id: contentHost

        anchors.fill: parent
    }

    Rectangle {
        anchors.fill: parent
        visible: root.customizeMode
        color: "transparent"
        border.width: 2
        border.color: "#f5f7ff"
        z: 1000
    }

    Button {
        id: resetButton

        objectName: "arenaOverlayReset"
        width: 96
        height: 36
        x: 8
        y: 8
        visible: root.customizeMode
        enabled: root.customizeMode
        z: 1002
        text: qsTr("Reset")
        activeFocusOnTab: true

        Accessible.name: qsTr("Reset Arena overlay position")
        Accessible.description: qsTr("Restore the default Arena overlay size and position")
        onClicked: root.resetPlacement()

        background: Rectangle {
            radius: 3
            color: resetButton.down ? "#d8dce8" : "#f5f7ff"
            border.width: 2
            border.color: "#181b24"
        }

        contentItem: Text {
            text: resetButton.text
            color: "#181b24"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    DragHandler {
        id: moveHandler

        parent: root.moveHandle || root
        target: null
        enabled: root.customizeMode || root.directMoveEnabled
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
                            Qt.rect(root.moveStartRect.x + translation.x,
                                    root.moveStartRect.y + translation.y,
                                    root.moveStartRect.width,
                                    root.moveStartRect.height));
            }
        }
    }

    component ResizeHandle: ArenaOverlayResizeHandle {
        required property string handleObjectName

        objectName: handleObjectName
        interactionEnabled: root.customizeMode || root.directResizeEnabled
        chromeVisible: root.customizeMode

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
        width: Math.max(16, root.width - 2 * root.resizeEdgeInset)
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
        height: Math.max(16, root.height - 2 * root.resizeEdgeInset)
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
        width: Math.max(16, root.width - 2 * root.resizeEdgeInset)
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
        height: Math.max(16, root.height - 2 * root.resizeEdgeInset)
        x: -width / 2
        y: root.resizeEdgeInset
    }
}
