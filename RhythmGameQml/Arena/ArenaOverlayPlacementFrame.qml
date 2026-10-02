pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    required property var themeVars
    readonly property Item viewport: parent
    required property string layoutVariant
    property size minimumPixelSize: Qt.size(280, 160)
    property rect defaultPixelRectHint: Qt.rect(0, 0, 0, 0)
    property Item moveHandle: null
    property bool customizeMode: false
    property string customizationLabel: qsTr("Arena panel")
    property bool defaultExpanded: false
    property bool visibilityCustomizable: true
    readonly property bool overlayVisible: !visibilityCustomizable
        || placementState.setting("Visible", true)
    readonly property bool expanded: placementState.setting("Expanded", defaultExpanded)
    readonly property bool chatSelected: placementState.setting("ChatSelected", false)
    default property alias contentData: contentHost.data
    readonly property real viewportScale: viewport
        ? Math.min(viewport.width / 1920, viewport.height / 1080) : 1
    readonly property real effectiveContentScale: viewportScale > 0 ? viewportScale : 1
    readonly property real resizeEdgeInset: 8 * viewportScale

    x: placementState.pixelRect.x
    y: placementState.pixelRect.y
    width: placementState.pixelRect.width
    height: placementState.pixelRect.height
    visible: overlayVisible || customizeMode

    function setOverlayVisible(value) {
        placementState.setSetting("Visible", visibilityCustomizable ? !!value : true);
    }

    function setExpanded(value) {
        placementState.setSetting("Expanded", !!value);
    }

    function setChatSelected(value) {
        placementState.setSetting("ChatSelected", !!value);
    }

    // A gesture belongs to the settings and coordinate space where it started.
    onThemeVarsChanged: placementState.editRect = null
    onLayoutVariantChanged: placementState.editRect = null
    onViewportChanged: placementState.editRect = null

    QtObject {
        id: placementState

        readonly property string prefix: {
            switch (root.layoutVariant) {
            case "k5": return "arenaOverlayK5";
            case "k7": return "arenaOverlayK7";
            case "k10": return "arenaOverlayK10";
            case "k14": return "arenaOverlayK14";
            case "result": return "arenaOverlayResult";
            case "select": return "arenaOverlaySelect";
            default: return "";
            }
        }
        // Keep edits in memory when the theme opts out of persistence.
        property var localSettings: ({})
        readonly property var settings: root.themeVars && prefix
            ? root.themeVars : localSettings
        readonly property real viewportWidth: root.viewport ? root.viewport.width : 0
        readonly property real viewportHeight: root.viewport ? root.viewport.height : 0
        readonly property rect safeRect: {
            const marginX = Math.min(24 * root.viewportScale,
                                     Math.max(0, (viewportWidth - 1) / 2));
            const marginY = Math.min(24 * root.viewportScale,
                                     Math.max(0, (viewportHeight - 1) / 2));
            return Qt.rect(marginX, marginY,
                           Math.max(0, viewportWidth - 2 * marginX),
                           Math.max(0, viewportHeight - 2 * marginY));
        }
        readonly property real minimumWidth: Math.min(safeRect.width,
            root.minimumPixelSize.width * root.viewportScale)
        readonly property real minimumHeight: Math.min(safeRect.height,
            root.minimumPixelSize.height * root.viewportScale)
        readonly property rect savedPixelRect: {
            const x = setting("XNormalized", NaN);
            const y = setting("YNormalized", NaN);
            const width = setting("WidthNormalized", NaN);
            const height = setting("HeightNormalized", NaN);
            if (Number.isFinite(x) && Number.isFinite(y)
                    && Number.isFinite(width) && Number.isFinite(height)
                    && x >= 0 && y >= 0 && width > 0 && height > 0
                    && x + width <= 1.000001 && y + height <= 1.000001) {
                return Qt.rect(x * viewportWidth, y * viewportHeight,
                               width * viewportWidth, height * viewportHeight);
            }
            return defaultRect();
        }
        readonly property rect pixelRect: clamp(editRect || savedPixelRect)
        // Only a gesture needs a temporary rectangle. Otherwise settings drive it.
        property var editRect: null
        property rect startRect: Qt.rect(0, 0, 0, 0)
        property int resizeInteractions: 0

        function setting(suffix, fallback) {
            const value = settings[prefix + suffix];
            return typeof value === typeof fallback ? value : fallback;
        }

        function setSetting(suffix, value) {
            const key = prefix + suffix;
            if (root.themeVars && prefix)
                root.themeVars[key] = value;
            else
                localSettings = Object.assign({}, localSettings, { [key]: value });
        }

        function bounded(value, minimum, maximum) {
            return Math.max(minimum, Math.min(maximum, value));
        }

        function defaultRect() {
            const hint = root.defaultPixelRectHint;
            if (Number.isFinite(hint.x) && Number.isFinite(hint.y)
                    && Number.isFinite(hint.width) && Number.isFinite(hint.height)
                    && hint.width > 0 && hint.height > 0)
                return hint;
            const result = root.layoutVariant === "result";
            const width = Math.min(safeRect.width,
                Math.max((result ? 360 : 320) * root.viewportScale,
                         Math.min((result ? 560 : 420) * root.viewportScale,
                                  viewportWidth * (result ? 0.40 : 0.30))));
            const height = Math.min(safeRect.height,
                Math.max((result ? 260 : 240) * root.viewportScale,
                         viewportHeight * (result ? 0.60 : 0.44)));
            return Qt.rect(viewportWidth - 24 * root.viewportScale - width,
                           24 * root.viewportScale, width, height);
        }

        function clamp(candidate) {
            const width = bounded(candidate.width, minimumWidth, safeRect.width);
            const height = bounded(candidate.height, minimumHeight, safeRect.height);
            return Qt.rect(bounded(candidate.x, safeRect.x, safeRect.x + safeRect.width - width),
                           bounded(candidate.y, safeRect.y, safeRect.y + safeRect.height - height),
                           width, height);
        }

        function beginEdit() {
            startRect = pixelRect;
            editRect = startRect;
        }

        function commitEdit() {
            if (editRect && viewportWidth > 0 && viewportHeight > 0) {
                const rect = pixelRect;
                setSetting("XNormalized", rect.x / viewportWidth);
                setSetting("YNormalized", rect.y / viewportHeight);
                setSetting("WidthNormalized", rect.width / viewportWidth);
                setSetting("HeightNormalized", rect.height / viewportHeight);
            }
            editRect = null;
        }

        function move(x, y) {
            if (editRect)
                editRect = Qt.rect(startRect.x + x, startRect.y + y,
                                   startRect.width, startRect.height);
        }

        function resize(x, y, horizontalEdge, verticalEdge) {
            if (!editRect)
                return;
            let left = startRect.x;
            let top = startRect.y;
            let right = startRect.x + startRect.width;
            let bottom = startRect.y + startRect.height;
            if (horizontalEdge < 0)
                left = bounded(left + x, safeRect.x, right - minimumWidth);
            else if (horizontalEdge > 0)
                right = bounded(right + x, left + minimumWidth, safeRect.x + safeRect.width);
            if (verticalEdge < 0)
                top = bounded(top + y, safeRect.y, bottom - minimumHeight);
            else if (verticalEdge > 0)
                bottom = bounded(bottom + y, top + minimumHeight, safeRect.y + safeRect.height);
            editRect = Qt.rect(left, top, right - left, bottom - top);
        }

        function beginResize() {
            if (resizeInteractions === 0)
                beginEdit();
            ++resizeInteractions;
        }

        function endResize() {
            resizeInteractions = Math.max(0, resizeInteractions - 1);
            if (resizeInteractions === 0)
                commitEdit();
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
        parent: root.moveHandle || root
        target: null
        enabled: true
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (active)
                placementState.beginEdit();
            else
                placementState.commitEdit();
        }
        onTranslationChanged: {
            if (active)
                placementState.move(activeTranslation.x, activeTranslation.y);
        }
    }

    component ResizeHandle: ArenaOverlayResizeHandle {
        required property string handleObjectName

        objectName: handleObjectName
        interactionEnabled: true
        metricScale: root.viewportScale

        Accessible.ignored: true

        onInteractionStarted: placementState.beginResize()
        onInteractionDelta: function(x, y, horizontalEdge, verticalEdge) {
            placementState.resize(x, y, horizontalEdge, verticalEdge);
        }
        onInteractionEnded: placementState.endResize()
        onKeyboardResizeRequested: function(x, y, horizontalEdge,
                                            verticalEdge) {
            placementState.beginEdit();
            placementState.resize(x, y, horizontalEdge, verticalEdge);
            placementState.commitEdit();
        }
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTopLeft"
        horizontalEdge: -1
        verticalEdge: -1
        x: -width / 2
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTop"
        verticalEdge: -1
        width: Math.max(16 * root.viewportScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeTopRight"
        horizontalEdge: 1
        verticalEdge: -1
        x: root.width - width / 2
        y: -height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeRight"
        horizontalEdge: 1
        height: Math.max(16 * root.viewportScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: root.width - width / 2
        y: root.resizeEdgeInset
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottomRight"
        horizontalEdge: 1
        verticalEdge: 1
        x: root.width - width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottom"
        verticalEdge: 1
        width: Math.max(16 * root.viewportScale,
                        root.width - 2 * root.resizeEdgeInset)
        x: root.resizeEdgeInset
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeBottomLeft"
        horizontalEdge: -1
        verticalEdge: 1
        x: -width / 2
        y: root.height - height / 2
    }

    ResizeHandle {
        handleObjectName: "arenaResizeLeft"
        horizontalEdge: -1
        height: Math.max(16 * root.viewportScale,
                         root.height - 2 * root.resizeEdgeInset)
        x: -width / 2
        y: root.resizeEdgeInset
    }
}
