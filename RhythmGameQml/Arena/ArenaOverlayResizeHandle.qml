import QtQuick

Item {
    id: root

    required property bool interactionEnabled
    required property bool chromeVisible
    required property string accessibleName
    property int horizontalEdge: 0
    property int verticalEdge: 0
    readonly property bool interactionActive: resizeHandler.active
    readonly property bool focusIndicatorVisible: chromeVisible
                                                   && activeFocus && enabled

    signal interactionStarted(int horizontalEdge, int verticalEdge)
    signal interactionDelta(real x, real y, int horizontalEdge, int verticalEdge)
    signal interactionEnded()
    signal keyboardResizeRequested(real x, real y, int horizontalEdge,
                                   int verticalEdge)

    width: 32
    height: 32
    visible: interactionEnabled
    enabled: interactionEnabled
    activeFocusOnTab: chromeVisible && enabled
    z: 1001

    Accessible.role: Accessible.Grip
    Accessible.name: accessibleName
    Accessible.description: qsTr("Use arrow keys to resize. Hold Shift for larger steps.")
    Accessible.focusable: chromeVisible && enabled

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
        const step = 4 * direction;
        requestKeyboardResize(horizontalEdge * step,
                              verticalEdge * step);
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: event => {
        if (!root.enabled) {
            return;
        }
        const step = (event.modifiers & Qt.ShiftModifier) !== 0 ? 16 : 4;
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

    Rectangle {
        objectName: root.objectName + "Chrome"
        anchors.centerIn: parent
        width: root.horizontalEdge !== 0 && root.verticalEdge !== 0 ? 12 :
               root.horizontalEdge !== 0 ? 8 : 18
        height: root.horizontalEdge !== 0 && root.verticalEdge !== 0 ? 12 :
                root.verticalEdge !== 0 ? 8 : 18
        radius: 2
        color: "#f5f7ff"
        border.width: 2
        border.color: "#181b24"
        visible: root.chromeVisible

        Accessible.ignored: true
    }

    Rectangle {
        objectName: root.objectName + "FocusIndicator"
        anchors.fill: parent
        anchors.margins: 2
        border.color: "#8fdcff"
        border.width: 2
        color: "transparent"
        radius: 3
        visible: root.chromeVisible && root.focusIndicatorVisible

        Accessible.ignored: true
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
