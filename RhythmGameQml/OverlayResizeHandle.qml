import QtQuick

Item {
    id: root

    required property bool interactionEnabled
    property real metricScale: 1
    property int horizontalEdge: 0
    property int verticalEdge: 0
    readonly property bool interactionActive: resizeHandler.active

    signal interactionStarted(int horizontalEdge, int verticalEdge)
    signal interactionDelta(real x, real y, int horizontalEdge, int verticalEdge)
    signal interactionEnded()
    signal keyboardResizeRequested(real x, real y, int horizontalEdge,
                                   int verticalEdge)

    width: 16 * metricScale
    height: 16 * metricScale
    visible: interactionEnabled
    enabled: interactionEnabled
    z: 1001

    Accessible.role: Accessible.Grip
    Accessible.focusable: false

    Accessible.onIncreaseAction: resizeOutward(1)
    Accessible.onDecreaseAction: resizeOutward(-1)

    function requestKeyboardResize(x, y) : void {
        const horizontalDelta = horizontalEdge === 0 ? 0 : x;
        const verticalDelta = verticalEdge === 0 ? 0 : y;
        if (horizontalDelta === 0 && verticalDelta === 0) {
            return;
        }
        keyboardResizeRequested(horizontalDelta, verticalDelta,
                                horizontalEdge, verticalEdge);
    }

    function resizeOutward(direction) : void {
        const step = 4 * root.metricScale * direction;
        requestKeyboardResize(horizontalEdge * step,
                              verticalEdge * step);
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: event => {
        if (!root.enabled) {
            return;
        }
        const step = ((event.modifiers & Qt.ShiftModifier) !== 0 ? 16 : 4)
                   * root.metricScale;
        if (event.key === Qt.Key_Left) {
            root.requestKeyboardResize(-step, 0);
        } else if (event.key === Qt.Key_Right) {
            root.requestKeyboardResize(step, 0);
        } else if (event.key === Qt.Key_Up) {
            root.requestKeyboardResize(0, -step);
        } else if (event.key === Qt.Key_Down) {
            root.requestKeyboardResize(0, step);
        } else {
            return;
        }
        event.accepted = true;
    }

    HoverHandler {
        enabled: root.enabled
        cursorShape: {
            if (root.horizontalEdge === root.verticalEdge)
                return Qt.SizeFDiagCursor;
            if (root.horizontalEdge === -root.verticalEdge)
                return Qt.SizeBDiagCursor;
            if (root.horizontalEdge !== 0)
                return Qt.SizeHorCursor;
            return Qt.SizeVerCursor;
        }
    }

    DragHandler {
        id: resizeHandler

        target: null
        enabled: root.enabled
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (active) {
                root.interactionStarted(root.horizontalEdge, root.verticalEdge);
            } else {
                root.interactionEnded();
            }
        }
        onTranslationChanged: {
            if (active) {
                root.interactionDelta(translation.x, translation.y,
                                      root.horizontalEdge, root.verticalEdge);
            }
        }
    }
}
